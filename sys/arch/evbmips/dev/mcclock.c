/*	$NetBSD: mcclock.c,v 1.1 2002/03/07 14:43:58 simonb Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: mcclock.c,v 1.1 2002/03/07 14:43:58 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ic/mc146818reg.h>

#include <evbmips/evbmips/clockvar.h>
#include <evbmips/dev/mcclockvar.h>

void	mcclock_init(struct device *);
void	mcclock_get(struct device *, time_t, struct clocktime *);
void	mcclock_set(struct device *, struct clocktime *);

const struct clockfns mcclock_clockfns = {
	mcclock_init, mcclock_get, mcclock_set,
};

#define	mc146818_write(dev, reg, datum)					\
	    (*(dev)->sc_busfns->mc_bf_write)(dev, reg, datum)
#define	mc146818_read(dev, reg)						\
	    (*(dev)->sc_busfns->mc_bf_read)(dev, reg)

void
mcclock_attach(struct mcclock_softc *sc, const struct mcclock_busfns *busfns)
{

	printf(": mc146818 or compatible");

	sc->sc_busfns = busfns;

	/*
	 * Turn interrupts off, just in case.  Need to leave the SQWE
	 * set, because that's the DRAM refresh signal on Rev. B boards.
	 */
	mc146818_write(sc, MC_REGB, MC_REGB_SQWE | MC_REGB_BINARY |
	    MC_REGB_24HR);

	clockattach(&sc->sc_dev, &mcclock_clockfns);
}

void
mcclock_init(struct device *dev)
{

	/* We don't use the mcclock for the hardclock interrupt. */
}

/*
 * Note the Algorithmics PMON firmware uses a different year base.
 */
#define	ALGOR_YEAR_OFFSET	80

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
void
mcclock_get(struct device *dev, time_t base, struct clocktime *ct)
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
	ct->year = regs[MC_YEAR] - ALGOR_YEAR_OFFSET;
}

/*
 * Reset the TODR based on the time value.
 */
void
mcclock_set(struct device *dev, struct clocktime *ct)
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
	regs[MC_YEAR] = ct->year + ALGOR_YEAR_OFFSET;

	s = splclock();
	MC146818_PUTTOD(sc, &regs);
	splx(s);
}
