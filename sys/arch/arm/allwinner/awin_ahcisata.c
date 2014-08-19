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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_ahcisata.c,v 1.11.12.2 2014/08/20 00:02:44 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

static int awin_ahci_match(device_t, cfdata_t, void *);
static void awin_ahci_attach(device_t, device_t, void *);

struct awin_ahci_softc {
	struct ahci_softc asc_sc;
	struct awin_gpio_pindata asc_gpio_pin;
	void *asc_ih;
};

CFATTACH_DECL_NEW(awin_ahcisata, sizeof(struct awin_ahci_softc),
	awin_ahci_match, awin_ahci_attach, NULL, NULL);

static int
awin_ahci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	if (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port)
		return 0;

	return 1;
}

static void
awin_ahci_phy_init(struct awin_ahci_softc *asc)
{
	bus_space_tag_t bst = asc->asc_sc.sc_ahcit;
	bus_space_handle_t bsh = asc->asc_sc.sc_ahcih;
	u_int timeout;
	uint32_t v;

	/*
	 * This is dark magic.
	 */
	delay(5000);
	bus_space_write_4(bst, bsh, AWIN_AHCI_RWCR_REG, 0);

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS1R_REG, __BIT(19), 0);

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS0R_REG,
	    __BIT(26)|__BIT(24)|__BIT(23)|__BIT(18),
	    __BIT(25));

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS1R_REG,
	    __BIT(17)|__BIT(10)|__BIT(9)|__BIT(7),
	    __BIT(16)|__BIT(12)|__BIT(11)|__BIT(8)|__BIT(6));

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS1R_REG,
	    __BIT(28)|__BIT(15), 0);

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS1R_REG, 0, __BIT(19));

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS0R_REG,
	    __BIT(21)|__BIT(20), __BIT(22));

	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS2R_REG,
	    __BIT(9)|__BIT(8)|__BIT(5), __BIT(7)|__BIT(6));

	delay(10);
	awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS0R_REG, __BIT(19), 0);

	timeout = 1000;
	do {
		delay(1);
		v = bus_space_read_4(bst, bsh, AWIN_AHCI_PHYCS0R_REG);
	} while (--timeout && __SHIFTOUT(v, __BITS(30,28)) != 2);

	if (!timeout) {
		aprint_error_dev(
		    asc->asc_sc.sc_atac.atac_dev,
		    "SATA PHY power failed (%#x)\n", v);
	} else {

		awin_reg_set_clear(bst, bsh, AWIN_AHCI_PHYCS2R_REG,
		    __BIT(24), 0);
		timeout = 1000;
		do {
			delay(10);
			v = bus_space_read_4(bst, bsh, AWIN_AHCI_PHYCS2R_REG);
		} while (--timeout && (v & __BIT(24)));

		if (!timeout) {
			aprint_error_dev(
			    asc->asc_sc.sc_atac.atac_dev,
			    "SATA PHY calibration failed (%#x)\n", v);
		}
	}
	delay(10);
	bus_space_write_4(bst, bsh, AWIN_AHCI_RWCR_REG, 7);
}

static void
awin_ahci_enable(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	/*
	 * SATA needs PLL6 to be a 100MHz clock.
	 */
	awin_pll6_enable();

	/*
	 * Make sure it's enabled for the AHB.
	 */
	awin_reg_set_clear(bst, bsh, AWIN_AHB_GATING0_REG,
	    AWIN_AHB_GATING0_SATA, 0);
	delay(1000);

	/*
	 * Now turn it on (forcing it to use PLL6).
	 */
	bus_space_write_4(bst, bsh, AWIN_SATA_CLK_REG, AWIN_CLK_ENABLE);
}

static void
awin_ahci_channel_start(struct ahci_softc *sc, struct ata_channel *chp)
{
	bus_size_t dma_reg = AHCI_P_AWIN_DMA(chp->ch_channel);

	uint32_t dma = AHCI_READ(sc, dma_reg);
	dma &= ~0xff00;
	dma |= 0x4400;
	AHCI_WRITE(sc, dma_reg, dma);
}

static void
awin_ahci_attach(device_t parent, device_t self, void *aux)
{
	struct awin_ahci_softc * const asc = device_private(self);
	struct ahci_softc * const sc = &asc->asc_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	awin_ahci_enable(aio->aio_core_bst, aio->aio_ccm_bsh);

        sc->sc_atac.atac_dev = self;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_ahcit = aio->aio_core_bst;
	sc->sc_ahcis = loc->loc_size;
	sc->sc_ahci_ports = 1;
	sc->sc_ahci_quirks = AHCI_QUIRK_BADPMP;
	sc->sc_save_init_data = true;
	sc->sc_channel_start = awin_ahci_channel_start;

	bus_space_subregion(aio->aio_core_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_ahcih);

	aprint_naive(": AHCI SATA controller\n");
	aprint_normal(": AHCI SATA controller\n");

	/*
	 * Bring up the PHY.
	 */
	awin_ahci_phy_init(asc);

	/*
	 * If there is a GPIO to turn on power, do it now.
	 */
	const char *pin_name;
	prop_dictionary_t dict = device_properties(self);
	if (prop_dictionary_get_cstring_nocopy(dict, "power-gpio", &pin_name)) {
		if (awin_gpio_pin_reserve(pin_name, &asc->asc_gpio_pin)) {
			awin_gpio_pindata_write(&asc->asc_gpio_pin, 1);
		} else {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		}
	}

	/*
	 * Establish the interrupt
	 */
	asc->asc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_LEVEL,
	    ahci_intr, sc);
	if (asc->asc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intr);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	ahci_attach(sc);

	return;

fail:
	if (asc->asc_ih) {
		intr_disestablish(asc->asc_ih);
		asc->asc_ih = NULL;
	}
}
