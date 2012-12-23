/*	$NetBSD: omap3_sdhc.c,v 1.7 2012/12/23 18:34:01 jakllsch Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: omap3_sdhc.c,v 1.7 2012/12/23 18:34:01 jakllsch Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap3_sdmmcreg.h>

#ifdef TI_AM335X
#  include <arm/omap/am335x_prcm.h>
#  include <arm/omap/omap2_prcm.h>
#endif

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>

#define CLKD(kz)	(sc->sc.sc_clkbase / (kz))

#define SDHC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_sdhc_bsh, (reg))
#define SDHC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_sdhc_bsh, (reg), (val))

static int obiosdhc_match(device_t, cfdata_t, void *);
static void obiosdhc_attach(device_t, device_t, void *);
static int obiosdhc_detach(device_t, int);

static int obiosdhc_bus_clock(struct sdhc_softc *, int);
static int obiosdhc_rod(struct sdhc_softc *, int);
static int obiosdhc_write_protect(struct sdhc_softc *);
static int obiosdhc_card_detect(struct sdhc_softc *);

struct obiosdhc_softc {
	struct sdhc_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_space_handle_t	sc_sdhc_bsh;
	struct sdhc_host	*sc_hosts[1];
	void 			*sc_ih;		/* interrupt vectoring */
};

#ifdef TI_AM335X
struct am335x_sdhc {
	const char *as_name;
	bus_addr_t as_base_addr;
	int as_intr;
	struct omap_module as_module;
};

static const struct am335x_sdhc am335x_sdhc[] = {
	/* XXX All offset by 0x100 because of the am335x's mmc registers.  */
	{ "MMCHS0",	0x48060100, 64, { AM335X_PRCM_CM_PER, 0x3c } },
	{ "MMC1",	0x481d8100, 28, { AM335X_PRCM_CM_PER, 0xf4 } },
	{ "MMCHS2",	0x47810100, 29, { AM335X_PRCM_CM_WKUP, 0xf8 } },
};
#endif

CFATTACH_DECL_NEW(obiosdhc, sizeof(struct obiosdhc_softc),
    obiosdhc_match, obiosdhc_attach, obiosdhc_detach, NULL);

static int
obiosdhc_match(device_t parent, cfdata_t cf, void *aux)
{
#if defined(OMAP_3430) || defined(OMAP_3530)
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
#ifdef TI_AM335X
	size_t i;
#endif

	sc->sc.sc_dmat = oa->obio_dmat;
	sc->sc.sc_dev = self;
	//sc->sc.sc_flags |= SDHC_FLAG_USE_DMA;
	sc->sc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;
	sc->sc.sc_flags |= SDHC_FLAG_NO_LED_ON;
	sc->sc.sc_flags |= SDHC_FLAG_RSP136_CRC;
	sc->sc.sc_flags |= SDHC_FLAG_SINGLE_ONLY;
#ifdef TI_AM335X
	sc->sc.sc_flags |= SDHC_FLAG_WAIT_RESET;
	sc->sc.sc_flags &= ~SDHC_FLAG_SINGLE_ONLY;
#endif
	sc->sc.sc_host = sc->sc_hosts;
	sc->sc.sc_clkbase = 96000;	/* 96MHZ */
	if (!prop_dictionary_get_uint32(prop, "clkmask", &sc->sc.sc_clkmsk))
		sc->sc.sc_clkmsk = 0x0000ffc0;
	sc->sc.sc_vendor_rod = obiosdhc_rod;
	sc->sc.sc_vendor_write_protect = obiosdhc_write_protect;
	sc->sc.sc_vendor_card_detect = obiosdhc_card_detect;
	sc->sc.sc_vendor_bus_clock = obiosdhc_bus_clock;
	sc->sc_bst = oa->obio_iot;

	clksft = ffs(sc->sc.sc_clkmsk) - 1;

	error = bus_space_map(sc->sc_bst, oa->obio_addr, oa->obio_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers: %d\n", error);
		return;
	}

	bus_space_subregion(sc->sc_bst, sc->sc_bsh, OMAP3_SDMMC_SDHC_OFFSET,
	    OMAP3_SDMMC_SDHC_SIZE, &sc->sc_sdhc_bsh);

	aprint_naive(": SDHC controller\n");
	aprint_normal(": SDHC controller\n");

#ifdef TI_AM335X
	/* XXX Not really AM335X-specific.  */
	for (i = 0; i < __arraycount(am335x_sdhc); i++)
		if ((oa->obio_addr == am335x_sdhc[i].as_base_addr) &&
		    (oa->obio_intr == am335x_sdhc[i].as_intr)) {
			prcm_module_enable(&am335x_sdhc[i].as_module);
			break;
		}
	KASSERT(i < __arraycount(am335x_sdhc));
#endif

	/* XXXXXX: Turn-on regurator via I2C. */
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

	sc->sc_ih = intr_establish(oa->obio_intr, IPL_VM, IST_LEVEL,
	    sdhc_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     oa->obio_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     oa->obio_intr);

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
	for (; n > 0; n--) {
		SDHC_WRITE(sc, SDHC_TRANSFER_MODE, 0x00000000);
		timo = 3000000;	/* XXXX 3 sec. */
		stat = 0;
		while (!(stat & SDHC_COMMAND_COMPLETE)) {
			stat = SDHC_READ(sc, SDHC_NINTR_STATUS);
			if (--timo == 0)
				break;
			delay(1);
		}
		if (timo == 0) {
			aprint_error_dev(self, "INIT Procedure timeout\n");
			break;
		}
		SDHC_WRITE(sc, SDHC_NINTR_STATUS, stat);
	}
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, MMCHS_CON) & ~CON_INIT);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~sc->sc.sc_clkmsk);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | CLKD(150) << clksft);
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
obiosdhc_bus_clock(struct sdhc_softc *sc, int clk)
{
	struct obiosdhc_softc *osc = (struct obiosdhc_softc *)sc;
	uint32_t ctl;

	ctl = bus_space_read_4(osc->sc_bst, osc->sc_bsh, MMCHS_SYSCTL);
	if (clk == 0) {
		clk &= ~SYSCTL_CEN;
	} else {
		clk |= SYSCTL_CEN;
	}
	bus_space_write_4(osc->sc_bst, osc->sc_bsh, MMCHS_SYSCTL, ctl);

	return 0;
}
