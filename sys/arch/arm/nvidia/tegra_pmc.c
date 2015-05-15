/* $NetBSD: tegra_pmc.c,v 1.4 2015/05/15 11:50:30 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_pmc.c,v 1.4 2015/05/15 11:50:30 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_pmc_match(device_t, cfdata_t, void *);
static void	tegra_pmc_attach(device_t, device_t, void *);

struct tegra_pmc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

static struct tegra_pmc_softc *pmc_softc = NULL;

CFATTACH_DECL_NEW(tegra_pmc, sizeof(struct tegra_pmc_softc),
	tegra_pmc_match, tegra_pmc_attach, NULL, NULL);

static int
tegra_pmc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_pmc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_pmc_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	KASSERT(pmc_softc == NULL);
	pmc_softc = sc;

	aprint_naive("\n");
	aprint_normal(": PMC\n");
}

static void
tegra_pmc_get_bs(bus_space_tag_t *pbst, bus_space_handle_t *pbsh)
{
	if (pmc_softc) {
		*pbst = pmc_softc->sc_bst;
		*pbsh = pmc_softc->sc_bsh;
	} else {
		*pbst = &armv7_generic_bs_tag;
		bus_space_subregion(*pbst, tegra_apb_bsh,
		    TEGRA_PMC_OFFSET, TEGRA_PMC_SIZE, pbsh);
	}
}

void
tegra_pmc_reset(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t cntrl;

	tegra_pmc_get_bs(&bst, &bsh);

	cntrl = bus_space_read_4(bst, bsh, PMC_CNTRL_0_REG);
	cntrl |= PMC_CNTRL_0_MAIN_RST;
	bus_space_write_4(bst, bsh, PMC_CNTRL_0_REG, cntrl);

	for (;;) {
		__asm("wfi");
	}
}

void
tegra_pmc_power(u_int partid, bool enable)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	uint32_t status;
	bool state;

	tegra_pmc_get_bs(&bst, &bsh);

	status = bus_space_read_4(bst, bsh, PMC_PWRGATE_STATUS_0_REG);
	state = !!(status & __BIT(partid));
	if (state == enable)
		return;

	bus_space_write_4(bst, bsh, PMC_PWRGATE_TOGGLE_0_REG,
	    __SHIFTIN(partid, PMC_PWRGATE_TOGGLE_0_PARTID) |
	    PMC_PWRGATE_TOGGLE_0_START);
}

void
tegra_pmc_remove_clamping(u_int partid)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	tegra_pmc_get_bs(&bst, &bsh);

	bus_space_write_4(bst, bsh, PMC_REMOVE_CLAMPING_CMD_0_REG,
	    __BIT(partid));
}
