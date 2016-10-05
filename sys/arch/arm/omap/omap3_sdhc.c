/*	$NetBSD: omap3_sdhc.c,v 1.14.6.7 2016/10/05 20:55:25 skrll Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: omap3_sdhc.c,v 1.14.6.7 2016/10/05 20:55:25 skrll Exp $");

#include "opt_omap.h"
#include "edma.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/bus.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap3_sdmmcreg.h>

#ifdef TI_AM335X
#  include <arm/omap/am335x_prcm.h>
#  include <arm/omap/omap2_prcm.h>
#  include <arm/omap/sitara_cm.h>
#  include <arm/omap/sitara_cmreg.h>
#endif

#if NEDMA > 0
#  include <arm/omap/omap_edma.h>
#endif

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#ifdef TI_AM335X
#define EDMA_MAX_PARAMS		32
#endif

#ifdef OM3SDHC_DEBUG
int om3sdhcdebug = 1;
#define DPRINTF(n,s)    do { if ((n) <= om3sdhcdebug) device_printf s; } while (0)
#else
#define DPRINTF(n,s)    do {} while (0)
#endif


#define CLKD(kz)	(sc->sc.sc_clkbase / (kz))

#define SDHC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_sdhc_bsh, (reg))
#define SDHC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_sdhc_bsh, (reg), (val))

static int obiosdhc_match(device_t, cfdata_t, void *);
static void obiosdhc_attach(device_t, device_t, void *);
static int obiosdhc_detach(device_t, int);

static int obiosdhc_bus_width(struct sdhc_softc *, int);
static int obiosdhc_rod(struct sdhc_softc *, int);
static int obiosdhc_write_protect(struct sdhc_softc *);
static int obiosdhc_card_detect(struct sdhc_softc *);

struct obiosdhc_softc {
	struct sdhc_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_space_handle_t	sc_hl_bsh;
	bus_space_handle_t	sc_sdhc_bsh;
	struct sdhc_host	*sc_hosts[1];
	void 			*sc_ih;		/* interrupt vectoring */

#if NEDMA > 0
	struct edma_channel	*sc_edma_tx;
	struct edma_channel	*sc_edma_rx;
	uint16_t		sc_edma_param_tx[EDMA_MAX_PARAMS];
	uint16_t		sc_edma_param_rx[EDMA_MAX_PARAMS];
	kcondvar_t		sc_edma_cv;
	bus_addr_t		sc_edma_fifo;
	bool			sc_edma_pending;
	bus_dmamap_t		sc_edma_dmamap;
	bus_dma_segment_t	sc_edma_segs[1];
	void			*sc_edma_bbuf;
#endif
};

#if NEDMA > 0
static int obiosdhc_edma_init(struct obiosdhc_softc *, unsigned int);
static int obiosdhc_edma_xfer_data(struct sdhc_softc *, struct sdmmc_command *);
static void obiosdhc_edma_done(void *);
static int obiosdhc_edma_transfer(struct sdhc_softc *, struct sdmmc_command *);
#endif

#ifdef TI_AM335X
struct am335x_sdhc {
	const char *as_name;
	bus_addr_t as_base_addr;
	int as_intr;
	struct omap_module as_module;
};

static const struct am335x_sdhc am335x_sdhc[] = {
	/* XXX All offset by 0x100 because of the am335x's mmc registers.  */
	{ "MMCHS0", SDMMC1_BASE_TIAM335X, 64, { AM335X_PRCM_CM_PER, 0x3c } },
	{ "MMC1",   SDMMC2_BASE_TIAM335X, 28, { AM335X_PRCM_CM_PER, 0xf4 } },
	{ "MMCHS2", SDMMC3_BASE_TIAM335X, 29, { AM335X_PRCM_CM_WKUP, 0xf8 } },
};

struct am335x_padconf {
	const char *padname;
	const char *padmode;
};
const struct am335x_padconf am335x_padconf_mmc1[] = {
	{ "GPMC_CSn1", "mmc1_clk" },
	{ "GPMC_CSn2", "mmc1_cmd" },
	{ "GPMC_AD0", "mmc1_dat0" },
	{ "GPMC_AD1", "mmc1_dat1" },
	{ "GPMC_AD2", "mmc1_dat2" },
	{ "GPMC_AD3", "mmc1_dat3" },
	{ "GPMC_AD4", "mmc1_dat4" },
	{ "GPMC_AD5", "mmc1_dat5" },
	{ "GPMC_AD6", "mmc1_dat6" },
	{ "GPMC_AD7", "mmc1_dat7" },
	{ NULL, NULL }
};
#endif

CFATTACH_DECL_NEW(obiosdhc, sizeof(struct obiosdhc_softc),
    obiosdhc_match, obiosdhc_attach, obiosdhc_detach, NULL);

static int
obiosdhc_match(device_t parent, cfdata_t cf, void *aux)
{
#if defined(OMAP_3430) || defined(OMAP_3530) || defined(OMAP4)
	struct obio_attach_args * const oa = aux;
#endif
#ifdef TI_AM335X
	struct obio_attach_args * const oa = aux;
	size_t i;
#endif

#if defined(OMAP_3430)
	if (oa->obio_addr == SDMMC1_BASE_3430
	    || oa->obio_addr == SDMMC2_BASE_3430
	    || oa->obio_addr == SDMMC3_BASE_3430)
		return 1;
#elif defined(OMAP_3530)
	if (oa->obio_addr == SDMMC1_BASE_3530
	    || oa->obio_addr == SDMMC2_BASE_3530
	    || oa->obio_addr == SDMMC3_BASE_3530)
		return 1;
#elif defined(OMAP4) || defined(OMAP5)
	if (oa->obio_addr == SDMMC1_BASE_4430
	    || oa->obio_addr == SDMMC2_BASE_4430
	    || oa->obio_addr == SDMMC3_BASE_4430
	    || oa->obio_addr == SDMMC4_BASE_4430
	    || oa->obio_addr == SDMMC5_BASE_4430)
		return 1;
#endif

#ifdef TI_AM335X
	for (i = 0; i < __arraycount(am335x_sdhc); i++)
		if ((oa->obio_addr == am335x_sdhc[i].as_base_addr) &&
		    (oa->obio_intr == am335x_sdhc[i].as_intr))
			return 1;
#endif

	return 0;
}

static void
obiosdhc_attach(device_t parent, device_t self, void *aux)
{
	struct obiosdhc_softc * const sc = device_private(self);
	struct obio_attach_args * const oa = aux;
	prop_dictionary_t prop = device_properties(self);
	uint32_t clkd, stat;
	int error, timo, clksft, n;
	bool support8bit = false;
	const char *transfer_mode = "PIO";
#if defined(OMAP4)
	uint32_t v;
	int x, y;
#endif
#ifdef TI_AM335X
	size_t i;
#endif

	prop_dictionary_get_bool(prop, "8bit", &support8bit);

	sc->sc.sc_dmat = oa->obio_dmat;
	sc->sc.sc_dev = self;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_NO_LED_ON;
	sc->sc.sc_flags |= SDHC_FLAG_RSP136_CRC;
	if (support8bit)
		sc->sc.sc_flags |= SDHC_FLAG_8BIT_MODE;
#if defined(OMAP_3430) || /* XXX until TI_DM37XX has working omap_devid() */ defined(TI_DM37XX)
	sc->sc.sc_flags |= SDHC_FLAG_SINGLE_ONLY;
#elif defined(OMAP_3530) || defined(TI_DM37XX)
	/*
	 * Advisory 2.1.1.128: MMC: Multiple Block Read Operation Issue
	 * from "OMAP3530/25/15/03 Applications Processor Silicon Revisions
	 * 3.1.2, 3.1, 3.0, 2.1, and 2.0".
	 */
	switch (omap_devid()) {
	case DEVID_OMAP35X_ES10:
	case DEVID_OMAP35X_ES20:
	case DEVID_OMAP35X_ES21:
	case DEVID_AMDM37X_ES10:	/* XXXX ? */
	case DEVID_AMDM37X_ES11:	/* XXXX ? */
	case DEVID_AMDM37X_ES12:	/* XXXX ? */
		sc->sc.sc_flags |= SDHC_FLAG_SINGLE_ONLY;
		break;
	default:
		break;
	}
	sc->sc.sc_flags |= SDHC_FLAG_NO_HS_BIT;
#elif defined(TI_AM335X)
	sc->sc.sc_flags |= SDHC_FLAG_WAIT_RESET;
#elif defined(OMAP_4430)
	/*
	 * MMCHS_HCTL.HSPE Is Not Functional
	 * Errata ID: i626
	 *
	 * Due to design issue MMCHS_HCTL.HSPE bit does not work as intended.
	 * This means that the configuration must always be the normal speed
	 * mode configuration (MMCHS_HCTL.HSPE=0).
	 */
	sc->sc.sc_flags |= SDHC_FLAG_NO_HS_BIT;

//	sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
//	sc->sc.sc_flags |= SDHC_FLAG_USE_ADMA2;
#endif
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 96000;	/* 96MHZ */
	if (!prop_dictionary_get_uint32(prop, "clkmask", &sc->sc.sc_clkmsk))
		sc->sc.sc_clkmsk = 0x0000ffc0;
	sc->sc.sc_vendor_rod = obiosdhc_rod;
	sc->sc.sc_vendor_write_protect = obiosdhc_write_protect;
	sc->sc.sc_vendor_card_detect = obiosdhc_card_detect;
	sc->sc.sc_vendor_bus_width = obiosdhc_bus_width;
	sc->sc_bst = oa->obio_iot;

	clksft = ffs(sc->sc.sc_clkmsk) - 1;

#if defined(TI_AM335X)
	error = bus_space_map(sc->sc_bst, oa->obio_addr + OMAP4_SDMMC_HL_SIZE,
	    oa->obio_size - OMAP4_SDMMC_HL_SIZE, 0, &sc->sc_bsh);
#elif defined(OMAP4)
	error = bus_space_map(sc->sc_bst, oa->obio_addr, oa->obio_size, 0,
	    &sc->sc_hl_bsh);
	if (!error)
		bus_space_subregion(sc->sc_bst, sc->sc_hl_bsh,
		    OMAP4_SDMMC_HL_SIZE, oa->obio_size - OMAP4_SDMMC_HL_SIZE,
		    &sc->sc_bsh);
#else
	error = bus_space_map(sc->sc_bst, oa->obio_addr, oa->obio_size, 0,
	    &sc->sc_bsh);
#endif
	if (error) {
		aprint_error_dev(self,
		    "can't map registers: %d\n", error);
		return;
	}

	bus_space_subregion(sc->sc_bst, sc->sc_bsh, OMAP3_SDMMC_SDHC_OFFSET,
	    OMAP3_SDMMC_SDHC_SIZE, &sc->sc_sdhc_bsh);

#if NEDMA > 0
	if (oa->obio_edmabase != -1) {
		if (obiosdhc_edma_init(sc, oa->obio_edmabase) != 0)
			goto no_dma;

		cv_init(&sc->sc_edma_cv, "sdhcedma");
		sc->sc_edma_fifo = oa->obio_addr +
#ifdef TI_AM335X
		    OMAP4_SDMMC_HL_SIZE +
#endif
		    OMAP3_SDMMC_SDHC_OFFSET + SDHC_DATA;
		sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
		sc->sc.sc_flags |= SDHC_FLAG_EXTERNAL_DMA;
		sc->sc.sc_flags |= SDHC_FLAG_EXTDMA_DMAEN;
		sc->sc.sc_vendor_transfer_data_dma = obiosdhc_edma_xfer_data;
		transfer_mode = "EDMA";
	}
no_dma:
#endif
	if (sc->sc.sc_flags & SDHC_FLAG_USE_ADMA2)
		transfer_mode = "ADMA2";

	aprint_naive("\n");
	aprint_normal(": SDHC controller (%s)\n", transfer_mode);

#if defined(OMAP4)
	v = bus_space_read_4(sc->sc_bst, sc->sc_hl_bsh, MMCHS_HL_REV);
	aprint_normal_dev(sc->sc.sc_dev, "IP Rev 0x%08x", v);
	v = bus_space_read_4(sc->sc_bst, sc->sc_hl_bsh, MMCHS_HL_HWINFO);
	aprint_normal("%s", v & HL_HWINFO_RETMODE ? ", Retention Mode" : "");
	x = 0;
	switch (v & HL_HWINFO_MEM_SIZE_MASK) {
	case HL_HWINFO_MEM_SIZE_512: 	x = 512;	y = 512;	break;
	case HL_HWINFO_MEM_SIZE_1024:	x = 1024;	y = 1024;	break;
	case HL_HWINFO_MEM_SIZE_2048:	x = 2048;	y = 2048;	break;
	case HL_HWINFO_MEM_SIZE_4096:	x = 4096;	y = 2048;	break;
	}
	if (x != 0)
		aprint_normal(", %d byte FIFO, max block length %d bytes",
		    x, y);
	aprint_normal("\n");
#endif

#if NEDMA > 0
	if (strcmp(transfer_mode, "EDMA") == 0)
		aprint_normal_dev(sc->sc.sc_dev,
		    "EDMA tx channel %d, rx channel %d\n",
		    edma_channel_index(sc->sc_edma_tx),
		    edma_channel_index(sc->sc_edma_rx));
#endif

#ifdef TI_AM335X
	/* XXX Not really AM335X-specific.  */
	for (i = 0; i < __arraycount(am335x_sdhc); i++)
		if ((oa->obio_addr == am335x_sdhc[i].as_base_addr) &&
		    (oa->obio_intr == am335x_sdhc[i].as_intr)) {
			prcm_module_enable(&am335x_sdhc[i].as_module);
			break;
		}
	KASSERT(i < __arraycount(am335x_sdhc));

	if (oa->obio_addr == SDMMC2_BASE_TIAM335X) {
		const char *mode;
		u_int state;

		const struct am335x_padconf *padconf = am335x_padconf_mmc1;
		for (i = 0; padconf[i].padname; i++) {
			const char *padname = padconf[i].padname;
			const char *padmode = padconf[i].padmode;
			if (sitara_cm_padconf_get(padname, &mode, &state) == 0) {
				aprint_debug_dev(self, "%s mode %s state %d\n",
				    padname, mode, state);
			}
			if (sitara_cm_padconf_set(padname, padmode,
			    (1 << 4) | (1 << 5)) != 0) {
				aprint_error_dev(self, "can't switch %s pad from %s to %s\n",
				    padname, mode, padmode);
				return;
			}
		}
	}
#endif

	/* XXXXXX: Turn-on regulator via I2C. */
	/* XXXXXX: And enable ICLOCK/FCLOCK. */

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
		aprint_error_dev(self, "Soft reset timeout\n");
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_SYSCONFIG,
	    SYSCONFIG_ENAWAKEUP | SYSCONFIG_AUTOIDLE | SYSCONFIG_SIDLEMODE_AUTO |
	    SYSCONFIG_CLOCKACTIVITY_FCLK | SYSCONFIG_CLOCKACTIVITY_ICLK);

	sc->sc_ih = intr_establish(oa->obio_intr, IPL_SDMMC, IST_LEVEL,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     oa->obio_intr);
		goto fail;
	}

	error = sdhc_host_found(&sc->sc, sc->sc_bst, sc->sc_sdhc_bsh,
	    oa->obio_size - OMAP3_SDMMC_SDHC_OFFSET);
	if (error != 0) {
		aprint_error_dev(self, "couldn't initialize host, error=%d\n",
		    error);
		goto fail;
	}

	/* Set SDVS 1.8v and DTW 1bit mode */
	SDHC_WRITE(sc, SDHC_HOST_CTL,
	    SDHC_VOLTAGE_1_8V << (SDHC_VOLTAGE_SHIFT + 8));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON) | CON_OD);
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
		aprint_error_dev(self, "ICS timeout(2)\n");
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_SDCLK_ENABLE);

	return;

fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, oa->obio_size);
}

static int
obiosdhc_detach(device_t self, int flags)
{
//	struct obiosdhc_softc *sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);

	/* XXXXXX: Regurator turn-off via I2C. */
	/* XXXXXX: And disable ICLOCK/FCLOCK. */

	return error;
}

static int
obiosdhc_rod(struct sdhc_softc *sc, int on)
{
	struct obiosdhc_softc *osc = (struct obiosdhc_softc *)sc;
	uint32_t con;

	con = bus_space_read_4(osc->sc_bst, osc->sc_bsh, MMCHS_CON);
	if (on)
		con |= CON_OD;
	else
		con &= ~CON_OD;
	bus_space_write_4(osc->sc_bst, osc->sc_bsh, MMCHS_CON, con);

	return 0;
}

static int
obiosdhc_write_protect(struct sdhc_softc *sc)
{

	/* Maybe board dependent, using GPIO. Get GPIO-pin from prop? */
	return 0;	/* XXXXXXX */
}

static int
obiosdhc_card_detect(struct sdhc_softc *sc)
{

	/* Maybe board dependent, using GPIO. Get GPIO-pin from prop? */
	return 1;	/* XXXXXXXX */
}

static int
obiosdhc_bus_width(struct sdhc_softc *sc, int width)
{
	struct obiosdhc_softc *osc = (struct obiosdhc_softc *)sc;
	uint32_t con;

	con = bus_space_read_4(osc->sc_bst, osc->sc_bsh, MMCHS_CON);
	if (width == 8) {
		con |= CON_DW8;
	} else {
		con &= ~CON_DW8;
	}
	bus_space_write_4(osc->sc_bst, osc->sc_bsh, MMCHS_CON, con);

	return 0;
}

#if NEDMA > 0
static int
obiosdhc_edma_init(struct obiosdhc_softc *sc, unsigned int edmabase)
{
	int i, error, rseg;

	/* Request tx and rx DMA channels */
	sc->sc_edma_tx = edma_channel_alloc(EDMA_TYPE_DMA, edmabase + 0,
	    obiosdhc_edma_done, sc);
	KASSERT(sc->sc_edma_tx != NULL);
	sc->sc_edma_rx = edma_channel_alloc(EDMA_TYPE_DMA, edmabase + 1,
	    obiosdhc_edma_done, sc);
	KASSERT(sc->sc_edma_rx != NULL);

	/* Allocate some PaRAM pages */
	for (i = 0; i < __arraycount(sc->sc_edma_param_tx); i++) {
		sc->sc_edma_param_tx[i] = edma_param_alloc(sc->sc_edma_tx);
		KASSERT(sc->sc_edma_param_tx[i] != 0xffff);
	}
	for (i = 0; i < __arraycount(sc->sc_edma_param_rx); i++) {
		sc->sc_edma_param_rx[i] = edma_param_alloc(sc->sc_edma_rx);
		KASSERT(sc->sc_edma_param_rx[i] != 0xffff);
	}

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
obiosdhc_edma_xfer_data(struct sdhc_softc *sdhc_sc, struct sdmmc_command *cmd)
{
	struct obiosdhc_softc *sc = device_private(sdhc_sc->sc_dev);
	const bus_dmamap_t map = cmd->c_dmamap;
	int seg, error;
	bool bounce;

	for (bounce = false, seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		if ((cmd->c_dmamap->dm_segs[seg].ds_addr & 0x1f) != 0) {
			bounce = true;
			break;
		}
	}

	if (bounce) {
		error = bus_dmamap_load(sc->sc.sc_dmat, sc->sc_edma_dmamap,
		    sc->sc_edma_bbuf, MAXPHYS, NULL, BUS_DMA_WAITOK);
		if (error) {
			device_printf(sc->sc.sc_dev,
			    "[bounce] bus_dmamap_load failed: %d\n", error);
			return error;
		}
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_edma_bbuf, cmd->c_data, cmd->c_datalen);
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_PREWRITE);
		}

		cmd->c_dmamap = sc->sc_edma_dmamap;
	}

	error = obiosdhc_edma_transfer(sdhc_sc, cmd);

	if (bounce) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_POSTREAD);
		} else {
			bus_dmamap_sync(sc->sc.sc_dmat, sc->sc_edma_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_POSTWRITE);
		}
		bus_dmamap_unload(sc->sc.sc_dmat, sc->sc_edma_dmamap);
		if (ISSET(cmd->c_flags, SCF_CMD_READ) && error == 0) {
			memcpy(cmd->c_data, sc->sc_edma_bbuf, cmd->c_datalen);
		}

		cmd->c_dmamap = map;
	}

	return error;
}

static int
obiosdhc_edma_transfer(struct sdhc_softc *sdhc_sc, struct sdmmc_command *cmd)
{
	struct obiosdhc_softc *sc = device_private(sdhc_sc->sc_dev);
	kmutex_t *plock = sdhc_host_lock(sc->sc_hosts[0]);
	struct edma_channel *edma;
	uint16_t *edma_param;
	struct edma_param ep;
	size_t seg;
	int error, resid = cmd->c_datalen;
	int blksize = MIN(cmd->c_datalen, cmd->c_blklen);

	KASSERT(mutex_owned(plock));

	edma = ISSET(cmd->c_flags, SCF_CMD_READ) ?
	    sc->sc_edma_rx : sc->sc_edma_tx;
	edma_param = ISSET(cmd->c_flags, SCF_CMD_READ) ?
	    sc->sc_edma_param_rx : sc->sc_edma_param_tx;

	DPRINTF(1, (sc->sc.sc_dev, "edma xfer: nsegs=%d ch# %d\n",
	    cmd->c_dmamap->dm_nsegs, edma_channel_index(edma)));

	if (cmd->c_dmamap->dm_nsegs > EDMA_MAX_PARAMS) {
		return ENOMEM;
	}

	for (seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		KASSERT(resid > 0);
		const int xferlen = min(resid,
		    cmd->c_dmamap->dm_segs[seg].ds_len);
		KASSERT(xferlen == cmd->c_dmamap->dm_segs[seg].ds_len ||
			seg == cmd->c_dmamap->dm_nsegs - 1);
		resid -= xferlen;
		KASSERT((xferlen & 0x3) == 0);
		ep.ep_opt = __SHIFTIN(2, EDMA_PARAM_OPT_FWID) /* 32-bit */;
		ep.ep_opt |= __SHIFTIN(edma_channel_index(edma),
				       EDMA_PARAM_OPT_TCC);
		if (seg == cmd->c_dmamap->dm_nsegs - 1) {
			ep.ep_opt |= EDMA_PARAM_OPT_TCINTEN;
			ep.ep_link = 0xffff;
		} else {
			ep.ep_link = EDMA_PARAM_BASE(edma_param[seg+1]);
		}
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			ep.ep_opt |= EDMA_PARAM_OPT_SAM;
			ep.ep_src = sc->sc_edma_fifo;
			ep.ep_dst = cmd->c_dmamap->dm_segs[seg].ds_addr;
		} else {
			ep.ep_opt |= EDMA_PARAM_OPT_DAM;
			ep.ep_src = cmd->c_dmamap->dm_segs[seg].ds_addr;
			ep.ep_dst = sc->sc_edma_fifo;
		}

		KASSERT(xferlen <= 65536 * 4);

		/*
		 * In constant addressing mode, the address must be aligned
		 * to 256-bits.
		 */
		KASSERT((cmd->c_dmamap->dm_segs[seg].ds_addr & 0x1f) == 0);

		/*
		 * For unknown reason, the A-DMA transfers never completes for
		 * transfers larger than 64 butes. So use a AB transfer,
		 * with a 64 bytes A len
		 */
		ep.ep_bcntrld = 0;	/* not used for AB-synchronous mode */
		ep.ep_opt |= EDMA_PARAM_OPT_SYNCDIM;
		ep.ep_acnt = min(xferlen, 64);
		ep.ep_bcnt = min(xferlen, blksize) / ep.ep_acnt;
		ep.ep_ccnt = xferlen / (ep.ep_acnt * ep.ep_bcnt);
		ep.ep_srcbidx = ep.ep_dstbidx = 0;
		ep.ep_srccidx = ep.ep_dstcidx = 0;
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			ep.ep_dstbidx = ep.ep_acnt;
			ep.ep_dstcidx = ep.ep_acnt * ep.ep_bcnt;
		} else {
			ep.ep_srcbidx = ep.ep_acnt;
			ep.ep_srccidx = ep.ep_acnt * ep.ep_bcnt;
		}

		edma_set_param(edma, edma_param[seg], &ep);
#ifdef OM3SDHC_DEBUG
		if (om3sdhcdebug >= 1) {
			printf("target OPT: %08x\n", ep.ep_opt);
			edma_dump_param(edma, edma_param[seg]);
		}
#endif
	}

	error = 0;
	sc->sc_edma_pending = true;
	edma_transfer_enable(edma, edma_param[0]);
	while (sc->sc_edma_pending) {
		error = cv_timedwait(&sc->sc_edma_cv, plock, hz*10);
		if (error == EWOULDBLOCK) {
			device_printf(sc->sc.sc_dev, "transfer timeout!\n");
			edma_dump(edma);
			edma_dump_param(edma, edma_param[0]);
			edma_halt(edma);
			sc->sc_edma_pending = false;
			error = ETIMEDOUT;
			break;
		}
	}
	edma_halt(edma);

	return error;
}

static void
obiosdhc_edma_done(void *priv)
{
	struct obiosdhc_softc *sc = priv;
	kmutex_t *plock = sdhc_host_lock(sc->sc_hosts[0]);

	mutex_enter(plock);
	KASSERT(sc->sc_edma_pending == true);
	sc->sc_edma_pending = false;
	cv_broadcast(&sc->sc_edma_cv);
	mutex_exit(plock);
}
#endif
