/* $NetBSD: tegra_com.c,v 1.1.2.3 2015/06/06 14:39:56 skrll Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(1, "$NetBSD: tegra_com.c,v 1.1.2.3 2015/06/06 14:39:56 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/ic/comvar.h>

static int tegra_com_match(device_t, cfdata_t, void *);
static void tegra_com_attach(device_t, device_t, void *);

struct tegra_com_softc {
	struct com_softc tsc_sc;
	void *tsc_ih;
};

CFATTACH_DECL_NEW(tegra_com, sizeof(struct tegra_com_softc),
	tegra_com_match, tegra_com_attach, NULL, NULL);

static int
tegra_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	bus_space_tag_t bst = tio->tio_a4x_bst;
	bus_space_handle_t bsh;

	if (com_is_console(bst, TEGRA_APB_BASE + loc->loc_offset, NULL))
		return 1;

	bus_space_subregion(bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &bsh);

	return comprobe1(bst, bsh);
}

static void
tegra_com_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_com_softc * const tsc = device_private(self);
	struct com_softc * const sc = &tsc->tsc_sc;
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	bus_space_tag_t bst = tio->tio_a4x_bst;
	const bus_addr_t iobase = TEGRA_APB_BASE + loc->loc_offset;
	bus_space_handle_t bsh;

	sc->sc_dev = self;
	sc->sc_frequency = tegra_car_uart_rate(loc->loc_port);
	sc->sc_type = COM_TYPE_TEGRA;

	if (com_is_console(bst, iobase, &bsh) == 0
	    && bus_space_subregion(bst, tio->tio_bsh,
		loc->loc_offset / 4, loc->loc_size, &bsh)) {
		panic(": can't map registers");
	}
	COM_INIT_REGS(sc->sc_regs, bst, bsh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	tsc->tsc_ih = intr_establish(loc->loc_intr, IPL_SERIAL,
	    IST_LEVEL | IST_MPSAFE, comintr, sc);
	if (tsc->tsc_ih == NULL)
		panic("%s: failed to establish interrupt %d",
		    device_xname(self), loc->loc_intr);
}
