/* $NetBSD: mcclock.c,v 1.1 2001/06/13 15:02:12 soda Exp $	*/
/* NetBSD: mcclock.c,v 1.12 1999/01/15 23:29:55 thorpej Exp  */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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

__KERNEL_RCSID(0, "$NetBSD: mcclock.c,v 1.1 2001/06/13 15:02:12 soda Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/ic/mc146818reg.h>

#include <arc/arc/todclockvar.h>
#include <arc/dev/mcclockvar.h>

void mcclock_get __P((struct device *, time_t, struct todclocktime *));
void mcclock_set __P((struct device *, struct todclocktime *));

const struct todclockfns mcclock_todclockfns = {
	mcclock_get, mcclock_set,
};

void
mcclock_attach(sc, busfns, year_offset)
	struct mcclock_softc *sc;
	const struct mcclock_busfns *busfns;
	int year_offset;
{

	printf(": mc146818 or compatible\n");

	sc->sc_busfns = busfns;

	todclockattach(&sc->sc_dev, &mcclock_todclockfns, year_offset);
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
void
mcclock_get(dev, base, ct)
	struct device *dev;
	time_t base;
	struct todclocktime *ct;
{
	struct mcclock_softc *sc = (struct mcclock_softc *)dev;
	mc_todregs regs;
	int s;

	s = splclock();
	MC146818_GETTOD(sc, &regs)
	splx(s);

	ct->sec = regs[MC_SEC];
	ct->min = regs[MC_MIN];
	ct->hour = regs[MC_HOUR];
	ct->dow = regs[MC_DOW];
	ct->day = regs[MC_DOM];
	ct->mon = regs[MC_MONTH];
	ct->year = regs[MC_YEAR];
}

/*
 * Reset the TODR based on the time value.
 */
void
mcclock_set(dev, ct)
	struct device *dev;
	struct todclocktime *ct;
{
	struct mcclock_softc *sc = (struct mcclock_softc *)dev;
	mc_todregs regs;
	int s;

	s = splclock();
	MC146818_GETTOD(sc, &regs);
	splx(s);

	regs[MC_SEC] = ct->sec;
	regs[MC_MIN] = ct->min;
	regs[MC_HOUR] = ct->hour;
	regs[MC_DOW] = ct->dow;
	regs[MC_DOM] = ct->day;
	regs[MC_MONTH] = ct->mon;
	regs[MC_YEAR] = ct->year;

	s = splclock();
	MC146818_PUTTOD(sc, &regs);
	splx(s);
}
