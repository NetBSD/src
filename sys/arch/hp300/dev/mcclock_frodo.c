/*	$NetBSD: mcclock_frodo.c,v 1.1.8.2 2014/08/10 06:53:57 tls Exp $	*/
/*-
 * Copyright (c) 2014 Izumi Tsutsui.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock_frodo.c,v 1.1.8.2 2014/08/10 06:53:57 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/cpu.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <hp300/dev/frodoreg.h>
#include <hp300/dev/frodovar.h>

#include "ioconf.h"

static int	mcclock_frodo_probe(device_t, cfdata_t, void *);
static void	mcclock_frodo_attach(device_t, device_t, void *);

static u_int	mcclock_frodo_read(struct mc146818_softc *, u_int);
static void	mcclock_frodo_write(struct mc146818_softc *, u_int, u_int);

CFATTACH_DECL_NEW(mcclock_frodo, sizeof(struct mc146818_softc),
    mcclock_frodo_probe, mcclock_frodo_attach, NULL, NULL);

#define	MCCLOCK_FRODO_NPORTS	2

static int
mcclock_frodo_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct frodo_attach_args *fa = aux;

	/* only 425e has a valid mcclock */
	if (machineid != HP_425 || mmuid != MMUID_425_E)
		return 0;

	if (strcmp(fa->fa_name, mcclock_cd.cd_name) != 0)
		return 0;

	if (fa->fa_offset != FRODO_CALENDAR)
		return 0;

	return 1;
}

static void
mcclock_frodo_attach(device_t parent, device_t self, void *aux)
{
	struct mc146818_softc *sc = device_private(self);
	struct frodo_attach_args *fa = aux;

	sc->sc_dev = self;
	sc->sc_bst = fa->fa_bst;

	if (bus_space_map(sc->sc_bst, fa->fa_base + fa->fa_offset,
	    MCCLOCK_FRODO_NPORTS << 2, 0, &sc->sc_bsh) != 0) {
		aprint_error(": can't map register\n");
		return;
	}

	sc->sc_year0 = 1900;
	sc->sc_mcread = mcclock_frodo_read;
	sc->sc_mcwrite = mcclock_frodo_write;
	sc->sc_flag = 0;
	mc146818_attach(sc);

	aprint_normal("\n");

	/* XXX: not sure which mode (BINARY or BCD) is used on HP-UX */
	mcclock_frodo_write(sc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

	/* make sure to start the 32.768kHz OSC */
	mcclock_frodo_write(sc, MC_REGA, 
	    (mcclock_frodo_read(sc, MC_REGA) & ~MC_REGA_DVMASK) |
	    MC_BASE_32_KHz);
}

/*
 * mc146818 calendar chip in the frodo utility chip is can be accessed
 * via register port at offset 0x80 and data port at offset 0x84.
 *
 * Note 4 byte offset stride is handled by frodo bus_space(9) layer,
 * so bus_space_read(9) and bus_space_write(9) takes 0 or 1 as offset
 * to access these registers.
 */

static void
mcclock_frodo_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

static u_int
mcclock_frodo_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int datum;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	datum = bus_space_read_1(iot, ioh, 1);

	return datum;
}
