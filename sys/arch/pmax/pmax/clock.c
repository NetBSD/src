/* $NetBSD: clock.c,v 1.30 2000/01/10 03:24:36 simonb Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.30 2000/01/10 03:24:36 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <machine/clock_machdep.h>

#include <dev/dec/clockvar.h>

#include "opt_ntp.h"

#define MINYEAR 1998 /* "today" */

struct device *clockdev;
const struct clockfns *clockfns;
int clockinitted;

#ifdef NTP
extern int fixtick;		/* XXX */
#endif

void
clockattach(dev, fns)
	struct device *dev;
	const struct clockfns *fns;
{

	/*
	 * Just bookkeeping.
	 */
	printf("\n");

	if (clockfns != NULL)
		panic("clockattach: multiple clocks");
	clockdev = dev;
	clockfns = fns;
#ifdef EVCNT_COUNTERS
	evcnt_attach(dev, "intr", &clock_intr_evcnt);
#endif
}

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardare clock lost state.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks()
{

	if (clockfns == NULL)
		panic("cpu_initclocks: no clock attached");

	hz = CLOCK_RATE;	/* 256 Hz clock */
	tick = 1000000 / hz;	/* number of microseconds between interrupts */
	tickfix = 1000000 - (hz * tick);
#ifdef NTP
	fixtick = tickfix;
#endif
	if (tickfix) {
		int ftp;

		ftp = min(ffs(tickfix), ffs(hz));
		tickfix >>= (ftp - 1);
		tickfixinterval = hz >> (ftp - 1);
        }

	/*
	 * Establish the clock interrupt; it's a special case.
	 *
	 * We establish the clock interrupt this late because if
	 * we do it at clock attach time, we may have never been at
	 * spl0() since taking over the system.  Some versions of
	 * PALcode save a clock interrupt, which would get delivered
	 * when we spl0() in autoconf.c.  If established the clock
	 * interrupt handler earlier, that interrupt would go to
	 * hardclock, which would then fall over because p->p_stats
	 * isn't set at that time.
	 */

	/*
	 * Get the clock started.
	 */
	(*clockfns->cf_init)(clockdev);
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(newhz)
	int newhz;
{

	/* nothing we can do */
}

/*
 * Experiments (and  passing years) show that Decstation PROMS
 * assume the kernel uses the clock chip as a time-of-year clock.
 * The PROM assumes the clock is always set to 1972 or 1973, and contains
 * time-of-year in seconds.   The PROM checks the clock at boot time,
 * and if it's outside that range, sets it to 1972-01-01.
 *
 * XXX should be at the mc146818 layer?
*/

/*
 * Initialze the time of day register, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock (if any) provides the rest.
 */
void
inittodr(base)
	time_t base;
{
	struct clocktime ct;
	struct clock_ymdhms dt;
	time_t yearsecs;
	time_t deltat;
	int badbase;

	if (base < (MINYEAR-1970)*SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = (MINYEAR-1970)*SECYR;
		badbase = 1;
	} else
		badbase = 0;

	(*clockfns->cf_get)(clockdev, base, &ct);
#ifdef DEBUG
	printf("readclock: %d/%d/%d/%d/%d/%d", ct.year, ct.mon, ct.day,
	       ct.hour, ct.min, ct.sec);
#endif
	clockinitted = 1;

	/* simple sanity checks */
	if (ct.year < 70 || ct.mon < 1 || ct.mon > 12 || ct.day < 1 ||
	    ct.day > 31 || ct.hour > 23 || ct.min > 59 || ct.sec > 59) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		time.tv_sec = base;
		if (!badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		goto bad;
	}

	/*
	 * The clock lives in 1972 (leapyear!);
	 * calculate seconds relative to this year.
	 */
	dt.dt_year = 1972;
	dt.dt_mon = ct.mon;
	dt.dt_day = ct.day;
	dt.dt_hour = ct.hour;
	dt.dt_min = ct.min;
	dt.dt_sec = ct.sec;
	yearsecs = clock_ymdhms_to_secs(&dt) - (72 - 70) * SECYR;

	/*
	 * Take the actual year from the filesystem if possible;
	 * allow for 2 days of clock loss and 363 days of clock gain.
	 */
	dt.dt_year = 1972; /* or MINYEAR or base/SECYR+1970 ... */
	dt.dt_mon = 1;
	dt.dt_day = 1;
	dt.dt_hour = 0;
	dt.dt_min = 0;
	dt.dt_sec = 0;
	for(;;) {
		time.tv_sec = yearsecs + clock_ymdhms_to_secs(&dt);
		if (badbase || (time.tv_sec > base - 2 * SECDAY))
			break;
		dt.dt_year++;
	}
#ifdef DEBUG
	printf("=>%ld (%ld)\n", time.tv_sec, base);
#endif

	if (!badbase) {
		/*
		 * See if we gained/lost two or more days;
		 * if so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;
		printf("WARNING: clock %s %d days",
		    time.tv_sec < base ? "lost" : "gained",
		       (int) (deltat / SECDAY));
	}
bad:
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
void
resettodr()
{
	time_t yearsecs;
	struct clock_ymdhms dt;
	struct clocktime ct;

	if (!clockinitted)
		return;

	/*
	 * calculate seconds relative to this year
	 */
	clock_secs_to_ymdhms(time.tv_sec, &dt); /* get the year */
	dt.dt_mon = 1;
	dt.dt_day = 1;
	dt.dt_hour = 0;
	dt.dt_min = 0;
	dt.dt_sec = 0;
	yearsecs = time.tv_sec - clock_ymdhms_to_secs(&dt);

	/*
	 * The clock lives in 1972 (leapyear!); calc fictious date.
	 */
#define first72 ((72 - 70) * SECYR)
	clock_secs_to_ymdhms(first72 + yearsecs, &dt);

#ifdef DEBUG
	if (dt.dt_year != 1972)
		printf("resettodr: botch (%ld, %ld)\n", yearsecs, time.tv_sec);
#endif
	ct.year = dt.dt_year % 100; /* rt clock wants 2 digits */
	ct.mon = dt.dt_mon;
	ct.day = dt.dt_day;
	ct.hour = dt.dt_hour;
	ct.min = dt.dt_min;
	ct.sec = dt.dt_sec;
	ct.dow = dt.dt_wday;
#ifdef DEBUG
	printf("setclock: %d/%d/%d/%d/%d/%d\n", ct.year, ct.mon, ct.day,
	       ct.hour, ct.min, ct.sec);
#endif

	(*clockfns->cf_set)(clockdev, &ct);
}
