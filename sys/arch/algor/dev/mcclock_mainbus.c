/*	$NetBSD: mcclock_mainbus.c,v 1.1 2001/06/01 16:00:04 thorpej Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: mcclock_mainbus.c,v 1.1 2001/06/01 16:00:04 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/mc146818reg.h>

#include <algor/algor/clockvar.h>
#include <algor/dev/mcclockvar.h>

struct mcclock_mainbus_softc {
	struct mcclock_softc	sc_mcclock;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

int	mcclock_mainbus_match(struct device *, struct cfdata *, void *);
void	mcclock_mainbus_attach(struct device *, struct device *, void *);

struct cfattach mcclock_mainbus_ca = {
	sizeof (struct mcclock_mainbus_softc), mcclock_mainbus_match,
	    mcclock_mainbus_attach, 
};

void	mcclock_mainbus_write(struct mcclock_softc *, u_int, u_int);
u_int	mcclock_mainbus_read(struct mcclock_softc *, u_int);

const struct mcclock_busfns mcclock_mainbus_busfns = {
	mcclock_mainbus_write, mcclock_mainbus_read,
};

int
mcclock_mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, match->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

void
mcclock_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct mcclock_mainbus_softc *sc = (void *)self;

	sc->sc_iot = ma->ma_st;
	if (bus_space_map(sc->sc_iot, ma->ma_addr, 2, 0, &sc->sc_ioh))
		panic("mcclock_mainbus_attach: couldn't map clock I/O space");

	mcclock_attach(&sc->sc_mcclock, &mcclock_mainbus_busfns);
}

void
mcclock_mainbus_write(struct mcclock_softc *mcsc, u_int reg, u_int datum)
{
	struct mcclock_mainbus_softc *sc = (void *)mcsc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

u_int
mcclock_mainbus_read(struct mcclock_softc *mcsc, u_int reg)
{
	struct mcclock_mainbus_softc *sc = (void *)mcsc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
