/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

#include "opt_broadcom.h"
#include "locators.h"

#define IDM_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_ccb.c,v 1.1.2.2 2014/08/20 00:02:45 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/mainbus/mainbus.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static int bcmccb_mainbus_match(device_t, cfdata_t, void *);
static void bcmccb_mainbus_attach(device_t, device_t, void *);

struct bcmccb_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

static struct bcmccb_softc bcmccb_sc;

CFATTACH_DECL_NEW(bcmccb, 0,
    bcmccb_mainbus_match, bcmccb_mainbus_attach, NULL, NULL);

static int
bcmccb_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	if (bcmccb_sc.sc_dev != NULL)
		return 0;

	return 1;
}

static int
bcmccb_print(void *aux, const char *pnp)
{
	const struct bcmccb_attach_args * const ccbaa = aux;

	if (ccbaa->ccbaa_loc.loc_port != BCMCCBCF_PORT_DEFAULT)
		aprint_normal(" port %d", ccbaa->ccbaa_loc.loc_port);

	return QUIET;
}

__unused static inline uint32_t
bcmccb_read_4(struct bcmccb_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

__unused static inline void
bcmccb_write_4(struct bcmccb_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static const struct bcm_locators bcmccb_locators[] = {
	{ "bcmpwn", PWM_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT },
	{ "bcmmdio", MII_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT },
	{ "bcmrng", RNG_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 1, { IRQ_RNG } },
	{ "bcmtmr", TIMER0_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 2, { IRQ_TIMER0_1, IRQ_TIMER0_2 } },
	{ "bcmtmr", TIMER1_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 2, { IRQ_TIMER1_1, IRQ_TIMER1_2 } },
#ifdef SRAB_BASE
	{ "bcmsw", SRAB_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, },
#endif
	{ "bcmcom", UART2_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 1, { IRQ_UART2 } },
#ifdef BCM5301X
	{ "bcmi2c", SMBUS1_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 1, { IRQ_SMBUS1 } },
#endif
#ifdef BCM536XX
	{ "bcmi2c", SMBUS1_BASE, 0x1000, 1, 1, { IRQ_SMBUS1 } },
	{ "bcmi2c", SMBUS2_BASE, 0x1000, 2, 1, { IRQ_SMBUS2 } },
#endif
	{ "bcmcru", CRU_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT },
	{ "bcmdmu", DMU_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT },
	{ "bcmddr", DDR_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 1, { IRQ_DDR_CONTROLLER } },
	{ "bcmeth", GMAC0_BASE, 0x1000, 0, 1, { IRQ_GMAC0 }, },
	{ "bcmeth", GMAC1_BASE, 0x1000, 1, 1, { IRQ_GMAC1 }, },
#ifdef GMAC2_BASE
	{ "bcmeth", GMAC2_BASE, 0x1000, 2, 1, { IRQ_GMAC2 }, },
#endif
	// { "bcmeth", GMAC3_BASE, 0x1000, 3, 1, { IRQ_GMAC3 }, },
	{ "bcmpax", PCIE0_BASE, 0x1000, 0, 6, { IRQ_PCIE_INT0 }, },
	{ "bcmpax", PCIE1_BASE, 0x1000, 1, 6, { IRQ_PCIE_INT1 }, },
#ifdef PCIE2_BASE
	{ "bcmpax", PCIE2_BASE, 0x1000, 2, 6, { IRQ_PCIE_INT2 }, },
#endif
#ifdef SDIO_BASE
	{ "sdhc", SDIO_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 1, { IRQ_SDIO } },
#endif
	{ "bcmnand", NAND_BASE, 0x1000, BCMCCBCF_PORT_DEFAULT, 8,
	  { IRQ_NAND_RD_MISS, IRQ_NAND_BLK_ERASE_COMP,
	    IRQ_NAND_COPY_BACK_COMP, IRQ_NAND_PGM_PAGE_COMP,
	    IRQ_NAND_RO_CTLR_READY, IRQ_NAND_RB_B,
	    IRQ_NAND_ECC_MIPS_UNCORR, IRQ_NAND_ECC_MIPS_CORR } },
	{ "bcmusb", EHCI_BASE, 0x2000, BCMCCBCF_PORT_DEFAULT, 1, { IRQ_USB2 } },
};

static int
bcmccb_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;

	if (strcmp(cf->cf_name, ccbaa->ccbaa_loc.loc_name) != 0)
		return 0;

	const bool is_bcmeth_p = strcmp(cf->cf_name, "bcmeth") == 0;
	const bool is_bcmpax_p = !is_bcmeth_p && strcmp(cf->cf_name, "bcmpax") == 0;
	if (cf->cf_loc[BCMCCBCF_PORT] != BCMCCBCF_PORT_DEFAULT
	    && (cf->cf_loc[BCMCCBCF_PORT] != ccbaa->ccbaa_loc.loc_port
		|| (!is_bcmeth_p && !is_bcmpax_p)))
		return 0;

	ccbaa->ccbaa_loc.loc_mdio = cf->cf_loc[BCMCCBCF_MDIO];
	ccbaa->ccbaa_loc.loc_phy = cf->cf_loc[BCMCCBCF_PHY];
	if (!is_bcmeth_p) {
		const bool is_mdio_default_p = ccbaa->ccbaa_loc.loc_mdio == BCMCCBCF_MDIO_DEFAULT;
		const bool is_phy_default_p = ccbaa->ccbaa_loc.loc_phy == BCMCCBCF_PHY_DEFAULT;
		if (!is_mdio_default_p || !is_phy_default_p)
			return 0;
	}

	return config_match(parent, cf, ccbaa);
}

static void
bcmccb_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct bcmccb_softc * const sc = &bcmccb_sc;

	sc->sc_dev = self;
	self->dv_private = sc;

	sc->sc_bst = bcm53xx_ioreg_bst;

	bus_space_subregion (sc->sc_bst, bcm53xx_ioreg_bsh,
	    CCB_BASE, CCB_SIZE, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	for (size_t i = 0; i < __arraycount(bcmccb_locators); i++) {
		const struct bcm_locators *loc = &bcmccb_locators[i];

#ifdef BCM5301X
		if (strcmp(loc->loc_name, "bcmsw") == 0) {
			bcm53xx_srab_init();	// need this for ethernet.
		}
#endif

		struct bcmccb_attach_args ccbaa = {
			.ccbaa_ccb_bst = sc->sc_bst,
			.ccbaa_ccb_bsh = sc->sc_bsh,
			.ccbaa_dmat = &bcm53xx_dma_tag,
			.ccbaa_loc = *loc,
		};

		/*
		 * If the device might be in reset, let's try to take it
		 * out of it.  If it fails or is unavailable, skip it.
		 */
		if (!bcm53xx_idm_device_init(loc, sc->sc_bst, sc->sc_bsh))
			continue;

		cfdata_t cf = config_search_ia(bcmccb_find, self, "bcmccb",
		    &ccbaa);
		if (cf != NULL)
			config_attach(self, cf, &ccbaa, bcmccb_print);
	}
}
