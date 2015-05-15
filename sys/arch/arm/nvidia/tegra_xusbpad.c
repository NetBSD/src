/* $NetBSD: tegra_xusbpad.c,v 1.1 2015/05/15 11:49:10 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_xusbpad.c,v 1.1 2015/05/15 11:49:10 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_xusbpadreg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_xusbpad_match(device_t, cfdata_t, void *);
static void	tegra_xusbpad_attach(device_t, device_t, void *);

struct tegra_xusbpad_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

static struct tegra_xusbpad_softc *xusbpad_softc = NULL;

CFATTACH_DECL_NEW(tegra_xusbpad, sizeof(struct tegra_xusbpad_softc),
	tegra_xusbpad_match, tegra_xusbpad_attach, NULL, NULL);

static int
tegra_xusbpad_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_xusbpad_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_xusbpad_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	KASSERT(xusbpad_softc == NULL);
	xusbpad_softc = sc;

	aprint_naive("\n");
	aprint_normal(": XUSB PADCTL\n");

}

static void
tegra_xusbpad_get_bs(bus_space_tag_t *pbst, bus_space_handle_t *pbsh)
{
	if (xusbpad_softc) {
		*pbst = xusbpad_softc->sc_bst;
		*pbsh = xusbpad_softc->sc_bsh;
	} else {
		*pbst = &armv7_generic_bs_tag;
		bus_space_subregion(*pbst, tegra_apb_bsh,
		    TEGRA_XUSB_PADCTL_OFFSET, TEGRA_XUSB_PADCTL_SIZE, pbsh);
	}
}

void
tegra_xusbpad_sata_enable(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	int retry;

	tegra_xusbpad_get_bs(&bst, &bsh);

	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_USB3_PAD_MUX_REG,
	    __SHIFTIN(XUSB_PADCTL_USB3_PAD_MUX_SATA_PAD_LANE0_SATA,
		      XUSB_PADCTL_USB3_PAD_MUX_SATA_PAD_LANE0) |
	    XUSB_PADCTL_USB3_PAD_MUX_FORCE_SATA_PAD_IDDQ_DISABLE_MASK0,
	    XUSB_PADCTL_USB3_PAD_MUX_SATA_PAD_LANE0);

	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_REG,
	    0,
	    XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ |
	    XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ_OVRD);
	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG,
	    0,
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_IDDQ |
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PWR_OVRD);
	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG,
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_MODE, 0);
	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG,
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_RST, 0);

	for (retry = 1000; retry > 0; retry--) {
		const uint32_t v = bus_space_read_4(bst, bsh,
		    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG);
		if (v & XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_LOCKDET)
			break;
		delay(100);
	}
	if (retry == 0) {
		printf("WARNING: SATA PHY power-on failed\n");
	}
}
