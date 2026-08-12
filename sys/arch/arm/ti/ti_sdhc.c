/*	$NetBSD: ti_sdhc.c,v 1.13 2026/08/12 09:36:49 yurix Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_sdhc.c,v 1.13 2026/08/12 09:36:49 yurix Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/bus.h>

#include <arm/ti/ti_prcm.h>
#include <arm/ti/ti_sdhcreg.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/fdt/fdtvar.h>

#ifdef TISDHC_DEBUG
int tisdhcdebug = 1;
#define DPRINTF(n,s)    do { if ((n) <= tisdhcdebug) device_printf s; } while (0)
#else
#define DPRINTF(n,s)    do {} while (0)
#endif


#define CLKD(kz)	(sc->sc.sc_clkbase / (kz))

#define SDHC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_sdhc_bsh, (reg))
#define SDHC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_sdhc_bsh, (reg), (val))

struct ti_sdhc_config {
	bus_size_t		regoff;
	uint32_t		flags;
};

static const struct ti_sdhc_config omap2_hsmmc_config = {
};

static const struct ti_sdhc_config omap3_pre_es3_hsmmc_config = {
	.flags = SDHC_FLAG_SINGLE_ONLY
};

static const struct ti_sdhc_config omap4_hsmmc_config = {
	.regoff = 0x100
};

static const struct ti_sdhc_config am335_sdhci_config = {
	.regoff = 0x100
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap2-hsmmc",
	  .data = &omap2_hsmmc_config },
	{ .compat = "ti,omap3-hsmmc",
	  .data = &omap2_hsmmc_config },
	{ .compat = "ti,omap3-pre-es3-hsmmc",
	  .data = &omap3_pre_es3_hsmmc_config },
	{ .compat = "ti,omap4-hsmmc",
	  .data = &omap4_hsmmc_config },
	{ .compat = "ti,am335-sdhci",
	  .data = &am335_sdhci_config },

	DEVICE_COMPAT_EOL
};

enum {
	EDMA_CHAN_TX,
	EDMA_CHAN_RX,
	EDMA_NCHAN
};

struct ti_sdhc_softc {
	struct sdhc_softc	sc;
	int			sc_phandle;
	bus_addr_t		sc_addr;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_space_handle_t	sc_hl_bsh;
	bus_space_handle_t	sc_sdhc_bsh;
	struct sdhc_host	*sc_hosts[1];
	void 			*sc_ih;		/* interrupt vectoring */

	struct fdtbus_dma	*sc_rx_dma;
	struct fdtbus_dma	*sc_tx_dma;

	kcondvar_t		sc_edma_cv;
	bus_addr_t		sc_edma_fifo;
	bool			sc_edma_pending;
	bus_dmamap_t		sc_edma_dmamap;
	bus_dma_segment_t	sc_edma_segs[1];
	void			*sc_edma_bbuf;
};

static int ti_sdhc_match(device_t, cfdata_t, void *);
static void ti_sdhc_attach(device_t, device_t, void *);

static void ti_sdhc_init(struct ti_sdhc_softc *, const struct ti_sdhc_config *);

static int ti_sdhc_bus_width(struct sdhc_softc *, int);
static int ti_sdhc_rod(struct sdhc_softc *, int);
static int ti_sdhc_write_protect(struct sdhc_softc *);
static int ti_sdhc_card_detect(struct sdhc_softc *);

static int ti_sdhc_edma_init(struct ti_sdhc_softc *);
static int ti_sdhc_edma_xfer_data(struct sdhc_softc *, struct sdmmc_command *);
static void ti_sdhc_edma_done(void *);
static int ti_sdhc_edma_transfer(struct sdhc_softc *, struct sdmmc_command *);

CFATTACH_DECL_NEW(ti_sdhc, sizeof(struct ti_sdhc_softc),
    ti_sdhc_match, ti_sdhc_attach, NULL, NULL);

static int
ti_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct ti_sdhc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct ti_sdhc_config *conf;
	bus_addr_t addr;
	bus_size_t size;
	u_int bus_width;

	conf = of_compatible_lookup(phandle, compat_data)->data;

	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0 || size <= conf->regoff) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	addr += conf->regoff;
	size -= conf->regoff;

	sc->sc.sc_dmat = faa->faa_dmat;
	sc->sc.sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_addr = addr;
	sc->sc_bst = faa->faa_bst;

	sc->sc_rx_dma = fdtbus_dma_get(phandle, "rx", ti_sdhc_edma_done, sc);
	sc->sc_tx_dma = fdtbus_dma_get(phandle, "tx", ti_sdhc_edma_done, sc);

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "bus-width", &bus_width) != 0)
		bus_width = 4;

	sc->sc.sc_flags |= conf->flags;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_NO_LED_ON;
	sc->sc.sc_flags |= SDHC_FLAG_RSP136_CRC;
	if (bus_width == 8)
		sc->sc.sc_flags |= SDHC_FLAG_8BIT_MODE;
	if (of_hasprop(phandle, "ti,needs-special-reset"))
		sc->sc.sc_flags |= SDHC_FLAG_WAIT_RESET;
	if (!of_hasprop(phandle, "ti,needs-special-hs-handling"))
		sc->sc.sc_flags |= SDHC_FLAG_NO_HS_BIT;
	if (of_hasprop(phandle, "ti,dual-volt"))
		sc->sc.sc_caps = SDHC_VOLTAGE_SUPP_3_0V;

	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 96000;	/* 96MHZ */
	sc->sc.sc_clkmsk = 0x0000ffc0;
	sc->sc.sc_vendor_rod = ti_sdhc_rod;
	sc->sc.sc_vendor_write_protect = ti_sdhc_write_protect;
	sc->sc.sc_vendor_card_detect = ti_sdhc_card_detect;
	sc->sc.sc_vendor_bus_width = ti_sdhc_bus_width;

	if (bus_space_subregion(sc->sc_bst, sc->sc_bsh, 0x100, 0x100,
	    &sc->sc_sdhc_bsh) != 0) {
		aprint_error(": couldn't map subregion\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": MMCHS\n");

	ti_sdhc_init(sc, conf);
}

static void
ti_sdhc_init(struct ti_sdhc_softc *sc, const struct ti_sdhc_config *conf)
{
	device_t dev = sc->sc.sc_dev;
	uint32_t clkd, stat;
	int error, timo, clksft, n;
	char intrstr[128];

	if (sc->sc_rx_dma != NULL && sc->sc_tx_dma != NULL) {
		aprint_normal_dev(dev,"DMA available\n");

		if (ti_sdhc_edma_init(sc) != 0) {
			aprint_error_dev(dev, "EDMA disabled\n");
			goto no_dma;
		}

		cv_init(&sc->sc_edma_cv, "sdhcedma");
		sc->sc_edma_fifo = sc->sc_addr + 0x100 + SDHC_DATA;
		sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
		sc->sc.sc_flags |= SDHC_FLAG_EXTERNAL_DMA;
		sc->sc.sc_flags |= SDHC_FLAG_EXTDMA_DMAEN;
		sc->sc.sc_vendor_transfer_data_dma = ti_sdhc_edma_xfer_data;
	}
no_dma:

	/* XXXXXX: Turn-on regulator via I2C. */
	/* XXXXXX: And enable ICLOCK/FCLOCK. */

	SDHC_WRITE(sc, SDHC_CAPABILITIES,
	    SDHC_READ(sc, SDHC_CAPABILITIES) | SDHC_VOLTAGE_SUPP_1_8V);
	if (sc->sc.sc_caps & SDHC_VOLTAGE_SUPP_3_0V)
		SDHC_WRITE(sc, SDHC_CAPABILITIES,
		    SDHC_READ(sc, SDHC_CAPABILITIES) | SDHC_VOLTAGE_SUPP_3_0V);

	/* MMCHS Soft reset */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_SYSCONFIG,
	    SYSCONFIG_SOFTRESET);
	timo = 3000000;	/* XXXX 3 sec. */
	while (timo--) {
		if (bus_space_read_4(sc->sc_bst, sc->sc_bsh, MMCHS_SYSSTATUS) &
		    SYSSTATUS_RESETDONE)
			break;
		delay(1);
	}
	if (timo == 0)
		aprint_error_dev(dev, "Soft reset timeout\n");
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_SYSCONFIG,
	    SYSCONFIG_ENAWAKEUP |
#if notyet
	    SYSCONFIG_AUTOIDLE |
	    SYSCONFIG_SIDLEMODE_AUTO |
#else
	    SYSCONFIG_SIDLEMODE_IGNORE |
#endif
	    SYSCONFIG_CLOCKACTIVITY_FCLK |
	    SYSCONFIG_CLOCKACTIVITY_ICLK);

	if (!fdtbus_intr_str(sc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(dev, "couldn't decode interrupt\n");
		return;
	}
	sc->sc_ih = fdtbus_intr_establish_xname(sc->sc_phandle, 0, IPL_VM,
	    0, sdhc_intr, &sc->sc, device_xname(dev));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(dev, "couldn't establish interrupt\n");
		return;
	}
	aprint_normal_dev(dev, "interrupting on %s\n", intrstr);

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_sdhc_bsh, 0x100);
	if (error != 0) {
		aprint_error_dev(dev, "couldn't initialize host, error=%d\n",
		    error);
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
		return;
	}

	clksft = ffs(sc->sc.sc_clkmsk) - 1;

	/* Set SDVS 1.8v and DTW 1bit mode */
	SDHC_WRITE(sc, SDHC_HOST_CTL,
	    SDHC_VOLTAGE_1_8V << (SDHC_VOLTAGE_SHIFT + 8));
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_INTCLK_ENABLE |
							SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_HOST_CTL,
	    SDHC_READ(sc, SDHC_HOST_CTL) | SDHC_BUS_POWER << 8);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | CLKD(150) << clksft);

	/*
	 * 22.6.1.3.1.5 MMCHS Controller INIT Procedure Start
	 * from 'OMAP35x Applications Processor  Technical Reference Manual'.
	 *
	 * During the INIT procedure, the MMCHS controller generates 80 clock
	 * periods. In order to keep the 1ms gap, the MMCHS controller should
	 * be configured to generate a clock whose frequency is smaller or
	 * equal to 80 KHz.
	 */

	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~sc->sc.sc_clkmsk);
	clkd = CLKD(80);
	n = 1;
	while (clkd & ~(sc->sc.sc_clkmsk >> clksft)) {
		clkd >>= 1;
		n <<= 1;
	}
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | (clkd << clksft));
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_SDCLK_ENABLE);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON) | CON_INIT);
	SDHC_WRITE(sc, SDHC_TRANSFER_MODE, 0x00000000);
	delay(1000);
	stat = SDHC_READ(sc, SDHC_NINTR_STATUS);
	SDHC_WRITE(sc, SDHC_NINTR_STATUS, stat | SDHC_COMMAND_COMPLETE);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON) & ~CON_INIT);
	SDHC_WRITE(sc, SDHC_NINTR_STATUS, 0xffffffff);

	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~sc->sc.sc_clkmsk);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | CLKD(150) << clksft);
	timo = 3000000;	/* XXXX 3 sec. */
	while (--timo) {
		if (SDHC_READ(sc, SDHC_CLOCK_CTL) & SDHC_INTCLK_STABLE)
			break;
		delay(1);
	}
	if (timo == 0)
		aprint_error_dev(dev, "ICS timeout\n");
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_SDCLK_ENABLE);

#if notyet
	if (sc->sc_use_adma2) {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON,
		    bus_space_read_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON) |
		    CON_MNS);
	}
#endif
}

static int
ti_sdhc_rod(struct sdhc_softc *sc, int on)
{
	struct ti_sdhc_softc *hmsc = (struct ti_sdhc_softc *)sc;
	uint32_t con;

	con = bus_space_read_4(hmsc->sc_bst, hmsc->sc_bsh, MMCHS_CON);
	if (on)
		con |= CON_OD;
	else
		con &= ~CON_OD;
	bus_space_write_4(hmsc->sc_bst, hmsc->sc_bsh, MMCHS_CON, con);

	return 0;
}

static int
ti_sdhc_write_protect(struct sdhc_softc *sc)
{

	/* Maybe board dependent, using GPIO. Get GPIO-pin from prop? */
	return 0;	/* XXXXXXX */
}

static int
ti_sdhc_card_detect(struct sdhc_softc *sc)
{

	/* Maybe board dependent, using GPIO. Get GPIO-pin from prop? */
	return 1;	/* XXXXXXXX */
}

static int
ti_sdhc_bus_width(struct sdhc_softc *sc, int width)
{
	struct ti_sdhc_softc *hmsc = (struct ti_sdhc_softc *)sc;
	uint32_t con, hctl;

	con = bus_space_read_4(hmsc->sc_bst, hmsc->sc_bsh, MMCHS_CON);
	hctl = SDHC_READ(hmsc, SDHC_HOST_CTL);
	if (width == 8) {
		con |= CON_DW8;
	} else if (width == 4) {
		con &= ~CON_DW8;
		hctl |= SDHC_4BIT_MODE;
	} else {
		con &= ~CON_DW8;
		hctl &= ~SDHC_4BIT_MODE;
	}
	bus_space_write_4(hmsc->sc_bst, hmsc->sc_bsh, MMCHS_CON, con);
	SDHC_WRITE(hmsc, SDHC_HOST_CTL, hctl);

	return 0;
}

static int
ti_sdhc_edma_init(struct ti_sdhc_softc *sc)
{
	int error, rseg;

	/* Setup bounce buffer */
	error = bus_dmamem_alloc(sc->sc.sc_dmat, MAXPHYS, 32, MAXPHYS,
	    sc->sc_edma_segs, 1, &rseg, BUS_DMA_WAITOK);
	if (error) {
		aprint_error_dev(sc->sc.sc_dev,
		    "couldn't allocate dmamem: %d\n", error);
		return error;
	}
	KASSERT(rseg == 1);
	error = bus_dmamem_map(sc->sc.sc_dmat, sc->sc_edma_segs, rseg, MAXPHYS,
	    &sc->sc_edma_bbuf, BUS_DMA_WAITOK);
	if (error) {
		aprint_error_dev(sc->sc.sc_dev, "couldn't map dmamem: %d\n",
		    error);
		return error;
	}
	error = bus_dmamap_create(sc->sc.sc_dmat, MAXPHYS, 1, MAXPHYS, 0,
	    BUS_DMA_WAITOK, &sc->sc_edma_dmamap);
	if (error) {
		aprint_error_dev(sc->sc.sc_dev, "couldn't create dmamap: %d\n",
		    error);
		return error;
	}

	return error;
}

static int
ti_sdhc_edma_xfer_data(struct sdhc_softc *sdhc_sc, struct sdmmc_command *cmd)
{
	struct ti_sdhc_softc *sc = device_private(sdhc_sc->sc_dev);
	const bus_dmamap_t map = cmd->c_dmamap;
	bool bounce;
	int error;

#if notyet
	bounce = false;
	for (int seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		if ((cmd->c_dmamap->dm_segs[seg].ds_addr & 0x1f) != 0 ||
		    (cmd->c_dmamap->dm_segs[seg].ds_len & 3) != 0) {
			bounce = true;
			break;
		}
	}
#else
	bounce = true;
#endif

	if (bounce) {
		error = bus_dmamap_load(sc->sc.sc_dmat, sc->sc_edma_dmamap,
		    sc->sc_edma_bbuf, cmd->c_datalen, NULL,
		    BUS_DMA_NOWAIT | BUS_DMA_WRITE | BUS_DMA_READ);
		if (error) {
			device_printf(sc->sc.sc_dev,
			    "couldn't load dmamap: %d\n", error);
			return error;
		}

		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    cmd->c_datalen, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_edma_bbuf, cmd->c_data, cmd->c_datalen);
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    cmd->c_datalen, BUS_DMASYNC_PREWRITE);
		}

		cmd->c_dmamap = sc->sc_edma_dmamap;
	}

	error = ti_sdhc_edma_transfer(sdhc_sc, cmd);

	if (bounce) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    cmd->c_datalen, BUS_DMASYNC_POSTREAD);
		} else {
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    cmd->c_datalen, BUS_DMASYNC_POSTWRITE);
		}
		if (ISSET(cmd->c_flags, SCF_CMD_READ) && error == 0) {
			memcpy(cmd->c_data, sc->sc_edma_bbuf, cmd->c_datalen);
		}

		bus_dmamap_unload(sc->sc.sc_dmat, sc->sc_edma_dmamap);

		cmd->c_dmamap = map;
	}

	return error;
}

static int
ti_sdhc_edma_transfer(struct sdhc_softc *sdhc_sc, struct sdmmc_command *cmd)
{
	struct ti_sdhc_softc *sc = device_private(sdhc_sc->sc_dev);
	kmutex_t *plock = sdhc_host_lock(sc->sc_hosts[0]);
	int error;
	int blksize = MIN(cmd->c_datalen, cmd->c_blklen);

	/*
	 * In constant addressing mode, the address must be aligned
	 * to 256-bits/32 bytes.
	 */
	KASSERT(cmd->c_dmamap->dm_nsegs == 1);
	KASSERT((cmd->c_dmamap->dm_segs[0].ds_addr & 0x1f) == 0);

	struct fdtbus_dma_req req;
	memset(&req, 0, sizeof(req));
	req.dreq_segs = cmd->c_dmamap->dm_segs;
	req.dreq_nsegs = cmd->c_dmamap->dm_nsegs;
	req.dreq_block_irq = 1;
	req.dreq_block_multi = 1;
	req.dreq_dev_opt.opt_bus_width = uimin(cmd->c_datalen, 64);
	req.dreq_dev_opt.opt_burst_len = uimin(cmd->c_datalen, blksize);
	req.dreq_dev_phys = sc->sc_edma_fifo;
	req.dreq_sel = 1; /* use SAM/DAM mode */

	sc->sc_edma_pending = true;
	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		req.dreq_dir = FDT_DMA_READ;

		fdtbus_dma_transfer(sc->sc_rx_dma, &req);
	} else {
		req.dreq_dir = FDT_DMA_WRITE;

		fdtbus_dma_transfer(sc->sc_tx_dma, &req);
	}

	KASSERT(mutex_owned(plock));

	error = 0;
	while (sc->sc_edma_pending) {
		error = cv_timedwait(&sc->sc_edma_cv, plock, hz*10);
		if (error == EWOULDBLOCK) {
			device_printf(sc->sc.sc_dev, "transfer timeout!\n");
			sc->sc_edma_pending = false;
			error = ETIMEDOUT;
			break;
		}
	}
	if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
		fdtbus_dma_halt(sc->sc_rx_dma);
	} else {
		fdtbus_dma_halt(sc->sc_tx_dma);
	}

	return error;
}

static void
ti_sdhc_edma_done(void *priv)
{
	struct ti_sdhc_softc *sc = priv;
	kmutex_t *plock = sdhc_host_lock(sc->sc_hosts[0]);

	mutex_enter(plock);
	KASSERT(sc->sc_edma_pending == true);
	sc->sc_edma_pending = false;
	cv_broadcast(&sc->sc_edma_cv);
	mutex_exit(plock);
}
