/* $NetBSD: latches.c,v 1.2.2.2 2001/04/23 09:41:36 bouyer Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: latches.c,v 1.2.2.2 2001/04/23 09:41:36 bouyer Exp $");

#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <arch/arm26/iobus/iocvar.h>
#include <arch/arm26/ioc/latchreg.h>
#include <arch/arm26/ioc/latchvar.h>
#include <arch/arm26/ioc/ioebvar.h>

#include "ioeb.h"

struct latches_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	u_int8_t	sc_latcha;
	u_int8_t	sc_latchb;
};

static int latches_match(struct device *, struct cfdata *, void *);
static void latches_attach(struct device *, struct device *, void *);

struct cfattach latches_ca = {
	sizeof(struct latches_softc), latches_match, latches_attach
};

struct device *the_latches;

static int
latches_match(struct device *parent, struct cfdata *cf, void *aux)
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
latches_attach(struct device *parent, struct device *self, void *aux)
{
	struct latches_softc *sc = (void *)self;
	struct ioc_attach_args *ioc = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	if (the_latches == NULL)
		the_latches = self;
	iot = sc->sc_iot = ioc->ioc_fast_t;
	ioh = sc->sc_ioh = ioc->ioc_fast_h;

	sc->sc_latcha =
	    LATCHA_NSEL0 | LATCHA_NSEL1 | LATCHA_NSEL2 | LATCHA_NSEL3 |
	    LATCHA_NSIDE1 | LATCHA_NMOTORON | LATCHA_NINUSE;
	sc->sc_latchb = LATCHB_NFDCR | LATCHB_NPSTB;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_A, sc->sc_latcha);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_B, sc->sc_latchb);
	printf("\n");
}

void
latcha_update(u_int8_t mask, u_int8_t value)
{
	struct latches_softc *sc = (void *)the_latches;

	sc->sc_latcha = (sc->sc_latcha & ~mask) | value;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_A, sc->sc_latcha);
}	

void
latchb_update(u_int8_t mask, u_int8_t value)
{
	struct latches_softc *sc = (void *)the_latches;

	sc->sc_latchb = (sc->sc_latchb & ~mask) | value;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LATCH_B, sc->sc_latcha);
}	
