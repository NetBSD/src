/* $NetBSD: amlogic_com.c,v 1.1 2015/02/07 17:20:17 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: amlogic_com.c,v 1.1 2015/02/07 17:20:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>

#include <dev/ic/comvar.h>

static int amlogic_com_match(device_t, cfdata_t, void *);
static void amlogic_com_attach(device_t, device_t, void *);

struct amlogic_com_softc {
	struct com_softc asc_sc;
	void *asc_ih;
};

CFATTACH_DECL_NEW(amlogic_com, sizeof(struct amlogic_com_softc),
	amlogic_com_match, amlogic_com_attach, NULL, NULL);

static int amlogic_com_ports;

static int
amlogic_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	bus_space_tag_t iot = aio->aio_core_a4x_bst;
	bus_space_handle_t bsh;

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT((amlogic_com_ports & __BIT(loc->loc_port)) == 0);
	KASSERT(cf->cf_loc[AMLOGICIOCF_PORT] == AMLOGICIOCF_PORT_DEFAULT
	    || cf->cf_loc[AMLOGICIOCF_PORT] == loc->loc_port);

	if (com_is_console(iot, AMLOGIC_CORE_BASE + loc->loc_offset, NULL))
		return 1;

	bus_space_subregion(iot, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &bsh);

	const int rv = comprobe1(iot, bsh);

	return rv;
}

static void
amlogic_com_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_com_softc * const asc = device_private(self);
	struct com_softc * const sc = &asc->asc_sc;
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	bus_space_tag_t iot = aio->aio_core_a4x_bst;
	const bus_addr_t iobase = AMLOGIC_CORE_BASE + loc->loc_offset;
	bus_space_handle_t ioh;

	amlogic_com_ports |= __BIT(loc->loc_port);

	sc->sc_dev = self;
	sc->sc_frequency = AMLOGIC_UART_FREQ;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(iot, iobase, &ioh) == 0
	    && bus_space_subregion(iot, aio->aio_bsh,
		loc->loc_offset / 4, loc->loc_size, &ioh)) {
		panic(": can't map registers");
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	KASSERT(loc->loc_intr != AMLOGICIO_INTR_DEFAULT);
	asc->asc_ih = intr_establish(loc->loc_intr, IPL_SERIAL,
	    IST_EDGE | IST_MPSAFE, comintr, sc);
	if (asc->asc_ih == NULL)
		panic("%s: failed to establish interrupt %d",
		    device_xname(self), loc->loc_intr);
}
