/* $NetBSD: latches.c,v 1.8.2.1 2014/08/20 00:02:40 tls Exp $ */

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: latches.c,v 1.8.2.1 2014/08/20 00:02:40 tls Exp $");

#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/ioc/latchreg.h>
#include <arch/acorn26/ioc/latchvar.h>
#include <arch/acorn26/ioc/ioebvar.h>

#include "ioeb.h"

struct latches_softc {
	device_t	sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	uint8_t		sc_latcha;
	uint8_t		sc_latchb;
};

static int latches_match(device_t, cfdata_t, void *);
static void latches_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(latches, sizeof(struct latches_softc),
    latches_match, latches_attach, NULL, NULL);

device_t the_latches;

static int
latches_match(device_t parent, cfdata_t cf, void *aux)
{

	/*
	 * Latches are write-only, so we can't probe for them.
	 * Happily, the set of machines they exist on is precisely the
	 * set that doesn't have IOEBs, so...
	 */
#if NIOEB > 0
	if (the_ioeb != NULL)
		return 0;
#endif
	return 1;
}

static void
latches_attach(device_t parent, device_t self, void *aux)
{
	struct latches_softc *sc = device_private(self);
	struct ioc_attach_args *ioc = aux;

	sc->sc_dev = self;
	if (the_latches == NULL)
		the_latches = self;
	sc->sc_iot = ioc->ioc_fast_t;
	sc->sc_ioh = ioc->ioc_fast_h;

	sc->sc_latcha =
	    LATCHA_NSEL0 | LATCHA_NSEL1 | LATCHA_NSEL2 | LATCHA_NSEL3 |
	    LATCHA_NSIDE1 | LATCHA_NMOTORON | LATCHA_NINUSE;
	sc->sc_latchb = LATCHB_NFDCR | LATCHB_NPSTB;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_A, sc->sc_latcha);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_B, sc->sc_latchb);
	aprint_normal("\n");
}

void
latcha_update(uint8_t mask, uint8_t value)
{
	struct latches_softc *sc = device_private(the_latches);

	sc->sc_latcha = (sc->sc_latcha & ~mask) | value;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_A, sc->sc_latcha);
}	

void
latchb_update(uint8_t mask, uint8_t value)
{
	struct latches_softc *sc = device_private(the_latches);

	sc->sc_latchb = (sc->sc_latchb & ~mask) | value;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_B, sc->sc_latcha);
}	
