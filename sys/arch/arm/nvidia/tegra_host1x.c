/* $NetBSD: tegra_host1x.c,v 1.1 2015/05/18 20:36:42 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_host1x.c,v 1.1 2015/05/18 20:36:42 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_host1x_match(device_t, cfdata_t, void *);
static void	tegra_host1x_attach(device_t, device_t, void *);

struct tegra_host1x_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

CFATTACH_DECL_NEW(tegra_host1x, sizeof(struct tegra_host1x_softc),
	tegra_host1x_match, tegra_host1x_attach, NULL, NULL);

static int
tegra_host1x_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_host1x_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_host1x_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	if (bus_space_map(sc->sc_bst, TEGRA_HOST1X_BASE + loc->loc_offset,
	    loc->loc_size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map HOST1X\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": HOST1X\n");

	tegra_car_host1x_enable();
}
