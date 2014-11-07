/*-
 * Copyright (c) 2013, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Martin Husemann.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_gige.c,v 1.18 2014/11/07 11:42:28 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>
#include <dev/ic/dwc_gmac_reg.h>

static int awin_gige_match(device_t, cfdata_t, void *);
static void awin_gige_attach(device_t, device_t, void *);
static int awin_gige_intr(void*);

struct awin_gige_softc {
	struct dwc_gmac_softc sc_core;
	void *sc_ih;
	struct awin_gpio_pindata sc_power_pin;
};

static const struct awin_gpio_pinset awin_gige_gpio_pinset = {
	'A', AWIN_PIO_PA_GMAC_FUNC, AWIN_PIO_PA_GMAC_PINS,
};

static const struct awin_gpio_pinset awin_gige_gpio_pinset_a31 = {
	'A', AWIN_A31_PIO_PA_GMAC_FUNC, AWIN_A31_PIO_PA_GMAC_PINS,
};


CFATTACH_DECL_NEW(awin_gige, sizeof(struct awin_gige_softc),
	awin_gige_match, awin_gige_attach, NULL, NULL);

static int
awin_gige_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_gpio_pinset *pinset =
	    awin_chip_id() == AWIN_CHIP_ID_A31 ?
	    &awin_gige_gpio_pinset_a31 : &awin_gige_gpio_pinset;
#ifdef DIAGNOSTIC
	const struct awin_locators * const loc = &aio->aio_loc;
#endif
	if (cf->cf_flags & 1)
		return 0;

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);

	if (!awin_gpio_pinset_available(pinset))
		return 0;

	return 1;
}

static void
awin_gige_attach(device_t parent, device_t self, void *aux)
{
	struct awin_gige_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	struct awin_gpio_pinset pinset =
	    awin_chip_id() == AWIN_CHIP_ID_A31 ?
	    awin_gige_gpio_pinset_a31 : awin_gige_gpio_pinset;
	prop_dictionary_t cfg = device_properties(self);
	uint32_t clkreg;
	const char *phy_type, *pin_name;

	sc->sc_core.sc_dev = self;

	prop_dictionary_get_uint8(cfg, "pinset-func", &pinset.pinset_func);
	awin_gpio_pinset_acquire(&pinset);

	sc->sc_core.sc_bst = aio->aio_core_bst;
	sc->sc_core.sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_core.sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_core.sc_bsh);

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	/*
	 * Interrupt handler
	 */
	sc->sc_ih = intr_establish(loc->loc_intr, IPL_NET, IST_LEVEL,
	    awin_gige_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     loc->loc_intr);

	if (prop_dictionary_get_cstring_nocopy(cfg, "phy-power", &pin_name)) {
		if (awin_gpio_pin_reserve(pin_name, &sc->sc_power_pin)) {
			awin_gpio_pindata_write(&sc->sc_power_pin, 1);
		} else {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		}
	}

	/*
	 * Enable GMAC clock
	 */
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING0_REG, AWIN_A31_AHB_GATING0_GMAC, 0);
	} else {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_GMAC, 0);
	}

	/*
	 * Soft reset
	 */
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET0_REG, AWIN_A31_AHB_RESET0_GMAC_RST, 0);
	}

	/*
	 * PHY clock setup
	 */
	if (!prop_dictionary_get_cstring_nocopy(cfg, "phy-type", &phy_type))
		phy_type = "rgmii";
	if (strcmp(phy_type, "rgmii") == 0) {
		clkreg = AWIN_GMAC_CLK_PIT | AWIN_GMAC_CLK_TCS_INT_RGMII;
	} else if (strcmp(phy_type, "rgmii-bpi") == 0) {
		clkreg = AWIN_GMAC_CLK_PIT | AWIN_GMAC_CLK_TCS_INT_RGMII;
		/*
		 * These magic bits seem to be necessary for RGMII at gigabit
		 * speeds on Banana Pi.
		 */
		clkreg |= __BITS(11,10);
	} else if (strcmp(phy_type, "gmii") == 0) {
		clkreg = AWIN_GMAC_CLK_TCS_INT_RGMII;
	} else if (strcmp(phy_type, "mii") == 0) {
		clkreg = AWIN_GMAC_CLK_TCS_MII;
	} else {
		panic("unknown phy type '%s'", phy_type);
	}
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_GMAC_CLK_REG, clkreg,
		    AWIN_GMAC_CLK_PIT|AWIN_GMAC_CLK_TCS);
	} else {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_GMAC_CLK_REG, clkreg,
		    AWIN_GMAC_CLK_PIT|AWIN_GMAC_CLK_TCS);
	}

	dwc_gmac_attach(&sc->sc_core, GMAC_MII_CLK_150_250M_DIV102);
}

static int
awin_gige_intr(void *arg)
{
	struct awin_gige_softc *sc = arg;

	return dwc_gmac_intr(&sc->sc_core);
}
