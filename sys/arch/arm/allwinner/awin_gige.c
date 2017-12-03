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

#include "axp806pm.h"
#include "axp809pm.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_gige.c,v 1.4.12.3 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#if NAXP806PM > 0
#include <dev/i2c/axp806.h>
#endif
#if NAXP809PM > 0
#include <dev/i2c/axp809.h>
#endif

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>
#include <dev/ic/dwc_gmac_reg.h>

static int awin_gige_match(device_t, cfdata_t, void *);
static void awin_gige_attach(device_t, device_t, void *);
static void awin_gige_pmu_init(device_t);
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
	'A', AWIN_A31_PIO_PA_GMAC_FUNC, AWIN_A31_PIO_PA_GMAC_PINS, 0, 3
};

static const struct awin_gpio_pinset awin_gige_gpio_pinset_a80 = {
	'A', AWIN_A80_PIO_PA_GMAC_FUNC, AWIN_A80_PIO_PA_GMAC_PINS, 0, 3
};

CFATTACH_DECL_NEW(awin_gige, sizeof(struct awin_gige_softc),
	awin_gige_match, awin_gige_attach, NULL, NULL);

static int
awin_gige_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct awin_gpio_pinset *pinset =
	    awin_chip_id() == AWIN_CHIP_ID_A31 ?
	    &awin_gige_gpio_pinset_a31 : &awin_gige_gpio_pinset;
	struct awinio_attach_args * const aio __diagused = aux;
	const struct awin_locators * const loc __diagused = &aio->aio_loc;
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
	struct awin_gpio_pinset pinset;
	prop_dictionary_t cfg = device_properties(self);
	uint32_t clkreg;
	const char *phy_type, *pin_name;
	bus_space_handle_t bsh;

	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A80:
		bsh = aio->aio_a80_core2_bsh;
		pinset = awin_gige_gpio_pinset_a80;
		break;
	case AWIN_CHIP_ID_A31:
		bsh = aio->aio_core_bsh;
		pinset = awin_gige_gpio_pinset_a31;
		break;
	default:
		bsh = aio->aio_core_bsh;
		pinset = awin_gige_gpio_pinset;
		break;
	}

	sc->sc_core.sc_dev = self;

	prop_dictionary_get_uint8(cfg, "pinset-func", &pinset.pinset_func);
	awin_gpio_pinset_acquire(&pinset);

	sc->sc_core.sc_bst = aio->aio_core_bst;
	sc->sc_core.sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_core.sc_bst, bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_core.sc_bsh);

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	awin_gige_pmu_init(self);

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
	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING1_REG,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING1_GMAC, 0);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING0_REG, AWIN_A31_AHB_GATING0_GMAC, 0);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_GMAC, 0);
	}

	/*
	 * Soft reset
	 */
	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST1_REG,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST1_GMAC, 0);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET0_REG,
		    AWIN_A31_AHB_RESET0_GMAC_RST, 0);
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
	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_a80_core2_bsh,
		    AWIN_A80_SYS_CTRL_OFFSET + AWIN_A80_SYS_CTRL_EMAC_CLK_REG,
		    clkreg, AWIN_GMAC_CLK_PIT|AWIN_GMAC_CLK_TCS);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A31) {
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

static void
awin_gige_pmu_init(device_t dev)
{
	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
#if NAXP806PM > 0
		device_t axp806 = device_find_by_driver_unit("axp806pm", 0);
		if (axp806) {
			struct axp806_ctrl *c = axp806_lookup(axp806, "CLDO1");
			if (c) {
				axp806_set_voltage(c, 3000, 3000);
				axp806_enable(c);
				delay(3000);
			}
		}
#endif
#if NAXP809PM > 0
		device_t axp809 = device_find_by_driver_unit("axp809pm", 0);
		if (axp809) {
			struct axp809_ctrl *c = axp809_lookup(axp809, "GPIO1");
			if (c) {
				axp809_enable(c);
				delay(3000);
			}
		}
#endif
		delay(100000);
	}
}
