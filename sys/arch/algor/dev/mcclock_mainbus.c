/*	$NetBSD: mcclock_mainbus.c,v 1.5.22.2 2008/01/21 09:35:06 yamt Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock_mainbus.c,v 1.5.22.2 2008/01/21 09:35:06 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

int	mcclock_mainbus_match(struct device *, struct cfdata *, void *);
void	mcclock_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mcclock_mainbus, sizeof(struct mc146818_softc),
    mcclock_mainbus_match, mcclock_mainbus_attach, NULL, NULL);

void	mcclock_mainbus_write(struct mc146818_softc *, u_int, u_int);
u_int	mcclock_mainbus_read(struct mc146818_softc *, u_int);

int
mcclock_mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
mcclock_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct mc146818_softc *sc = (struct mc146818_softc *)self;

	sc->sc_bst = ma->ma_st;
	if (bus_space_map(sc->sc_bst, ma->ma_addr, 2, 0, &sc->sc_bsh))
		panic("mcclock_mainbus_attach: couldn't map clock I/O space");

	/*
	 * Turn interrupts off, just in case.  Need to leave the SQWE
	 * set, because that's the DRAM refresh signal on Rev. B boards.
	 */
	mcclock_mainbus_write(sc, MC_REGB, MC_REGB_SQWE | MC_REGB_BINARY |
	    MC_REGB_24HR);

	sc->sc_mcread = mcclock_mainbus_read;
	sc->sc_mcwrite = mcclock_mainbus_write;
	sc->sc_getcent = NULL;
	sc->sc_setcent = NULL;
	sc->sc_flag = 0;

	/* Algor uses year 1980 as offset */
	sc->sc_year0 = 1980;

	mc146818_attach(sc);

	aprint_normal("\n");
}

void
mcclock_mainbus_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

u_int
mcclock_mainbus_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
