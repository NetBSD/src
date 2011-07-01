/*	$NetBSD: mcclock.c,v 1.5 2011/07/01 20:36:42 dyoung Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock.c,v 1.5 2011/07/01 20:36:42 dyoung Exp $");

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#define	MCCLOCK_NPORTS	2

static int mcclock_match(device_t, cfdata_t, void *);
static void mcclock_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcclock, sizeof (struct mc146818_softc),
    mcclock_match, mcclock_attach, NULL, NULL);

static void mcclock_write(struct mc146818_softc *, u_int, u_int);
static u_int mcclock_read(struct mc146818_softc *, u_int);


static int
mcclock_match(device_t parent, cfdata_t cf, void *aux)
{
	static int mcclock_found;

	if (mcclock_found)
		return 0;

	mcclock_found = 1;

	return 1;
}

static void
mcclock_attach(device_t parent, device_t self, void *aux)
{
	struct mc146818_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;

	sc->sc_dev = self;
	sc->sc_bst = ma->ma_iot;
	if (bus_space_map(sc->sc_bst, ma->ma_addr, MCCLOCK_NPORTS,
	    0, &sc->sc_bsh)) {
		aprint_error("mcclock_attach: unable to map registers\n");
		return;
	}

	sc->sc_year0 = 2000;
	sc->sc_mcread = mcclock_read;
	sc->sc_mcwrite = mcclock_write;
	sc->sc_flag = MC146818_BCD;
	mc146818_attach(sc);

	aprint_normal("\n");

	(*sc->sc_mcwrite)(sc, MC_REGB, MC_REGB_24HR);
}

static void
mcclock_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

static u_int
mcclock_read(struct mc146818_softc *sc, u_int reg)
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
