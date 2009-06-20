/*	$NetBSD: sdhc.c,v 1.1.4.3 2009/06/20 07:20:29 yamt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: sdhc.c,v 1.1.4.3 2009/06/20 07:20:29 yamt Exp $");

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

	uint32_t flags;			/* flags for this host */
#define SHF_USE_DMA		0x0001
#define SHF_USE_4BIT_MODE	0x0002
};

#define HDEVNAME(hp)	(device_xname((hp)->sc->sc_dev))

#define HREAD1(hp, reg)							\
	(bus_space_read_1((hp)->iot, (hp)->ioh, (reg)))
#define HREAD2(hp, reg)							\
	(bus_space_read_2((hp)->iot, (hp)->ioh, (reg)))
#define HREAD4(hp, reg)							\
	(bus_space_read_4((hp)->iot, (hp)->ioh, (reg)))
#define HWRITE1(hp, reg, val)						\
	bus_space_write_1((hp)->iot, (hp)->ioh, (reg), (val))
#define HWRITE2(hp, reg, val)						\
	bus_space_write_2((hp)->iot, (hp)->ioh, (reg), (val))
#define HWRITE4(hp, reg, val)						\
	bus_space_write_4((hp)->iot, (hp)->ioh, (reg), (val))
#define HCLR1(hp, reg, bits)						\
	HWRITE1((hp), (reg), HREAD1((hp), (reg)) & ~(bits))
#define HCLR2(hp, reg, bits)						\
	HWRITE2((hp), (reg), HREAD2((hp), (reg)) & ~(bits))
#define HSET1(hp, reg, bits)						\
	HWRITE1((hp), (reg), HREAD1((hp), (reg)) | (bits))
#define HSET2(hp, reg, bits)						\
	HWRITE2((hp), (reg), HREAD2((hp), (reg)) | (bits))

static int	sdhc_host_reset(sdmmc_chipset_handle_t);
static int	sdhc_host_reset1(sdmmc_chipset_handle_t);
static uint32_t	sdhc_host_ocr(sdmmc_chipset_handle_t);
static int	sdhc_host_maxblklen(sdmmc_chipset_handle_t);
static int	sdhc_card_detect(sdmmc_chipset_handle_t);
static int	sdhc_write_protect(sdmmc_chipset_handle_t);
static int	sdhc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	sdhc_bus_clock(sdmmc_chipset_handle_t, int);
static int	sdhc_bus_width(sdmmc_chipset_handle_t, int);
static void	sdhc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	sdhc_card_intr_ack(sdmmc_chipset_handle_t);
static void	sdhc_exec_command(sdmmc_chipset_handle_t,
		    struct sdmmc_command *);
static int	sdhc_start_command(struct sdhc_host *, struct sdmmc_command *);
static int	sdhc_wait_state(struct sdhc_host *, uint32_t, uint32_t);
static int	sdhc_soft_reset(struct sdhc_host *, int);
static int	sdhc_wait_intr(struct sdhc_host *, int, int);
static void	sdhc_transfer_data(struct sdhc_host *, struct sdmmc_command *);
static int	sdhc_transfer_data_pio(struct sdhc_host *, struct sdmmc_command *);
static void	sdhc_read_data_pio(struct sdhc_host *, uint8_t *, int);
static void	sdhc_write_data_pio(struct sdhc_host *, uint8_t *, int);

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

	/* command execution */
	sdhc_exec_command,

	/* card interrupt */
	sdhc_card_enable_intr,
	sdhc_card_intr_ack
};

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
#ifdef SDHC_DEBUG
	uint16_t sdhcver;

	sdhcver = bus_space_read_2(iot, ioh, SDHC_HOST_CTL_VERSION);
	aprint_normal_dev(sc->sc_dev, "SD Host Specification/Vendor Version ");
	switch (SDHC_SPEC_VERSION(sdhcver)) {
	case 0x00:
		aprint_normal("1.0/%u\n", SDHC_VENDOR_VERSION(sdhcver));
		break;

	default:
		aprint_normal(">1.0/%u\n", SDHC_VENDOR_VERSION(sdhcver));
		break;
	}
#endif

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
	hp->dmat = sc->sc_dmat;

	mutex_init(&hp->host_mtx, MUTEX_DEFAULT, IPL_SDMMC);
	mutex_init(&hp->intr_mtx, MUTEX_DEFAULT, IPL_SDMMC);
	cv_init(&hp->intr_cv, "sdhcintr");

	/*
	 * eset the host controller and enable interrupts.
	 */
	(void)sdhc_host_reset(hp);

	/* Determine host capabilities. */
	mutex_enter(&hp->host_mtx);
	caps = HREAD4(hp, SDHC_CAPABILITIES);
	mutex_exit(&hp->host_mtx);

#if notyet
	/* Use DMA if the host system and the controller support it. */
	if (ISSET(sc->sc_flags, SDHC_FLAG_FORCE_DMA)
	 || ((ISSET(sc->sc_flags, SDHC_FLAG_USE_DMA)
	   && ISSET(caps, SDHC_DMA_SUPPORT)))) {
		SET(hp->flags, SHF_USE_DMA);
		aprint_normal_dev(sc->sc_dev, "using DMA transfer\n");
	}
#endif

	/*
	 * Determine the base clock frequency. (2.2.24)
	 */
	if (SDHC_BASE_FREQ_KHZ(caps) != 0)
		hp->clkbase = SDHC_BASE_FREQ_KHZ(caps);
	if (hp->clkbase == 0) {
		/* The attachment driver must tell us. */
		aprint_error_dev(sc->sc_dev,"unknown base clock frequency\n");
		goto err;
	} else if (hp->clkbase < 10000 || hp->clkbase > 63000) {
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

	/*
	 * Determine SD bus voltage levels supported by the controller.
	 */
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_1_8V))
		SET(hp->ocr, MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V);
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_3_0V))
		SET(hp->ocr, MMC_OCR_2_9V_3_0V | MMC_OCR_3_0V_3_1V);
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_3_3V))
		SET(hp->ocr, MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V);

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
	saa.saa_clkmin = hp->clkbase / 256;
	saa.saa_clkmax = hp->clkbase;
	saa.saa_caps = SMC_CAPS_4BIT_MODE|SMC_CAPS_AUTO_STOP;
#if notyet
	if (ISSET(hp->flags, SHF_USE_DMA))
		saa.saa_caps |= SMC_CAPS_DMA;
#endif

	hp->sdmmc = config_found(sc->sc_dev, &saa, NULL);

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

bool
sdhc_suspend(device_t dev PMF_FN_ARGS)
{
	struct sdhc_softc *sc = device_private(dev);
	struct sdhc_host *hp;
	int n, i;

	/* XXX poll for command completion or suspend command
	 * in progress */

	/* Save the host controller state. */
	for (n = 0; n < sc->sc_nhosts; n++) {
		hp = sc->sc_host[n];
		for (i = 0; i < sizeof hp->regs; i++)
			hp->regs[i] = HREAD1(hp, i);
	}
	return true;
}

bool
sdhc_resume(device_t dev PMF_FN_ARGS)
{
	struct sdhc_softc *sc = device_private(dev);
	struct sdhc_host *hp;
	int n, i;

	/* Restore the host controller state. */
	for (n = 0; n < sc->sc_nhosts; n++) {
		hp = sc->sc_host[n];
		(void)sdhc_host_reset(hp);
		for (i = 0; i < sizeof hp->regs; i++)
			HWRITE1(hp, i, hp->regs[i]);
	}
	return true;
}

bool
sdhc_shutdown(device_t dev, int flags)
{
	struct sdhc_softc *sc = device_private(dev);
	struct sdhc_host *hp;
	int i;

	/* XXX chip locks up if we don't disable it before reboot. */
	for (i = 0; i < sc->sc_nhosts; i++) {
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
	uint16_t sdhcimask;
	int error;

	/* Don't lock. */

	/* Disable all interrupts. */
	HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, 0);

	/*
	 * Reset the entire host controller and wait up to 100ms for
	 * the controller to clear the reset bit.
	 */
	error = sdhc_soft_reset(hp, SDHC_RESET_ALL);
	if (error)
		goto out;

	/* Set data timeout counter value to max for now. */
	HWRITE1(hp, SDHC_TIMEOUT_CTL, SDHC_TIMEOUT_MAX);

	/* Enable interrupts. */
	sdhcimask = SDHC_CARD_REMOVAL | SDHC_CARD_INSERTION |
	    SDHC_BUFFER_READ_READY | SDHC_BUFFER_WRITE_READY |
	    SDHC_DMA_INTERRUPT | SDHC_BLOCK_GAP_EVENT |
	    SDHC_TRANSFER_COMPLETE | SDHC_COMMAND_COMPLETE;
	HWRITE2(hp, SDHC_NINTR_STATUS_EN, sdhcimask);
	HWRITE2(hp, SDHC_EINTR_STATUS_EN, SDHC_EINTR_STATUS_MASK);
	HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, sdhcimask);
	HWRITE2(hp, SDHC_EINTR_SIGNAL_EN, SDHC_EINTR_SIGNAL_MASK);

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

	mutex_enter(&hp->host_mtx);
	r = ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_CARD_INSERTED);
	mutex_exit(&hp->host_mtx);

	if (r)
		return 1;
	return 0;
}

/*
 * Return non-zero if the card is currently write-protected.
 */
static int
sdhc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int r;

	mutex_enter(&hp->host_mtx);
	r = ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_WRITE_PROTECT_SWITCH);
	mutex_exit(&hp->host_mtx);

	if (!r)
		return 1;
	return 0;
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

	mutex_enter(&hp->host_mtx);

	/*
	 * Disable bus power before voltage change.
	 */
	if (!(hp->sc->sc_flags & SDHC_FLAG_NO_PWR0))
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
	if (ISSET(ocr, MMC_OCR_1_7V_1_8V|MMC_OCR_1_8V_1_9V))
		vdd = SDHC_VOLTAGE_1_8V;
	else if (ISSET(ocr, MMC_OCR_2_9V_3_0V|MMC_OCR_3_0V_3_1V))
		vdd = SDHC_VOLTAGE_3_0V;
	else if (ISSET(ocr, MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V))
		vdd = SDHC_VOLTAGE_3_3V;
	else {
		/* Unsupported voltage level requested. */
		error = EINVAL;
		goto out;
	}

	/*
	 * Enable bus power.  Wait at least 1 ms (or 74 clocks) plus
	 * voltage ramp until power rises.
	 */
	HWRITE1(hp, SDHC_POWER_CTL,
	    (vdd << SDHC_VOLTAGE_SHIFT) | SDHC_BUS_POWER);
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

out:
	mutex_exit(&hp->host_mtx);

	return error;
}

/*
 * Return the smallest possible base clock frequency divisor value
 * for the CLOCK_CTL register to produce `freq' (KHz).
 */
static int
sdhc_clock_divisor(struct sdhc_host *hp, u_int freq)
{
	int div;

	for (div = 1; div <= 256; div *= 2)
		if ((hp->clkbase / div) <= freq)
			return (div / 2);
	/* No divisor found. */
	return -1;
}

/*
 * Set or change SDCLK frequency or disable the SD clock.
 * Return zero on success.
 */
static int
sdhc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int div;
	int timo;
	int error = 0;
#ifdef DIAGNOSTIC
	int ispresent;
#endif

#ifdef DIAGNOSTIC
	mutex_enter(&hp->host_mtx);
	ispresent = ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_CMD_INHIBIT_MASK);
	mutex_exit(&hp->host_mtx);

	/* Must not stop the clock if commands are in progress. */
	if (ispresent && sdhc_card_detect(hp))
		printf("%s: sdhc_sdclk_frequency_select: command in progress\n",
		    device_xname(hp->sc->sc_dev));
#endif

	mutex_enter(&hp->host_mtx);

	/*
	 * Stop SD clock before changing the frequency.
	 */
	HWRITE2(hp, SDHC_CLOCK_CTL, 0);
	if (freq == SDMMC_SDCLK_OFF)
		goto out;

	/*
	 * Set the minimum base clock frequency divisor.
	 */
	if ((div = sdhc_clock_divisor(hp, freq)) < 0) {
		/* Invalid base clock frequency or `freq' value. */
		error = EINVAL;
		goto out;
	}
	HWRITE2(hp, SDHC_CLOCK_CTL, div << SDHC_SDCLK_DIV_SHIFT);

	/*
	 * Start internal clock.  Wait 10ms for stabilization.
	 */
	HSET2(hp, SDHC_CLOCK_CTL, SDHC_INTCLK_ENABLE);
	for (timo = 1000; timo > 0; timo--) {
		if (ISSET(HREAD2(hp, SDHC_CLOCK_CTL), SDHC_INTCLK_STABLE))
			break;
		sdmmc_delay(10);
	}
	if (timo == 0) {
		error = ETIMEDOUT;
		goto out;
	}

	/*
	 * Enable SD clock.
	 */
	HSET2(hp, SDHC_CLOCK_CTL, SDHC_SDCLK_ENABLE);

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

	default:
		DPRINTF(0,("%s: unsupported bus width (%d)\n",
		    HDEVNAME(hp), width));
		return 1;
	}

	mutex_enter(&hp->host_mtx);
	reg = HREAD1(hp, SDHC_POWER_CTL);
	reg &= ~SDHC_4BIT_MODE;
	if (width == 4)
		reg |= SDHC_4BIT_MODE;
	HWRITE1(hp, SDHC_POWER_CTL, reg);
	mutex_exit(&hp->host_mtx);

	return 0;
}

static void
sdhc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	mutex_enter(&hp->host_mtx);
	if (enable) {
		HSET2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
		HSET2(hp, SDHC_NINTR_SIGNAL_EN, SDHC_CARD_INTERRUPT);
	} else {
		HCLR2(hp, SDHC_NINTR_SIGNAL_EN, SDHC_CARD_INTERRUPT);
		HCLR2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
	}
	mutex_exit(&hp->host_mtx);
}

static void 
sdhc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	mutex_enter(&hp->host_mtx);
	HSET2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
	mutex_exit(&hp->host_mtx);
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
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			uint8_t *p = (uint8_t *)cmd->c_resp;
			int i;

			for (i = 0; i < 15; i++)
				*p++ = HREAD1(hp, SDHC_RESPONSE + i);
		} else {
			cmd->c_resp[0] = HREAD4(hp, SDHC_RESPONSE);
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

out:
	mutex_enter(&hp->host_mtx);
	/* Turn off the LED. */
	HCLR1(hp, SDHC_HOST_CTL, SDHC_LED_ON);
	mutex_exit(&hp->host_mtx);
	SET(cmd->c_flags, SCF_ITSDONE);

	DPRINTF(1,("%s: cmd %d %s (flags=%08x error=%d)\n", HDEVNAME(hp),
	    cmd->c_opcode, (cmd->c_error == 0) ? "done" : "abort",
	    cmd->c_flags, cmd->c_error));
}

static int
sdhc_start_command(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	uint16_t blksize = 0;
	uint16_t blkcount = 0;
	uint16_t mode;
	uint16_t command;
	int error;

	DPRINTF(1,("%s: start cmd %d arg=%08x data=%p dlen=%d flags=%08x "
	    "proc=%p \"%s\"\n", HDEVNAME(hp), cmd->c_opcode, cmd->c_arg,
	    cmd->c_data, cmd->c_datalen, cmd->c_flags, curproc,
	    curproc ? curproc->p_comm : ""));

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
			aprint_error_dev(hp->sc->sc_dev,
			    "data not a multiple of %u bytes\n", blksize);
			return EINVAL;
		}
	}

	/* Check limit imposed by 9-bit block count. (1.7.2) */
	if (blkcount > SDHC_BLOCK_COUNT_MAX) {
		aprint_error_dev(hp->sc->sc_dev, "too much data\n");
		return EINVAL;
	}

	/* Prepare transfer mode register value. (2.2.5) */
	mode = 0;
	if (ISSET(cmd->c_flags, SCF_CMD_READ))
		mode |= SDHC_READ_MODE;
	if (blkcount > 0) {
		mode |= SDHC_BLOCK_COUNT_ENABLE;
		if (blkcount > 1) {
			mode |= SDHC_MULTI_BLOCK_MODE;
			/* XXX only for memory commands? */
			mode |= SDHC_AUTO_CMD12_ENABLE;
		}
	}
#if notyet
	if (cmd->c_dmap != NULL && cmd->c_datalen > 0)
		mode |= SDHC_DMA_ENABLE;
#endif

	/*
	 * Prepare command register value. (2.2.6)
	 */
	command =
	 (cmd->c_opcode & SDHC_COMMAND_INDEX_MASK) << SDHC_COMMAND_INDEX_SHIFT;

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

	mutex_enter(&hp->host_mtx);

	/* Alert the user not to remove the card. */
	HSET1(hp, SDHC_HOST_CTL, SDHC_LED_ON);

	/*
	 * Start a CPU data transfer.  Writing to the high order byte
	 * of the SDHC_COMMAND register triggers the SD command. (1.5)
	 */
	HWRITE2(hp, SDHC_TRANSFER_MODE, mode);
	HWRITE2(hp, SDHC_BLOCK_SIZE, blksize);
	if (blkcount > 1)
		HWRITE2(hp, SDHC_BLOCK_COUNT, blkcount);
	HWRITE4(hp, SDHC_ARGUMENT, cmd->c_arg);
	HWRITE2(hp, SDHC_COMMAND, command);

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

	error = sdhc_transfer_data_pio(hp, cmd);
	if (error)
		cmd->c_error = error;
	SET(cmd->c_flags, SCF_ITSDONE);

	DPRINTF(1,("%s: data transfer done (error=%d)\n",
	    HDEVNAME(hp), cmd->c_error));
}

static int
sdhc_transfer_data_pio(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	uint8_t *data = cmd->c_data;
	int len, datalen;
	int mask;
	int error = 0;

	mask = ISSET(cmd->c_flags, SCF_CMD_READ) ?
	    SDHC_BUFFER_READ_ENABLE : SDHC_BUFFER_WRITE_ENABLE;
	datalen = cmd->c_datalen;

	while (datalen > 0) {
		if (!sdhc_wait_intr(hp,
		    SDHC_BUFFER_READ_READY|SDHC_BUFFER_WRITE_READY,
		    SDHC_BUFFER_TIMEOUT)) {
			error = ETIMEDOUT;
			break;
		}

		error = sdhc_wait_state(hp, mask, mask);
		if (error)
			break;

		len = MIN(datalen, cmd->c_blklen);
		if (ISSET(cmd->c_flags, SCF_CMD_READ))
			sdhc_read_data_pio(hp, data, len);
		else
			sdhc_write_data_pio(hp, data, len);

		data += len;
		datalen -= len;
	}

	if (error == 0 && !sdhc_wait_intr(hp, SDHC_TRANSFER_COMPLETE,
	    SDHC_TRANSFER_TIMEOUT))
		error = ETIMEDOUT;

	return error;
}

static void
sdhc_read_data_pio(struct sdhc_host *hp, uint8_t *data, int datalen)
{

	if (((__uintptr_t)data & 3) == 0) {
		while (datalen > 3) {
			*(uint32_t *)data = HREAD4(hp, SDHC_DATA);
			data += 4;
			datalen -= 4;
		}
		if (datalen > 1) {
			*(uint16_t *)data = HREAD2(hp, SDHC_DATA);
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
			*(uint16_t *)data = HREAD2(hp, SDHC_DATA);
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
sdhc_write_data_pio(struct sdhc_host *hp, uint8_t *data, int datalen)
{

	if (((__uintptr_t)data & 3) == 0) {
		while (datalen > 3) {
			HWRITE4(hp, SDHC_DATA, *(uint32_t *)data);
			data += 4;
			datalen -= 4;
		}
		if (datalen > 1) {
			HWRITE2(hp, SDHC_DATA, *(uint16_t *)data);
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
			HWRITE2(hp, SDHC_DATA, *(uint16_t *)data);
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

/* Prepare for another command. */
static int
sdhc_soft_reset(struct sdhc_host *hp, int mask)
{
	int timo;

	DPRINTF(1,("%s: software reset reg=%08x\n", HDEVNAME(hp), mask));

	HWRITE1(hp, SDHC_SOFTWARE_RESET, mask);
	for (timo = 10; timo > 0; timo--) {
		if (!ISSET(HREAD1(hp, SDHC_SOFTWARE_RESET), mask))
			break;
		sdmmc_delay(10000);
		HWRITE1(hp, SDHC_SOFTWARE_RESET, 0);
	}
	if (timo == 0) {
		DPRINTF(1,("%s: timeout reg=%08x\n", HDEVNAME(hp),
		    HREAD1(hp, SDHC_SOFTWARE_RESET)));
		HWRITE1(hp, SDHC_SOFTWARE_RESET, 0);
		return ETIMEDOUT;
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
	if (ISSET(status, SDHC_ERROR_INTERRUPT)) {
		hp->intr_error_status = 0;
		(void)sdhc_soft_reset(hp, SDHC_RESET_DAT|SDHC_RESET_CMD);
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
	int host;
	int done = 0;
	uint16_t status;
	uint16_t error;

	/* We got an interrupt, but we don't know from which slot. */
	for (host = 0; host < sc->sc_nhosts; host++) {
		hp = sc->sc_host[host];
		if (hp == NULL)
			continue;

		/* Find out which interrupts are pending. */
		status = HREAD2(hp, SDHC_NINTR_STATUS);
		if (!ISSET(status, SDHC_NINTR_STATUS_MASK))
			continue; /* no interrupt for us */

		/* Acknowledge the interrupts we are about to handle. */
		HWRITE2(hp, SDHC_NINTR_STATUS, status);
		DPRINTF(2,("%s: interrupt status=%x\n", HDEVNAME(hp),
		    status));

		if (!ISSET(status, SDHC_NINTR_STATUS_MASK))
			continue;

		/* Claim this interrupt. */
		done = 1;

		/*
		 * Service error interrupts.
		 */
		if (ISSET(status, SDHC_ERROR_INTERRUPT)) {
			/* Acknowledge error interrupts. */
			error = HREAD2(hp, SDHC_EINTR_STATUS);
			HWRITE2(hp, SDHC_EINTR_STATUS, error);
			DPRINTF(2,("%s: error interrupt, status=%x\n",
			    HDEVNAME(hp), error));

			if (ISSET(error, SDHC_CMD_TIMEOUT_ERROR|
			    SDHC_DATA_TIMEOUT_ERROR)) {
				hp->intr_error_status |= error;
				hp->intr_status |= status;
				cv_broadcast(&hp->intr_cv);
			}
		}

		/*
		 * Wake up the sdmmc event thread to scan for cards.
		 */
		if (ISSET(status, SDHC_CARD_REMOVAL|SDHC_CARD_INSERTION))
			sdmmc_needs_discover(hp->sdmmc);

		/*
		 * Wake up the blocking process to service command
		 * related interrupt(s).
		 */
		if (ISSET(status, SDHC_BUFFER_READ_READY|
		    SDHC_BUFFER_WRITE_READY|SDHC_COMMAND_COMPLETE|
		    SDHC_TRANSFER_COMPLETE|SDHC_DMA_INTERRUPT)) {
			hp->intr_status |= status;
			cv_broadcast(&hp->intr_cv);
		}

		/*
		 * Service SD card interrupts.
		 */
		if (ISSET(status, SDHC_CARD_INTERRUPT)) {
			DPRINTF(0,("%s: card interrupt\n", HDEVNAME(hp)));
			HCLR2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
			sdmmc_card_intr(hp->sdmmc);
		}
	}

	return done;
}

#ifdef SDHC_DEBUG
void
sdhc_dump_regs(struct sdhc_host *hp)
{

	printf("0x%02x PRESENT_STATE:    %x\n", SDHC_PRESENT_STATE,
	    HREAD4(hp, SDHC_PRESENT_STATE));
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
