/*      $NetBSD: com_ss.c,v 1.1 2012/04/17 09:59:03 rkujawa Exp $ */

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

/* Driver for Individual Computers SilverSurfer. */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/device.h>
#include <sys/termios.h> 

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <amiga/clockport/clockportvar.h>

#define COM_SS_REGS	8

static int com_ss_probe(device_t, cfdata_t , void *);
static void com_ss_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_ss, sizeof(struct com_softc),
    com_ss_probe, com_ss_attach, NULL, NULL);

static int
com_ss_probe(device_t parent, cfdata_t cf, void *aux)
{
	return 1; /* TODO: implement probe */
}

static void
com_ss_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc *sc = device_private(self);
	struct clockport_attach_args *caa = aux;

	bus_space_handle_t ioh;

	sc->sc_dev = self;
	sc->sc_frequency = COM_FREQ;

	bus_space_map(caa->cp_iot, 0, COM_SS_REGS, 0, &ioh);

	COM_INIT_REGS(sc->sc_regs, caa->cp_iot, ioh, 0 /* off */);

	com_attach_subr(sc);

	caa->cp_intr_establish(comintr, sc);
}

