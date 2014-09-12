/*	$NetBSD: sdhc.c,v 1.46 2014/09/12 19:47:40 jakllsch Exp $	*/
/*	$OpenBSD: sdhc.c,v 1.25 2009/01/13 19:44:20 grange Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * SD Host Controller driver based on the SD Host Controller Standard
 * Simplified Specification Version 1.00 (www.sdcard.com).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sdhc.c,v 1.46 2014/09/12 19:47:40 jakllsch Exp $");

#ifdef _KERNEL_OPT
#include "opt_sdmmc.h"
#endif

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

#ifdef SDHC_DEBUG
int sdhcdebug = 1;
#define DPRINTF(n,s)	do { if ((n) <= sdhcdebug) printf s; } while (0)
void	sdhc_dump_regs(struct sdhc_host *);
#else
#define DPRINTF(n,s)	do {} while (0)
#endif

#define SDHC_COMMAND_TIMEOUT	hz
#define SDHC_BUFFER_TIMEOUT	hz
#define SDHC_TRANSFER_TIMEOUT	hz
#define SDHC_DMA_TIMEOUT	hz

struct sdhc_host {
	struct sdhc_softc *sc;		/* host controller device */

	bus_space_tag_t iot;		/* host register set tag */
	bus_space_handle_t ioh;		/* host register set handle */
	bus_size_t ios;			/* host register space size */
	bus_dma_tag_t dmat;		/* host DMA tag */

	device_t sdmmc;			/* generic SD/MMC device */

	struct kmutex host_mtx;

	u_int clkbase;			/* base clock frequency in KHz */
	int maxblklen;			/* maximum block length */
	uint32_t ocr;			/* OCR value from capabilities */

	uint8_t regs[14];		/* host controller state */

	uint16_t intr_status;		/* soft interrupt status */
	uint16_t intr_error_status;	/* soft error status */
	struct kmutex intr_mtx;
	struct kcondvar intr_cv;

	int specver;			/* spec. version */

	uint32_t flags;			/* flags for this host */
#define SHF_USE_DMA		0x0001
#define SHF_USE_4BIT_MODE	0x0002
#define SHF_USE_8BIT_MODE	0x0004
};

#define HDEVNAME(hp)	(device_xname((hp)->sc->sc_dev))

static uint8_t
hread1(struct sdhc_host *hp, bus_size_t reg)
{

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS))
		return bus_space_read_1(hp->iot, hp->ioh, reg);
	return bus_space_read_4(hp->iot, hp->ioh, reg & -4) >> (8 * (reg & 3));
}

static uint16_t
hread2(struct sdhc_host *hp, bus_size_t reg)
{

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS))
		return bus_space_read_2(hp->iot, hp->ioh, reg);
	return bus_space_read_4(hp->iot, hp->ioh, reg & -4) >> (8 * (reg & 2));
}

#define HREAD1(hp, reg)		hread1(hp, reg)
#define HREAD2(hp, reg)		hread2(hp, reg)
#define HREAD4(hp, reg)		\
	(bus_space_read_4((hp)->iot, (hp)->ioh, (reg)))


static void
hwrite1(struct sdhc_host *hp, bus_size_t o, uint8_t val)
{

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
		bus_space_write_1(hp->iot, hp->ioh, o, val);
	} else {
		const size_t shift = 8 * (o & 3);
		o &= -4;
		uint32_t tmp = bus_space_read_4(hp->iot, hp->ioh, o);
		tmp = (val << shift) | (tmp & ~(0xff << shift));
		bus_space_write_4(hp->iot, hp->ioh, o, tmp);
	}
}

static void
hwrite2(struct sdhc_host *hp, bus_size_t o, uint16_t val)
{

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
		bus_space_write_2(hp->iot, hp->ioh, o, val);
	} else {
		const size_t shift = 8 * (o & 2);
		o &= -4;
		uint32_t tmp = bus_space_read_4(hp->iot, hp->ioh, o);
		tmp = (val << shift) | (tmp & ~(0xffff << shift));
		bus_space_write_4(hp->iot, hp->ioh, o, tmp);
	}
}

#define HWRITE1(hp, reg, val)		hwrite1(hp, reg, val)
#define HWRITE2(hp, reg, val)		hwrite2(hp, reg, val)
#define HWRITE4(hp, reg, val)						\
	bus_space_write_4((hp)->iot, (hp)->ioh, (reg), (val))

#define HCLR1(hp, reg, bits)						\
	do if (bits) HWRITE1((hp), (reg), HREAD1((hp), (reg)) & ~(bits)); while (0)
#define HCLR2(hp, reg, bits)						\
	do if (bits) HWRITE2((hp), (reg), HREAD2((hp), (reg)) & ~(bits)); while (0)
#define HCLR4(hp, reg, bits)						\
	do if (bits) HWRITE4((hp), (reg), HREAD4((hp), (reg)) & ~(bits)); while (0)
#define HSET1(hp, reg, bits)						\
	do if (bits) HWRITE1((hp), (reg), HREAD1((hp), (reg)) | (bits)); while (0)
#define HSET2(hp, reg, bits)						\
	do if (bits) HWRITE2((hp), (reg), HREAD2((hp), (reg)) | (bits)); while (0)
#define HSET4(hp, reg, bits)						\
	do if (bits) HWRITE4((hp), (reg), HREAD4((hp), (reg)) | (bits)); while (0)

static int	sdhc_host_reset(sdmmc_chipset_handle_t);
static int	sdhc_host_reset1(sdmmc_chipset_handle_t);
static uint32_t	sdhc_host_ocr(sdmmc_chipset_handle_t);
static int	sdhc_host_maxblklen(sdmmc_chipset_handle_t);
static int	sdhc_card_detect(sdmmc_chipset_handle_t);
static int	sdhc_write_protect(sdmmc_chipset_handle_t);
static int	sdhc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	sdhc_bus_clock(sdmmc_chipset_handle_t, int);
static int	sdhc_bus_width(sdmmc_chipset_handle_t, int);
static int	sdhc_bus_rod(sdmmc_chipset_handle_t, int);
static void	sdhc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	sdhc_card_intr_ack(sdmmc_chipset_handle_t);
static void	sdhc_exec_command(sdmmc_chipset_handle_t,
		    struct sdmmc_command *);
static int	sdhc_start_command(struct sdhc_host *, struct sdmmc_command *);
static int	sdhc_wait_state(struct sdhc_host *, uint32_t, uint32_t);
static int	sdhc_soft_reset(struct sdhc_host *, int);
static int	sdhc_wait_intr(struct sdhc_host *, int, int);
static void	sdhc_transfer_data(struct sdhc_host *, struct sdmmc_command *);
static int	sdhc_transfer_data_dma(struct sdhc_host *, struct sdmmc_command *);
static int	sdhc_transfer_data_pio(struct sdhc_host *, struct sdmmc_command *);
static void	sdhc_read_data_pio(struct sdhc_host *, uint8_t *, u_int);
static void	sdhc_write_data_pio(struct sdhc_host *, uint8_t *, u_int);
static void	esdhc_read_data_pio(struct sdhc_host *, uint8_t *, u_int);
static void	esdhc_write_data_pio(struct sdhc_host *, uint8_t *, u_int);


static struct sdmmc_chip_functions sdhc_functions = {
	/* host controller reset */
	sdhc_host_reset,

	/* host controller capabilities */
	sdhc_host_ocr,
	sdhc_host_maxblklen,

	/* card detection */
	sdhc_card_detect,

	/* write protect */
	sdhc_write_protect,

	/* bus power, clock frequency and width */
	sdhc_bus_power,
	sdhc_bus_clock,
	sdhc_bus_width,
	sdhc_bus_rod,

	/* command execution */
	sdhc_exec_command,

	/* card interrupt */
	sdhc_card_enable_intr,
	sdhc_card_intr_ack
};

static int
sdhc_cfprint(void *aux, const char *pnp)
{
	const struct sdmmcbus_attach_args * const saa = aux;
	const struct sdhc_host * const hp = saa->saa_sch;
	
	if (pnp) {
		aprint_normal("sdmmc at %s", pnp);
	}
	for (size_t host = 0; host < hp->sc->sc_nhosts; host++) {
		if (hp->sc->sc_host[host] == hp) {
			aprint_normal(" slot %zu", host);
		}
	}

	return UNCONF;
}

/*
 * Called by attachment driver.  For each SD card slot there is one SD
 * host controller standard register set. (1.3)
 */
int
sdhc_host_found(struct sdhc_softc *sc, bus_space_tag_t iot,
    bus_space_handle_t ioh, bus_size_t iosize)
{
	struct sdmmcbus_attach_args saa;
	struct sdhc_host *hp;
	uint32_t caps;
	uint16_t sdhcver;

	/* Allocate one more host structure. */
	hp = malloc(sizeof(struct sdhc_host), M_DEVBUF, M_WAITOK|M_ZERO);
	if (hp == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't alloc memory (sdhc host)\n");
		goto err1;
	}
	sc->sc_host[sc->sc_nhosts++] = hp;

	/* Fill in the new host structure. */
	hp->sc = sc;
	hp->iot = iot;
	hp->ioh = ioh;
	hp->ios = iosize;
	hp->dmat = sc->sc_dmat;

	mutex_init(&hp->host_mtx, MUTEX_DEFAULT, IPL_SDMMC);
	mutex_init(&hp->intr_mtx, MUTEX_DEFAULT, IPL_SDMMC);
	cv_init(&hp->intr_cv, "sdhcintr");

	sdhcver = HREAD2(hp, SDHC_HOST_CTL_VERSION);
	aprint_normal_dev(sc->sc_dev, "SD Host Specification ");
	hp->specver = SDHC_SPEC_VERSION(sdhcver);
	switch (SDHC_SPEC_VERSION(sdhcver)) {
	case SDHC_SPEC_VERS_100:
		aprint_normal("1.0");
		break;

	case SDHC_SPEC_VERS_200:
		aprint_normal("2.0");
		break;

	case SDHC_SPEC_VERS_300:
		aprint_normal("3.0");
		break;

	default:
		aprint_normal("unknown version(0x%x)",
		    SDHC_SPEC_VERSION(sdhcver));
		break;
	}
	aprint_normal(", rev.%u\n", SDHC_VENDOR_VERSION(sdhcver));

	/*
	 * Reset the host controller and enable interrupts.
	 */
	(void)sdhc_host_reset(hp);

	/* Determine host capabilities. */
	if (ISSET(sc->sc_flags, SDHC_FLAG_HOSTCAPS)) {
		caps = sc->sc_caps;
	} else {
		mutex_enter(&hp->host_mtx);
		caps = HREAD4(hp, SDHC_CAPABILITIES);
		mutex_exit(&hp->host_mtx);
	}

	/* Use DMA if the host system and the controller support it. */
	if (ISSET(sc->sc_flags, SDHC_FLAG_FORCE_DMA) ||
	    (ISSET(sc->sc_flags, SDHC_FLAG_USE_DMA &&
	     ISSET(caps, SDHC_DMA_SUPPORT)))) {
		SET(hp->flags, SHF_USE_DMA);
		aprint_normal_dev(sc->sc_dev, "using DMA transfer\n");
	}

	/*
	 * Determine the base clock frequency. (2.2.24)
	 */
	if (hp->specver == SDHC_SPEC_VERS_300) {
		hp->clkbase = SDHC_BASE_V3_FREQ_KHZ(caps);
	} else {
		hp->clkbase = SDHC_BASE_FREQ_KHZ(caps);
	}
	if (hp->clkbase == 0) {
		if (sc->sc_clkbase == 0) {
			/* The attachment driver must tell us. */
			aprint_error_dev(sc->sc_dev,
			    "unknown base clock frequency\n");
			goto err;
		}
		hp->clkbase = sc->sc_clkbase;
	}
	if (hp->clkbase < 10000 || hp->clkbase > 10000 * 256) {
		/* SDHC 1.0 supports only 10-63 MHz. */
		aprint_error_dev(sc->sc_dev,
		    "base clock frequency out of range: %u MHz\n",
		    hp->clkbase / 1000);
		goto err;
	}
	DPRINTF(1,("%s: base clock frequency %u MHz\n",
	    device_xname(sc->sc_dev), hp->clkbase / 1000));

	/*
	 * XXX Set the data timeout counter value according to
	 * capabilities. (2.2.15)
	 */
	HWRITE1(hp, SDHC_TIMEOUT_CTL, SDHC_TIMEOUT_MAX);
#if 1
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED))
		HWRITE4(hp, SDHC_NINTR_STATUS, SDHC_CMD_TIMEOUT_ERROR << 16);
#endif

	/*
	 * Determine SD bus voltage levels supported by the controller.
	 */
	if (ISSET(caps, SDHC_EMBEDDED_SLOT) &&
	    ISSET(caps, SDHC_VOLTAGE_SUPP_1_8V)) {
		SET(hp->ocr, MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V);
	}
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_3_0V)) {
		SET(hp->ocr, MMC_OCR_2_9V_3_0V | MMC_OCR_3_0V_3_1V);
	}
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_3_3V)) {
		SET(hp->ocr, MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V);
	}

	/*
	 * Determine the maximum block length supported by the host
	 * controller. (2.2.24)
	 */
	switch((caps >> SDHC_MAX_BLK_LEN_SHIFT) & SDHC_MAX_BLK_LEN_MASK) {
	case SDHC_MAX_BLK_LEN_512:
		hp->maxblklen = 512;
		break;

	case SDHC_MAX_BLK_LEN_1024:
		hp->maxblklen = 1024;
		break;

	case SDHC_MAX_BLK_LEN_2048:
		hp->maxblklen = 2048;
		break;

	case SDHC_MAX_BLK_LEN_4096:
		hp->maxblklen = 4096;
		break;

	default:
		aprint_error_dev(sc->sc_dev, "max block length unknown\n");
		goto err;
	}
	DPRINTF(1, ("%s: max block length %u byte%s\n",
	    device_xname(sc->sc_dev), hp->maxblklen,
	    hp->maxblklen > 1 ? "s" : ""));

	/*
	 * Attach the generic SD/MMC bus driver.  (The bus driver must
	 * not invoke any chipset functions before it is attached.)
	 */
	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &sdhc_functions;
	saa.saa_sch = hp;
	saa.saa_dmat = hp->dmat;
	saa.saa_clkmax = hp->clkbase;
	if (ISSET(sc->sc_flags, SDHC_FLAG_HAVE_CGM))
		saa.saa_clkmin = hp->clkbase / 256 / 2046;
	else if (ISSET(sc->sc_flags, SDHC_FLAG_HAVE_DVS))
		saa.saa_clkmin = hp->clkbase / 256 / 16;
	else if (hp->sc->sc_clkmsk != 0)
		saa.saa_clkmin = hp->clkbase / (hp->sc->sc_clkmsk >>
		    (ffs(hp->sc->sc_clkmsk) - 1));
	else if (hp->specver == SDHC_SPEC_VERS_300)
		saa.saa_clkmin = hp->clkbase / 0x3ff;
	else
		saa.saa_clkmin = hp->clkbase / 256;
	saa.saa_caps = SMC_CAPS_4BIT_MODE|SMC_CAPS_AUTO_STOP;
	if (ISSET(sc->sc_flags, SDHC_FLAG_8BIT_MODE))
		saa.saa_caps |= SMC_CAPS_8BIT_MODE;
	if (ISSET(caps, SDHC_HIGH_SPEED_SUPP))
		saa.saa_caps |= SMC_CAPS_SD_HIGHSPEED;
	if (ISSET(hp->flags, SHF_USE_DMA)) {
		saa.saa_caps |= SMC_CAPS_DMA | SMC_CAPS_MULTI_SEG_DMA;
	}
	if (ISSET(sc->sc_flags, SDHC_FLAG_SINGLE_ONLY))
		saa.saa_caps |= SMC_CAPS_SINGLE_ONLY;
	hp->sdmmc = config_found(sc->sc_dev, &saa, sdhc_cfprint);

	return 0;

err:
	cv_destroy(&hp->intr_cv);
	mutex_destroy(&hp->intr_mtx);
	mutex_destroy(&hp->host_mtx);
	free(hp, M_DEVBUF);
	sc->sc_host[--sc->sc_nhosts] = NULL;
err1:
	return 1;
}

int
sdhc_detach(struct sdhc_softc *sc, int flags)
{
	struct sdhc_host *hp;
	int rv = 0;

	for (size_t n = 0; n < sc->sc_nhosts; n++) {
		hp = sc->sc_host[n];
		if (hp == NULL)
			continue;
		if (hp->sdmmc != NULL) {
			rv = config_detach(hp->sdmmc, flags);
			if (rv)
				break;
			hp->sdmmc = NULL;
		}
		/* disable interrupts */
		if ((flags & DETACH_FORCE) == 0) {
			if (ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
				HWRITE4(hp, SDHC_NINTR_SIGNAL_EN, 0);
			} else {
				HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, 0);
			}
			sdhc_soft_reset(hp, SDHC_RESET_ALL);
		}
		cv_destroy(&hp->intr_cv);
		mutex_destroy(&hp->intr_mtx);
		mutex_destroy(&hp->host_mtx);
		if (hp->ios > 0) {
			bus_space_unmap(hp->iot, hp->ioh, hp->ios);
			hp->ios = 0;
		}
		free(hp, M_DEVBUF);
		sc->sc_host[n] = NULL;
	}

	return rv;
}

bool
sdhc_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct sdhc_softc *sc = device_private(dev);
	struct sdhc_host *hp;
	size_t i;

	/* XXX poll for command completion or suspend command
	 * in progress */

	/* Save the host controller state. */
	for (size_t n = 0; n < sc->sc_nhosts; n++) {
		hp = sc->sc_host[n];
		if (ISSET(sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
			for (i = 0; i < sizeof hp->regs; i += 4) {
				uint32_t v = HREAD4(hp, i);
				hp->regs[i + 0] = (v >> 0);
				hp->regs[i + 1] = (v >> 8);
				if (i + 3 < sizeof hp->regs) {
					hp->regs[i + 2] = (v >> 16);
					hp->regs[i + 3] = (v >> 24);
				}
			}
		} else {
			for (i = 0; i < sizeof hp->regs; i++) {
				hp->regs[i] = HREAD1(hp, i);
			}
		}
	}
	return true;
}

bool
sdhc_resume(device_t dev, const pmf_qual_t *qual)
{
	struct sdhc_softc *sc = device_private(dev);
	struct sdhc_host *hp;
	size_t i;

	/* Restore the host controller state. */
	for (size_t n = 0; n < sc->sc_nhosts; n++) {
		hp = sc->sc_host[n];
		(void)sdhc_host_reset(hp);
		if (ISSET(sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
			for (i = 0; i < sizeof hp->regs; i += 4) {
				if (i + 3 < sizeof hp->regs) {
					HWRITE4(hp, i,
					    (hp->regs[i + 0] << 0)
					    | (hp->regs[i + 1] << 8)
					    | (hp->regs[i + 2] << 16)
					    | (hp->regs[i + 3] << 24));
				} else {
					HWRITE4(hp, i,
					    (hp->regs[i + 0] << 0)
					    | (hp->regs[i + 1] << 8));
				}
			}
		} else {
			for (i = 0; i < sizeof hp->regs; i++) {
				HWRITE1(hp, i, hp->regs[i]);
			}
		}
	}
	return true;
}

bool
sdhc_shutdown(device_t dev, int flags)
{
	struct sdhc_softc *sc = device_private(dev);
	struct sdhc_host *hp;

	/* XXX chip locks up if we don't disable it before reboot. */
	for (size_t i = 0; i < sc->sc_nhosts; i++) {
		hp = sc->sc_host[i];
		(void)sdhc_host_reset(hp);
	}
	return true;
}

/*
 * Reset the host controller.  Called during initialization, when
 * cards are removed, upon resume, and during error recovery.
 */
static int
sdhc_host_reset1(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	uint32_t sdhcimask;
	int error;

	/* Don't lock. */

	/* Disable all interrupts. */
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
		HWRITE4(hp, SDHC_NINTR_SIGNAL_EN, 0);
	} else {
		HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, 0);
	}

	/*
	 * Reset the entire host controller and wait up to 100ms for
	 * the controller to clear the reset bit.
	 */
	error = sdhc_soft_reset(hp, SDHC_RESET_ALL);
	if (error)
		goto out;

	/* Set data timeout counter value to max for now. */
	HWRITE1(hp, SDHC_TIMEOUT_CTL, SDHC_TIMEOUT_MAX);
#if 1
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED))
		HWRITE4(hp, SDHC_NINTR_STATUS, SDHC_CMD_TIMEOUT_ERROR << 16);
#endif

	/* Enable interrupts. */
	mutex_enter(&hp->intr_mtx);
	sdhcimask = SDHC_CARD_REMOVAL | SDHC_CARD_INSERTION |
	    SDHC_BUFFER_READ_READY | SDHC_BUFFER_WRITE_READY |
	    SDHC_DMA_INTERRUPT | SDHC_BLOCK_GAP_EVENT |
	    SDHC_TRANSFER_COMPLETE | SDHC_COMMAND_COMPLETE;
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
		sdhcimask |= SDHC_EINTR_STATUS_MASK << 16;
		HWRITE4(hp, SDHC_NINTR_STATUS_EN, sdhcimask);
		sdhcimask ^=
		    (SDHC_EINTR_STATUS_MASK ^ SDHC_EINTR_SIGNAL_MASK) << 16;
		sdhcimask ^= SDHC_BUFFER_READ_READY ^ SDHC_BUFFER_WRITE_READY;
		HWRITE4(hp, SDHC_NINTR_SIGNAL_EN, sdhcimask);
	} else {
		HWRITE2(hp, SDHC_NINTR_STATUS_EN, sdhcimask);
		HWRITE2(hp, SDHC_EINTR_STATUS_EN, SDHC_EINTR_STATUS_MASK);
		sdhcimask ^= SDHC_BUFFER_READ_READY ^ SDHC_BUFFER_WRITE_READY;
		HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, sdhcimask);
		HWRITE2(hp, SDHC_EINTR_SIGNAL_EN, SDHC_EINTR_SIGNAL_MASK);
	}
	mutex_exit(&hp->intr_mtx);

out:
	return error;
}

static int
sdhc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int error;

	mutex_enter(&hp->host_mtx);
	error = sdhc_host_reset1(sch);
	mutex_exit(&hp->host_mtx);

	return error;
}

static uint32_t
sdhc_host_ocr(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	return hp->ocr;
}

static int
sdhc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	return hp->maxblklen;
}

/*
 * Return non-zero if the card is currently inserted.
 */
static int
sdhc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int r;

	if (hp->sc->sc_vendor_card_detect)
		return (*hp->sc->sc_vendor_card_detect)(hp->sc);

	mutex_enter(&hp->host_mtx);
	r = ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_CARD_INSERTED);
	mutex_exit(&hp->host_mtx);

	return r ? 1 : 0;
}

/*
 * Return non-zero if the card is currently write-protected.
 */
static int
sdhc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int r;

	if (hp->sc->sc_vendor_write_protect)
		return (*hp->sc->sc_vendor_write_protect)(hp->sc);

	mutex_enter(&hp->host_mtx);
	r = ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_WRITE_PROTECT_SWITCH);
	mutex_exit(&hp->host_mtx);

	return r ? 0 : 1;
}

/*
 * Set or change SD bus voltage and enable or disable SD bus power.
 * Return zero on success.
 */
static int
sdhc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	uint8_t vdd;
	int error = 0;
	const uint32_t pcmask =
	    ~(SDHC_BUS_POWER | (SDHC_VOLTAGE_MASK << SDHC_VOLTAGE_SHIFT));

	mutex_enter(&hp->host_mtx);

	/*
	 * Disable bus power before voltage change.
	 */
	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)
	    && !ISSET(hp->sc->sc_flags, SDHC_FLAG_NO_PWR0))
		HWRITE1(hp, SDHC_POWER_CTL, 0);

	/* If power is disabled, reset the host and return now. */
	if (ocr == 0) {
		(void)sdhc_host_reset1(hp);
		goto out;
	}

	/*
	 * Select the lowest voltage according to capabilities.
	 */
	ocr &= hp->ocr;
	if (ISSET(ocr, MMC_OCR_1_7V_1_8V|MMC_OCR_1_8V_1_9V)) {
		vdd = SDHC_VOLTAGE_1_8V;
	} else if (ISSET(ocr, MMC_OCR_2_9V_3_0V|MMC_OCR_3_0V_3_1V)) {
		vdd = SDHC_VOLTAGE_3_0V;
	} else if (ISSET(ocr, MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V)) {
		vdd = SDHC_VOLTAGE_3_3V;
	} else {
		/* Unsupported voltage level requested. */
		error = EINVAL;
		goto out;
	}

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		/*
		 * Enable bus power.  Wait at least 1 ms (or 74 clocks) plus
		 * voltage ramp until power rises.
		 */
		HWRITE1(hp, SDHC_POWER_CTL,
		    HREAD1(hp, SDHC_POWER_CTL) & pcmask);
		sdmmc_delay(1);
		HWRITE1(hp, SDHC_POWER_CTL, (vdd << SDHC_VOLTAGE_SHIFT));
		sdmmc_delay(1);
		HSET1(hp, SDHC_POWER_CTL, SDHC_BUS_POWER);
		sdmmc_delay(10000);

		/*
		 * The host system may not power the bus due to battery low,
		 * etc.  In that case, the host controller should clear the
		 * bus power bit.
		 */
		if (!ISSET(HREAD1(hp, SDHC_POWER_CTL), SDHC_BUS_POWER)) {
			error = ENXIO;
			goto out;
		}
	}

out:
	mutex_exit(&hp->host_mtx);

	return error;
}

/*
 * Return the smallest possible base clock frequency divisor value
 * for the CLOCK_CTL register to produce `freq' (KHz).
 */
static bool
sdhc_clock_divisor(struct sdhc_host *hp, u_int freq, u_int *divp)
{
	u_int div;

	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_HAVE_CGM)) {
		for (div = hp->clkbase / freq; div <= 0x3ff; div++) {
			if ((hp->clkbase / div) <= freq) {
				*divp = SDHC_SDCLK_CGM
				    | ((div & 0x300) << SDHC_SDCLK_XDIV_SHIFT)
				    | ((div & 0x0ff) << SDHC_SDCLK_DIV_SHIFT);
				//freq = hp->clkbase / div;
				return true;
			}
		}
		/* No divisor found. */
		return false;
	}
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_HAVE_DVS)) {
		u_int dvs = (hp->clkbase + freq - 1) / freq;
		u_int roundup = dvs & 1;
		for (dvs >>= 1, div = 1; div <= 256; div <<= 1, dvs >>= 1) {
			if (dvs + roundup <= 16) {
				dvs += roundup - 1;
				*divp = (div << SDHC_SDCLK_DIV_SHIFT)
				    |   (dvs << SDHC_SDCLK_DVS_SHIFT);
				DPRINTF(2,
				    ("%s: divisor for freq %u is %u * %u\n",
				    HDEVNAME(hp), freq, div * 2, dvs + 1));
				//freq = hp->clkbase / (div * 2) * (dvs + 1);
				return true;
			}
			/*
			 * If we drop bits, we need to round up the divisor.
			 */
			roundup |= dvs & 1;
		}
		/* No divisor found. */
		return false;
	}
	if (hp->sc->sc_clkmsk != 0) {
		div = howmany(hp->clkbase, freq);
		if (div > (hp->sc->sc_clkmsk >> (ffs(hp->sc->sc_clkmsk) - 1)))
			return false;
		*divp = div << (ffs(hp->sc->sc_clkmsk) - 1);
		//freq = hp->clkbase / div;
		return true;
	}
	if (hp->specver == SDHC_SPEC_VERS_300) {
		div = howmany(hp->clkbase, freq);
		if (div > 0x3ff)
			return false;
		*divp = (((div >> 8) & SDHC_SDCLK_XDIV_MASK)
			 << SDHC_SDCLK_XDIV_SHIFT) |
			(((div >> 0) & SDHC_SDCLK_DIV_MASK)
			 << SDHC_SDCLK_DIV_SHIFT);
		//freq = hp->clkbase / div;
		return true;
	} else {
		for (div = 1; div <= 256; div *= 2) {
			if ((hp->clkbase / div) <= freq) {
				*divp = (div / 2) << SDHC_SDCLK_DIV_SHIFT;
				//freq = hp->clkbase / div;
				return true;
			}
		}
		/* No divisor found. */
		return false;
	}
	/* No divisor found. */
	return false;
}

/*
 * Set or change SDCLK frequency or disable the SD clock.
 * Return zero on success.
 */
static int
sdhc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	u_int div;
	u_int timo;
	int16_t reg;
	int error = 0;
#ifdef DIAGNOSTIC
	bool present;

	mutex_enter(&hp->host_mtx);
	present = ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_CMD_INHIBIT_MASK);
	mutex_exit(&hp->host_mtx);

	/* Must not stop the clock if commands are in progress. */
	if (present && sdhc_card_detect(hp)) {
		aprint_normal_dev(hp->sc->sc_dev,
		    "%s: command in progress\n", __func__);
	}
#endif

	mutex_enter(&hp->host_mtx);

	if (hp->sc->sc_vendor_bus_clock) {
		error = (*hp->sc->sc_vendor_bus_clock)(hp->sc, freq);
		if (error != 0)
			goto out;
	}

	/*
	 * Stop SD clock before changing the frequency.
	 */
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		HCLR4(hp, SDHC_CLOCK_CTL, 0xfff8);
		if (freq == SDMMC_SDCLK_OFF) {
			HSET4(hp, SDHC_CLOCK_CTL, 0x80f0);
			goto out;
		}
	} else {
		HCLR2(hp, SDHC_CLOCK_CTL, SDHC_SDCLK_ENABLE);
		if (freq == SDMMC_SDCLK_OFF)
			goto out;
	}

	/*
	 * Set the minimum base clock frequency divisor.
	 */
	if (!sdhc_clock_divisor(hp, freq, &div)) {
		/* Invalid base clock frequency or `freq' value. */
		error = EINVAL;
		goto out;
	}
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		HWRITE4(hp, SDHC_CLOCK_CTL,
		    div | (SDHC_TIMEOUT_MAX << 16));
	} else {
		reg = HREAD2(hp, SDHC_CLOCK_CTL);
		reg &= (SDHC_INTCLK_STABLE | SDHC_INTCLK_ENABLE);
		HWRITE2(hp, SDHC_CLOCK_CTL, reg | div);
	}

	/*
	 * Start internal clock.  Wait 10ms for stabilization.
	 */
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		sdmmc_delay(10000);
		HSET4(hp, SDHC_CLOCK_CTL,
		    8 | SDHC_INTCLK_ENABLE | SDHC_INTCLK_STABLE);
	} else {
		HSET2(hp, SDHC_CLOCK_CTL, SDHC_INTCLK_ENABLE);
		for (timo = 1000; timo > 0; timo--) {
			if (ISSET(HREAD2(hp, SDHC_CLOCK_CTL),
			    SDHC_INTCLK_STABLE))
				break;
			sdmmc_delay(10);
		}
		if (timo == 0) {
			error = ETIMEDOUT;
			goto out;
		}
	}

	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		HSET1(hp, SDHC_SOFTWARE_RESET, SDHC_INIT_ACTIVE);
		/*
		 * Sending 80 clocks at 400kHz takes 200us.
		 * So delay for that time + slop and then
		 * check a few times for completion.
		 */
		sdmmc_delay(210);
		for (timo = 10; timo > 0; timo--) {
			if (!ISSET(HREAD1(hp, SDHC_SOFTWARE_RESET),
			    SDHC_INIT_ACTIVE))
				break;
			sdmmc_delay(10);
		}
		DPRINTF(2,("%s: %u init spins\n", __func__, 10 - timo));

		/*
		 * Enable SD clock.
		 */
		HSET4(hp, SDHC_CLOCK_CTL, SDHC_SDCLK_ENABLE);
	} else {
		/*
		 * Enable SD clock.
		 */
		HSET2(hp, SDHC_CLOCK_CTL, SDHC_SDCLK_ENABLE);

		if (freq > 25000 &&
		    !ISSET(hp->sc->sc_flags, SDHC_FLAG_NO_HS_BIT))
			HSET1(hp, SDHC_HOST_CTL, SDHC_HIGH_SPEED);
		else
			HCLR1(hp, SDHC_HOST_CTL, SDHC_HIGH_SPEED);
	}

out:
	mutex_exit(&hp->host_mtx);

	return error;
}

static int
sdhc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int reg;

	switch (width) {
	case 1:
	case 4:
		break;

	case 8:
		if (ISSET(hp->sc->sc_flags, SDHC_FLAG_8BIT_MODE))
			break;
		/* FALLTHROUGH */
	default:
		DPRINTF(0,("%s: unsupported bus width (%d)\n",
		    HDEVNAME(hp), width));
		return 1;
	}

	mutex_enter(&hp->host_mtx);
	reg = HREAD1(hp, SDHC_HOST_CTL);
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		reg &= ~(SDHC_4BIT_MODE|SDHC_ESDHC_8BIT_MODE);
		if (width == 4)
			reg |= SDHC_4BIT_MODE;
		else if (width == 8)
			reg |= SDHC_ESDHC_8BIT_MODE;
	} else {
		reg &= ~SDHC_4BIT_MODE;
		if (width == 4)
			reg |= SDHC_4BIT_MODE;
	}
	HWRITE1(hp, SDHC_HOST_CTL, reg);
	mutex_exit(&hp->host_mtx);

	return 0;
}

static int
sdhc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	if (hp->sc->sc_vendor_rod)
		return (*hp->sc->sc_vendor_rod)(hp->sc, on);

	return 0;
}

static void
sdhc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		mutex_enter(&hp->intr_mtx);
		if (enable) {
			HSET2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
			HSET2(hp, SDHC_NINTR_SIGNAL_EN, SDHC_CARD_INTERRUPT);
		} else {
			HCLR2(hp, SDHC_NINTR_SIGNAL_EN, SDHC_CARD_INTERRUPT);
			HCLR2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
		}
		mutex_exit(&hp->intr_mtx);
	}
}

static void 
sdhc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		mutex_enter(&hp->intr_mtx);
		HSET2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
		mutex_exit(&hp->intr_mtx);
	}
}

static int
sdhc_wait_state(struct sdhc_host *hp, uint32_t mask, uint32_t value)
{
	uint32_t state;
	int timeout;

	for (timeout = 10; timeout > 0; timeout--) {
		if (((state = HREAD4(hp, SDHC_PRESENT_STATE)) & mask) == value)
			return 0;
		sdmmc_delay(10000);
	}
	DPRINTF(0,("%s: timeout waiting for %x (state=%x)\n", HDEVNAME(hp),
	    value, state));
	return ETIMEDOUT;
}

static void
sdhc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int error;

	if (cmd->c_data && ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		const uint16_t ready = SDHC_BUFFER_READ_READY | SDHC_BUFFER_WRITE_READY;
		mutex_enter(&hp->intr_mtx);
		if (ISSET(hp->flags, SHF_USE_DMA)) {
			HCLR2(hp, SDHC_NINTR_SIGNAL_EN, ready);
			HCLR2(hp, SDHC_NINTR_STATUS_EN, ready);
		} else {
			HSET2(hp, SDHC_NINTR_SIGNAL_EN, ready);
			HSET2(hp, SDHC_NINTR_STATUS_EN, ready);
		}  
		mutex_exit(&hp->intr_mtx);
	}

	/*
	 * Start the MMC command, or mark `cmd' as failed and return.
	 */
	error = sdhc_start_command(hp, cmd);
	if (error) {
		cmd->c_error = error;
		goto out;
	}

	/*
	 * Wait until the command phase is done, or until the command
	 * is marked done for any other reason.
	 */
	if (!sdhc_wait_intr(hp, SDHC_COMMAND_COMPLETE, SDHC_COMMAND_TIMEOUT)) {
		cmd->c_error = ETIMEDOUT;
		goto out;
	}

	/*
	 * The host controller removes bits [0:7] from the response
	 * data (CRC) and we pass the data up unchanged to the bus
	 * driver (without padding).
	 */
	mutex_enter(&hp->host_mtx);
	if (cmd->c_error == 0 && ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		cmd->c_resp[0] = HREAD4(hp, SDHC_RESPONSE + 0);
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			cmd->c_resp[1] = HREAD4(hp, SDHC_RESPONSE + 4);
			cmd->c_resp[2] = HREAD4(hp, SDHC_RESPONSE + 8);
			cmd->c_resp[3] = HREAD4(hp, SDHC_RESPONSE + 12);
			if (ISSET(hp->sc->sc_flags, SDHC_FLAG_RSP136_CRC)) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) |
				    (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) |
				    (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) |
				    (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		}
	}
	mutex_exit(&hp->host_mtx);
	DPRINTF(1,("%s: resp = %08x\n", HDEVNAME(hp), cmd->c_resp[0]));

	/*
	 * If the command has data to transfer in any direction,
	 * execute the transfer now.
	 */
	if (cmd->c_error == 0 && cmd->c_data != NULL)
		sdhc_transfer_data(hp, cmd);
	else if (ISSET(cmd->c_flags, SCF_RSP_BSY)) {
		if (!sdhc_wait_intr(hp, SDHC_TRANSFER_COMPLETE, hz * 10)) {
			cmd->c_error = ETIMEDOUT;
			goto out;
		}
	}

out:
	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)
	    && !ISSET(hp->sc->sc_flags, SDHC_FLAG_NO_LED_ON)) {
		mutex_enter(&hp->host_mtx);
		/* Turn off the LED. */
		HCLR1(hp, SDHC_HOST_CTL, SDHC_LED_ON);
		mutex_exit(&hp->host_mtx);
	}
	SET(cmd->c_flags, SCF_ITSDONE);

	DPRINTF(1,("%s: cmd %d %s (flags=%08x error=%d)\n", HDEVNAME(hp),
	    cmd->c_opcode, (cmd->c_error == 0) ? "done" : "abort",
	    cmd->c_flags, cmd->c_error));
}

static int
sdhc_start_command(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	struct sdhc_softc * const sc = hp->sc;
	uint16_t blksize = 0;
	uint16_t blkcount = 0;
	uint16_t mode;
	uint16_t command;
	int error;

	DPRINTF(1,("%s: start cmd %d arg=%08x data=%p dlen=%d flags=%08x, status=%#x\n",
	    HDEVNAME(hp), cmd->c_opcode, cmd->c_arg, cmd->c_data,
	    cmd->c_datalen, cmd->c_flags, HREAD4(hp, SDHC_NINTR_STATUS)));

	/*
	 * The maximum block length for commands should be the minimum
	 * of the host buffer size and the card buffer size. (1.7.2)
	 */

	/* Fragment the data into proper blocks. */
	if (cmd->c_datalen > 0) {
		blksize = MIN(cmd->c_datalen, cmd->c_blklen);
		blkcount = cmd->c_datalen / blksize;
		if (cmd->c_datalen % blksize > 0) {
			/* XXX: Split this command. (1.7.4) */
			aprint_error_dev(sc->sc_dev,
			    "data not a multiple of %u bytes\n", blksize);
			return EINVAL;
		}
	}

	/* Check limit imposed by 9-bit block count. (1.7.2) */
	if (blkcount > SDHC_BLOCK_COUNT_MAX) {
		aprint_error_dev(sc->sc_dev, "too much data\n");
		return EINVAL;
	}

	/* Prepare transfer mode register value. (2.2.5) */
	mode = SDHC_BLOCK_COUNT_ENABLE;
	if (ISSET(cmd->c_flags, SCF_CMD_READ))
		mode |= SDHC_READ_MODE;
	if (blkcount > 1) {
		mode |= SDHC_MULTI_BLOCK_MODE;
		/* XXX only for memory commands? */
		mode |= SDHC_AUTO_CMD12_ENABLE;
	}
	if (cmd->c_dmamap != NULL && cmd->c_datalen > 0 &&
	    !ISSET(sc->sc_flags, SDHC_FLAG_EXTERNAL_DMA)) {
		mode |= SDHC_DMA_ENABLE;
	}

	/*
	 * Prepare command register value. (2.2.6)
	 */
	command = (cmd->c_opcode & SDHC_COMMAND_INDEX_MASK) << SDHC_COMMAND_INDEX_SHIFT;

	if (ISSET(cmd->c_flags, SCF_RSP_CRC))
		command |= SDHC_CRC_CHECK_ENABLE;
	if (ISSET(cmd->c_flags, SCF_RSP_IDX))
		command |= SDHC_INDEX_CHECK_ENABLE;
	if (cmd->c_data != NULL)
		command |= SDHC_DATA_PRESENT_SELECT;

	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		command |= SDHC_NO_RESPONSE;
	else if (ISSET(cmd->c_flags, SCF_RSP_136))
		command |= SDHC_RESP_LEN_136;
	else if (ISSET(cmd->c_flags, SCF_RSP_BSY))
		command |= SDHC_RESP_LEN_48_CHK_BUSY;
	else
		command |= SDHC_RESP_LEN_48;

	/* Wait until command and data inhibit bits are clear. (1.5) */
	error = sdhc_wait_state(hp, SDHC_CMD_INHIBIT_MASK, 0);
	if (error)
		return error;

	DPRINTF(1,("%s: writing cmd: blksize=%d blkcnt=%d mode=%04x cmd=%04x\n",
	    HDEVNAME(hp), blksize, blkcount, mode, command));

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		blksize |= (MAX(0, PAGE_SHIFT - 12) & SDHC_DMA_BOUNDARY_MASK) <<
		    SDHC_DMA_BOUNDARY_SHIFT;	/* PAGE_SIZE DMA boundary */
	}

	mutex_enter(&hp->host_mtx);

	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		/* Alert the user not to remove the card. */
		HSET1(hp, SDHC_HOST_CTL, SDHC_LED_ON);
	}

	/* Set DMA start address. */
	if (ISSET(mode, SDHC_DMA_ENABLE))
		HWRITE4(hp, SDHC_DMA_ADDR, cmd->c_dmamap->dm_segs[0].ds_addr);

	/*
	 * Start a CPU data transfer.  Writing to the high order byte
	 * of the SDHC_COMMAND register triggers the SD command. (1.5)
	 */
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
		HWRITE4(hp, SDHC_BLOCK_SIZE, blksize | (blkcount << 16));
		HWRITE4(hp, SDHC_ARGUMENT, cmd->c_arg);
		HWRITE4(hp, SDHC_TRANSFER_MODE, mode | (command << 16));
	} else {
		HWRITE2(hp, SDHC_BLOCK_SIZE, blksize);
		HWRITE2(hp, SDHC_BLOCK_COUNT, blkcount);
		HWRITE4(hp, SDHC_ARGUMENT, cmd->c_arg);
		HWRITE2(hp, SDHC_TRANSFER_MODE, mode);
		HWRITE2(hp, SDHC_COMMAND, command);
	}

	mutex_exit(&hp->host_mtx);

	return 0;
}

static void
sdhc_transfer_data(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	int error;

	DPRINTF(1,("%s: data transfer: resp=%08x datalen=%u\n", HDEVNAME(hp),
	    MMC_R1(cmd->c_resp), cmd->c_datalen));

#ifdef SDHC_DEBUG
	/* XXX I forgot why I wanted to know when this happens :-( */
	if ((cmd->c_opcode == 52 || cmd->c_opcode == 53) &&
	    ISSET(MMC_R1(cmd->c_resp), 0xcb00)) {
		aprint_error_dev(hp->sc->sc_dev,
		    "CMD52/53 error response flags %#x\n",
		    MMC_R1(cmd->c_resp) & 0xff00);
	}
#endif

	if (hp->sc->sc_vendor_transfer_data_dma != NULL &&
	    cmd->c_dmamap != NULL)
		error = hp->sc->sc_vendor_transfer_data_dma(hp, cmd);
	else if (cmd->c_dmamap != NULL)
		error = sdhc_transfer_data_dma(hp, cmd);
	else
		error = sdhc_transfer_data_pio(hp, cmd);
	if (error)
		cmd->c_error = error;
	SET(cmd->c_flags, SCF_ITSDONE);

	DPRINTF(1,("%s: data transfer done (error=%d)\n",
	    HDEVNAME(hp), cmd->c_error));
}

static int
sdhc_transfer_data_dma(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	bus_dma_segment_t *dm_segs = cmd->c_dmamap->dm_segs;
	bus_addr_t posaddr;
	bus_addr_t segaddr;
	bus_size_t seglen;
	u_int seg = 0;
	int error = 0;
	int status;

	KASSERT(HREAD2(hp, SDHC_NINTR_STATUS_EN) & SDHC_DMA_INTERRUPT);
	KASSERT(HREAD2(hp, SDHC_NINTR_SIGNAL_EN) & SDHC_DMA_INTERRUPT);
	KASSERT(HREAD2(hp, SDHC_NINTR_STATUS_EN) & SDHC_TRANSFER_COMPLETE);
	KASSERT(HREAD2(hp, SDHC_NINTR_SIGNAL_EN) & SDHC_TRANSFER_COMPLETE);

	for (;;) {
		status = sdhc_wait_intr(hp,
		    SDHC_DMA_INTERRUPT|SDHC_TRANSFER_COMPLETE,
		    SDHC_DMA_TIMEOUT);

		if (status & SDHC_TRANSFER_COMPLETE) {
			break;
		}
		if (!status) {
			error = ETIMEDOUT;
			break;
		}
		if ((status & SDHC_DMA_INTERRUPT) == 0) {
			continue;
		}

		/* DMA Interrupt (boundary crossing) */

		segaddr = dm_segs[seg].ds_addr;
		seglen = dm_segs[seg].ds_len;
		mutex_enter(&hp->host_mtx);
		posaddr = HREAD4(hp, SDHC_DMA_ADDR);
		mutex_exit(&hp->host_mtx);

		if ((seg == (cmd->c_dmamap->dm_nsegs-1)) && (posaddr == (segaddr + seglen))) {
			continue;
		}
		mutex_enter(&hp->host_mtx);
		if ((posaddr >= segaddr) && (posaddr < (segaddr + seglen)))
			HWRITE4(hp, SDHC_DMA_ADDR, posaddr);
		else if ((posaddr >= segaddr) && (posaddr == (segaddr + seglen)) && (seg + 1) < cmd->c_dmamap->dm_nsegs)
			HWRITE4(hp, SDHC_DMA_ADDR, dm_segs[++seg].ds_addr);
		mutex_exit(&hp->host_mtx);
		KASSERT(seg < cmd->c_dmamap->dm_nsegs);
	}

	return error;
}

static int
sdhc_transfer_data_pio(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	uint8_t *data = cmd->c_data;
	void (*pio_func)(struct sdhc_host *, uint8_t *, u_int);
	u_int len, datalen;
	u_int imask;
	u_int pmask;
	int error = 0;

	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		imask = SDHC_BUFFER_READ_READY;
		pmask = SDHC_BUFFER_READ_ENABLE;
		if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
			pio_func = esdhc_read_data_pio;
		} else {
			pio_func = sdhc_read_data_pio;
		}
	} else {
		imask = SDHC_BUFFER_WRITE_READY;
		pmask = SDHC_BUFFER_WRITE_ENABLE;
		if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
			pio_func = esdhc_write_data_pio;
		} else {
			pio_func = sdhc_write_data_pio;
		}
	}
	datalen = cmd->c_datalen;

	KASSERT(HREAD2(hp, SDHC_NINTR_STATUS_EN) & imask);
	KASSERT(HREAD2(hp, SDHC_NINTR_STATUS_EN) & SDHC_TRANSFER_COMPLETE);
	KASSERT(HREAD2(hp, SDHC_NINTR_SIGNAL_EN) & SDHC_TRANSFER_COMPLETE);

	while (datalen > 0) {
		if (!ISSET(HREAD4(hp, SDHC_PRESENT_STATE), imask)) {
			mutex_enter(&hp->intr_mtx);
			if (ISSET(hp->sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
				HSET4(hp, SDHC_NINTR_SIGNAL_EN, imask);
			} else {
				HSET2(hp, SDHC_NINTR_SIGNAL_EN, imask);
			}
			mutex_exit(&hp->intr_mtx);
			if (!sdhc_wait_intr(hp, imask, SDHC_BUFFER_TIMEOUT)) {
				error = ETIMEDOUT;
				break;
			}

			error = sdhc_wait_state(hp, pmask, pmask);
			if (error)
				break;
		}

		len = MIN(datalen, cmd->c_blklen);
		(*pio_func)(hp, data, len);
		DPRINTF(2,("%s: pio data transfer %u @ %p\n",
		    HDEVNAME(hp), len, data));

		data += len;
		datalen -= len;
	}

	if (error == 0 && !sdhc_wait_intr(hp, SDHC_TRANSFER_COMPLETE,
	    SDHC_TRANSFER_TIMEOUT))
		error = ETIMEDOUT;

	return error;
}

static void
sdhc_read_data_pio(struct sdhc_host *hp, uint8_t *data, u_int datalen)
{

	if (((__uintptr_t)data & 3) == 0) {
		while (datalen > 3) {
			*(uint32_t *)data = le32toh(HREAD4(hp, SDHC_DATA));
			data += 4;
			datalen -= 4;
		}
		if (datalen > 1) {
			*(uint16_t *)data = le16toh(HREAD2(hp, SDHC_DATA));
			data += 2;
			datalen -= 2;
		}
		if (datalen > 0) {
			*data = HREAD1(hp, SDHC_DATA);
			data += 1;
			datalen -= 1;
		}
	} else if (((__uintptr_t)data & 1) == 0) {
		while (datalen > 1) {
			*(uint16_t *)data = le16toh(HREAD2(hp, SDHC_DATA));
			data += 2;
			datalen -= 2;
		}
		if (datalen > 0) {
			*data = HREAD1(hp, SDHC_DATA);
			data += 1;
			datalen -= 1;
		}
	} else {
		while (datalen > 0) {
			*data = HREAD1(hp, SDHC_DATA);
			data += 1;
			datalen -= 1;
		}
	}
}

static void
sdhc_write_data_pio(struct sdhc_host *hp, uint8_t *data, u_int datalen)
{

	if (((__uintptr_t)data & 3) == 0) {
		while (datalen > 3) {
			HWRITE4(hp, SDHC_DATA, htole32(*(uint32_t *)data));
			data += 4;
			datalen -= 4;
		}
		if (datalen > 1) {
			HWRITE2(hp, SDHC_DATA, htole16(*(uint16_t *)data));
			data += 2;
			datalen -= 2;
		}
		if (datalen > 0) {
			HWRITE1(hp, SDHC_DATA, *data);
			data += 1;
			datalen -= 1;
		}
	} else if (((__uintptr_t)data & 1) == 0) {
		while (datalen > 1) {
			HWRITE2(hp, SDHC_DATA, htole16(*(uint16_t *)data));
			data += 2;
			datalen -= 2;
		}
		if (datalen > 0) {
			HWRITE1(hp, SDHC_DATA, *data);
			data += 1;
			datalen -= 1;
		}
	} else {
		while (datalen > 0) {
			HWRITE1(hp, SDHC_DATA, *data);
			data += 1;
			datalen -= 1;
		}
	}
}

static void
esdhc_read_data_pio(struct sdhc_host *hp, uint8_t *data, u_int datalen)
{
	uint16_t status = HREAD2(hp, SDHC_NINTR_STATUS);
	uint32_t v;

	const size_t watermark = (HREAD4(hp, SDHC_WATERMARK_LEVEL) >> SDHC_WATERMARK_READ_SHIFT) & SDHC_WATERMARK_READ_MASK;
	size_t count = 0;

	while (datalen > 3 && !ISSET(status, SDHC_TRANSFER_COMPLETE)) {
		if (count == 0) {
			/*
			 * If we've drained "watermark" words, we need to wait
			 * a little bit so the read FIFO can refill.
			 */
			sdmmc_delay(10);
			count = watermark;
		}
		v = HREAD4(hp, SDHC_DATA);
		v = le32toh(v);
		*(uint32_t *)data = v;
		data += 4;
		datalen -= 4;
		status = HREAD2(hp, SDHC_NINTR_STATUS);
		count--;
	}
	if (datalen > 0 && !ISSET(status, SDHC_TRANSFER_COMPLETE)) {
		if (count == 0) {
			sdmmc_delay(10);
		}
		v = HREAD4(hp, SDHC_DATA);
		v = le32toh(v);
		do {
			*data++ = v;
			v >>= 8;
		} while (--datalen > 0);
	}
}

static void
esdhc_write_data_pio(struct sdhc_host *hp, uint8_t *data, u_int datalen)
{
	uint16_t status = HREAD2(hp, SDHC_NINTR_STATUS);
	uint32_t v;

	const size_t watermark = (HREAD4(hp, SDHC_WATERMARK_LEVEL) >> SDHC_WATERMARK_WRITE_SHIFT) & SDHC_WATERMARK_WRITE_MASK;
	size_t count = watermark;

	while (datalen > 3 && !ISSET(status, SDHC_TRANSFER_COMPLETE)) {
		if (count == 0) {
			sdmmc_delay(10);
			count = watermark;
		}
		v = *(uint32_t *)data;
		v = htole32(v);
		HWRITE4(hp, SDHC_DATA, v);
		data += 4;
		datalen -= 4;
		status = HREAD2(hp, SDHC_NINTR_STATUS);
		count--;
	}
	if (datalen > 0 && !ISSET(status, SDHC_TRANSFER_COMPLETE)) {
		if (count == 0) {
			sdmmc_delay(10);
		}
		v = *(uint32_t *)data;
		v = htole32(v);
		HWRITE4(hp, SDHC_DATA, v);
	}
}

/* Prepare for another command. */
static int
sdhc_soft_reset(struct sdhc_host *hp, int mask)
{
	int timo;

	DPRINTF(1,("%s: software reset reg=%08x\n", HDEVNAME(hp), mask));

	/* Request the reset.  */
	HWRITE1(hp, SDHC_SOFTWARE_RESET, mask);

	/*
	 * If necessary, wait for the controller to set the bits to
	 * acknowledge the reset.
	 */
	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_WAIT_RESET) &&
	    ISSET(mask, (SDHC_RESET_DAT | SDHC_RESET_CMD))) {
		for (timo = 10000; timo > 0; timo--) {
			if (ISSET(HREAD1(hp, SDHC_SOFTWARE_RESET), mask))
				break;
			/* Short delay because I worry we may miss it...  */
			sdmmc_delay(1);
		}
		if (timo == 0)
			return ETIMEDOUT;
	}

	/*
	 * Wait for the controller to clear the bits to indicate that
	 * the reset has completed.
	 */
	for (timo = 10; timo > 0; timo--) {
		if (!ISSET(HREAD1(hp, SDHC_SOFTWARE_RESET), mask))
			break;
		sdmmc_delay(10000);
	}
	if (timo == 0) {
		DPRINTF(1,("%s: timeout reg=%08x\n", HDEVNAME(hp),
		    HREAD1(hp, SDHC_SOFTWARE_RESET)));
		return ETIMEDOUT;
	}

	if (ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		HWRITE4(hp, SDHC_DMA_CTL, SDHC_DMA_SNOOP);
	}

	return 0;
}

static int
sdhc_wait_intr(struct sdhc_host *hp, int mask, int timo)
{
	int status;

	mask |= SDHC_ERROR_INTERRUPT;

	mutex_enter(&hp->intr_mtx);
	status = hp->intr_status & mask;
	while (status == 0) {
		if (cv_timedwait(&hp->intr_cv, &hp->intr_mtx, timo)
		    == EWOULDBLOCK) {
			status |= SDHC_ERROR_INTERRUPT;
			break;
		}
		status = hp->intr_status & mask;
	}
	hp->intr_status &= ~status;

	DPRINTF(2,("%s: intr status %#x error %#x\n", HDEVNAME(hp), status,
	    hp->intr_error_status));
	
	/* Command timeout has higher priority than command complete. */
	if (ISSET(status, SDHC_ERROR_INTERRUPT) || hp->intr_error_status) {
		hp->intr_error_status = 0;
		hp->intr_status &= ~SDHC_ERROR_INTERRUPT;
		if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED)) {
		    (void)sdhc_soft_reset(hp, SDHC_RESET_DAT|SDHC_RESET_CMD);
		}
		status = 0;
	}
	mutex_exit(&hp->intr_mtx);

	return status;
}

/*
 * Established by attachment driver at interrupt priority IPL_SDMMC.
 */
int
sdhc_intr(void *arg)
{
	struct sdhc_softc *sc = (struct sdhc_softc *)arg;
	struct sdhc_host *hp;
	int done = 0;
	uint16_t status;
	uint16_t error;

	/* We got an interrupt, but we don't know from which slot. */
	for (size_t host = 0; host < sc->sc_nhosts; host++) {
		hp = sc->sc_host[host];
		if (hp == NULL)
			continue;

		if (ISSET(sc->sc_flags, SDHC_FLAG_32BIT_ACCESS)) {
			/* Find out which interrupts are pending. */
			uint32_t xstatus = HREAD4(hp, SDHC_NINTR_STATUS);
			status = xstatus;
			error = xstatus >> 16;
			if (error)
				xstatus |= SDHC_ERROR_INTERRUPT;
			else if (!ISSET(status, SDHC_NINTR_STATUS_MASK))
				continue; /* no interrupt for us */
			/* Acknowledge the interrupts we are about to handle. */
			HWRITE4(hp, SDHC_NINTR_STATUS, xstatus);
		} else {
			/* Find out which interrupts are pending. */
			error = 0;
			status = HREAD2(hp, SDHC_NINTR_STATUS);
			if (!ISSET(status, SDHC_NINTR_STATUS_MASK))
				continue; /* no interrupt for us */
			/* Acknowledge the interrupts we are about to handle. */
			HWRITE2(hp, SDHC_NINTR_STATUS, status);
			if (ISSET(status, SDHC_ERROR_INTERRUPT)) {
				/* Acknowledge error interrupts. */
				error = HREAD2(hp, SDHC_EINTR_STATUS);
				HWRITE2(hp, SDHC_EINTR_STATUS, error);
			}
		}
	
		DPRINTF(2,("%s: interrupt status=%x error=%x\n", HDEVNAME(hp),
		    status, error));

		mutex_enter(&hp->intr_mtx);

		/* Claim this interrupt. */
		done = 1;

		/*
		 * Service error interrupts.
		 */
		if (ISSET(error, SDHC_CMD_TIMEOUT_ERROR|
		    SDHC_DATA_TIMEOUT_ERROR)) {
			hp->intr_error_status |= error;
			hp->intr_status |= status;
			cv_broadcast(&hp->intr_cv);
		}

		/*
		 * Wake up the sdmmc event thread to scan for cards.
		 */
		if (ISSET(status, SDHC_CARD_REMOVAL|SDHC_CARD_INSERTION)) {
			if (hp->sdmmc != NULL) {
				sdmmc_needs_discover(hp->sdmmc);
			}
			if (ISSET(sc->sc_flags, SDHC_FLAG_ENHANCED)) {
				HCLR4(hp, SDHC_NINTR_STATUS_EN,
				    status & (SDHC_CARD_REMOVAL|SDHC_CARD_INSERTION));
				HCLR4(hp, SDHC_NINTR_SIGNAL_EN,
				    status & (SDHC_CARD_REMOVAL|SDHC_CARD_INSERTION));
			}
		}

		/*
		 * Wake up the blocking process to service command
		 * related interrupt(s).
		 */
		if (ISSET(status, SDHC_COMMAND_COMPLETE|
		    SDHC_BUFFER_READ_READY|SDHC_BUFFER_WRITE_READY|
		    SDHC_TRANSFER_COMPLETE|SDHC_DMA_INTERRUPT)) {
			hp->intr_status |= status;
			if (ISSET(sc->sc_flags, SDHC_FLAG_ENHANCED)) {
				HCLR4(hp, SDHC_NINTR_SIGNAL_EN,
				    status & (SDHC_BUFFER_READ_READY|SDHC_BUFFER_WRITE_READY));
			}
			cv_broadcast(&hp->intr_cv);
		}

		/*
		 * Service SD card interrupts.
		 */
		if (!ISSET(sc->sc_flags, SDHC_FLAG_ENHANCED)
		    && ISSET(status, SDHC_CARD_INTERRUPT)) {
			DPRINTF(0,("%s: card interrupt\n", HDEVNAME(hp)));
			HCLR2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
			sdmmc_card_intr(hp->sdmmc);
		}
		mutex_exit(&hp->intr_mtx);
	}

	return done;
}

#ifdef SDHC_DEBUG
void
sdhc_dump_regs(struct sdhc_host *hp)
{

	printf("0x%02x PRESENT_STATE:    %x\n", SDHC_PRESENT_STATE,
	    HREAD4(hp, SDHC_PRESENT_STATE));
	if (!ISSET(hp->sc->sc_flags, SDHC_FLAG_ENHANCED))
		printf("0x%02x POWER_CTL:        %x\n", SDHC_POWER_CTL,
		    HREAD1(hp, SDHC_POWER_CTL));
	printf("0x%02x NINTR_STATUS:     %x\n", SDHC_NINTR_STATUS,
	    HREAD2(hp, SDHC_NINTR_STATUS));
	printf("0x%02x EINTR_STATUS:     %x\n", SDHC_EINTR_STATUS,
	    HREAD2(hp, SDHC_EINTR_STATUS));
	printf("0x%02x NINTR_STATUS_EN:  %x\n", SDHC_NINTR_STATUS_EN,
	    HREAD2(hp, SDHC_NINTR_STATUS_EN));
	printf("0x%02x EINTR_STATUS_EN:  %x\n", SDHC_EINTR_STATUS_EN,
	    HREAD2(hp, SDHC_EINTR_STATUS_EN));
	printf("0x%02x NINTR_SIGNAL_EN:  %x\n", SDHC_NINTR_SIGNAL_EN,
	    HREAD2(hp, SDHC_NINTR_SIGNAL_EN));
	printf("0x%02x EINTR_SIGNAL_EN:  %x\n", SDHC_EINTR_SIGNAL_EN,
	    HREAD2(hp, SDHC_EINTR_SIGNAL_EN));
	printf("0x%02x CAPABILITIES:     %x\n", SDHC_CAPABILITIES,
	    HREAD4(hp, SDHC_CAPABILITIES));
	printf("0x%02x MAX_CAPABILITIES: %x\n", SDHC_MAX_CAPABILITIES,
	    HREAD4(hp, SDHC_MAX_CAPABILITIES));
}
#endif
