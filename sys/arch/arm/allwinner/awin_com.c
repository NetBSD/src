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

__KERNEL_RCSID(1, "$NetBSD: awin_com.c,v 1.1 2013/09/04 02:39:01 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/ic/comvar.h>

static int awin_com_match(device_t, cfdata_t, void *);
static void awin_com_attach(device_t, device_t, void *);

struct awin_com_softc {
	struct com_softc asc_sc;
	void *asc_ih;
};

CFATTACH_DECL_NEW(awin_com, sizeof(struct awin_com_softc),
	awin_com_match, awin_com_attach, NULL, NULL);

static int awin_com_ports;

static int
awin_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];
	bus_space_tag_t iot = aio->aio_core_a4x_bst;
        bus_space_handle_t bsh;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

        KASSERT(loc->loc_offset >= AWIN_UART0_OFFSET);
	KASSERT(loc->loc_offset <= AWIN_UART7_OFFSET);
	KASSERT((loc->loc_offset & 0x3ff) == 0);
        KASSERT((awin_com_ports & __BIT(loc->loc_port)) == 0);

        if (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port)
                return 0;

        if (com_is_console(iot, AWIN_CORE_PBASE + loc->loc_offset, NULL))
                return 1;

        bus_space_subregion(iot, aio->aio_core_bsh,
            loc->loc_offset, loc->loc_size, &bsh);

        return comprobe1(iot, bsh);
}

static void
awin_com_attach(device_t parent, device_t self, void *aux)
{
	struct awin_com_softc * const asc = device_private(self);
	struct com_softc * const sc = &asc->asc_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	bus_space_tag_t iot = aio->aio_core_a4x_bst;
	const bus_addr_t iobase = AWIN_CORE_PBASE + loc->loc_offset;
	bus_space_handle_t ioh;

        awin_com_ports |= __BIT(loc->loc_port);

	sc->sc_dev = self;
	sc->sc_frequency = AWIN_UART_FREQ;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(iot, iobase, &ioh) == 0
	    && bus_space_subregion(iot, aio->aio_core_bsh,
		loc->loc_offset / 4, loc->loc_size, &ioh)) {
		panic(": can't map registers");
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	KASSERT(loc->loc_intr != AWINIO_INTR_DEFAULT);
	asc->asc_ih = intr_establish(loc->loc_intr, IPL_SERIAL, IST_LEVEL,
	    comintr, sc);
	if (asc->asc_ih == NULL)
		panic("%s: failed to establish interrupt %d",
		    device_xname(self), loc->loc_intr);
}
