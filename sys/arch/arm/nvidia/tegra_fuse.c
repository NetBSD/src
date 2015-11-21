/* $NetBSD: tegra_fuse.c,v 1.1 2015/11/21 12:09:39 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_fuse.c,v 1.1 2015/11/21 12:09:39 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_fuse_match(device_t, cfdata_t, void *);
static void	tegra_fuse_attach(device_t, device_t, void *);

struct tegra_fuse_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

static struct tegra_fuse_softc *fuse_softc = NULL;

CFATTACH_DECL_NEW(tegra_fuse, sizeof(struct tegra_fuse_softc),
	tegra_fuse_match, tegra_fuse_attach, NULL, NULL);

static int
tegra_fuse_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_fuse_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_fuse_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	KASSERT(fuse_softc == NULL);
	fuse_softc = sc;

	aprint_naive("\n");
	aprint_normal(": FUSE\n");
}

uint32_t
tegra_fuse_read(u_int offset)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	if (fuse_softc) {
		bst = fuse_softc->sc_bst;
		bsh = fuse_softc->sc_bsh;
	} else {
		bst = &armv7_generic_bs_tag;
		bus_space_subregion(bst, tegra_apb_bsh,
		    TEGRA_FUSE_OFFSET, TEGRA_FUSE_SIZE, &bsh);
	}

	tegra_car_fuse_enable();
	const uint32_t v = bus_space_read_4(bst, bsh, offset);
	tegra_car_fuse_disable();

	return v;
}
