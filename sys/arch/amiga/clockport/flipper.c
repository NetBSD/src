/*      $NetBSD: flipper.c,v 1.1.4.2 2013/01/16 05:32:40 yamt Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/* Driver for Individual Computers Delfina Flipper / Petsoff Delfina 1200. 
 *
 * TODO:
 * - linux-style /dev/dsp56k interface
 * - audio
 * - firmware
 * - interrupts: caa->cp_intr_establish(dspintr, sc);
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <amiga/clockport/clockportvar.h>

#include <amiga/clockport/flipperreg.h>
#include <amiga/clockport/flippervar.h>

#define FLIPPER_DEBUG 1

static int flipper_probe(device_t, cfdata_t , void *);
static void flipper_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(flipper, sizeof(struct flipper_softc),
    flipper_probe, flipper_attach, NULL, NULL);

static int
flipper_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct clockport_attach_args *caa = aux;
	uint8_t delfinaver;
	bus_space_handle_t ioh;

	bus_space_map(caa->cp_iot, 0, FLIPPER_REGSIZE, 0, &ioh);

	delfinaver = bus_space_read_1(caa->cp_iot, ioh, FLIPPER_HOSTCTL);
#ifdef FLIPPER_DEBUG
	aprint_normal("flipper: hostctl probe read %x\n", delfinaver);
#endif /* FLIPPER_DEBUG */

	bus_space_unmap(caa->cp_iot, ioh, FLIPPER_REGSIZE);

	if ((delfinaver == 0xB5) || (delfinaver == 0xB6))
		return 1;

	return 0; 
}

static void
flipper_attach(device_t parent, device_t self, void *aux)
{
	struct flipper_softc *sc = device_private(self);
	struct clockport_attach_args *caa = aux;
	sc->sc_dev = self;
	sc->sc_iot = caa->cp_iot;

	if (bus_space_map(sc->sc_iot, 0, FLIPPER_REGSIZE, 0, &sc->sc_ioh)) {
		aprint_normal("can't map the bus space\n");
		return;
	}

	sc->sc_delfinaver = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 
	    FLIPPER_HOSTCTL);

	switch (sc->sc_delfinaver) {
	case DELFINA_FLIPPER:
		aprint_normal(": Individual Computers Delfina Flipper\n");
		break;
	case DELFINA_1200:
		aprint_normal(": Petsoff Delfina 1200\n");
		break;
	default:
		aprint_normal(": unknown model\n");
		return;
	}

	/* reset the board */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, FLIPPER_HOSTCTL,
	    FLIPPER_HOSTCTL_RESET | FLIPPER_HOSTCTL_IRQDIS);
	delay(10000);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, FLIPPER_HOSTCTL, 
	    FLIPPER_HOSTCTL_IRQDIS); /* leave interrupts disabled for now */

	/* attach dsp56k, audio, etc. */
}

