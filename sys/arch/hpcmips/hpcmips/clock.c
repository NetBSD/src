/*	$NetBSD: clock.c,v 1.11.4.1 2001/10/01 12:39:06 fvdl Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura, All rights reserved.
 * Copyright (c) 1999-2001 SATO Kazumi, All rights reserved.
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
 */

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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.11.4.1 2001/10/01 12:39:06 fvdl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>			/* hz */

#include <dev/clock_subr.h>
#include <machine/sysconf.h>		/* platform */

#define MINYEAR			2001	/* "today" */
#define UNIX_YEAR_OFFSET	0

/* 
 * platform_clock_attach:
 *
 *	Register CPU(VR41XX or TX39XX) dependent clock routine to system.
 */
void
platform_clock_attach(void *ctx, struct platform_clock *clock)
{

	printf("\n");

	clock->self = ctx;
	platform.clock = clock;
}

/*
 * cpu_initclocks:
 *
 *	starts periodic timer, which provides hardclock interrupts to
 *	kern_clock.c.
 *	Leave stathz 0 since there are no other timers available.
 */
void
cpu_initclocks()
{
	struct platform_clock *clock = platform.clock;

	if (clock == NULL)
		panic("cpu_initclocks: no clock attached");

	hz = clock->hz;
	tick = 1000000 / hz;	
	/* number of microseconds between interrupts */
	tickfix = 1000000 - (hz * tick);
	if (tickfix) {
		int ftp;
		
		ftp = min(ffs(tickfix), ffs(hz));
		tickfix >>= (ftp - 1);
		tickfixinterval = hz >> (ftp - 1);
	}

	/* start periodic timer */
	(*clock->init)(clock->self);
}

/*
 * setstatclockrate:
 *
 *	We assume newhz is either stathz or profhz, and that neither will
 *	change after being set up above.  Could recalculate intervals here
 *	but that would be a drag.
 */
void
setstatclockrate(int newhz)
{

	/* nothing we can do */
}

/*
 * inittodr:
 *
 *	initializes the time of day hardware which provides
 *	date functions.  Its primary function is to use some file
 *	system information in case the hardare clock lost state.
 *
 *	Initialze the time of day register, based on the time base which is,
 *	e.g. from a filesystem.  Base provides the time to within six months,
 *	and the time of year clock (if any) provides the rest.
 */
void
inittodr(time_t base)
{
	struct platform_clock *clock = platform.clock;
	struct clock_ymdhms dt;
	int year, badbase;
	time_t deltat;

	if (clock == NULL)
		panic("inittodr: no clock attached");		

	if (base < (MINYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR;
		badbase = 1;
	} else
		badbase = 0;

	(*clock->rtc_get)(clock->self, base, &dt);
#ifdef DEBUG
	printf("readclock: %d/%d/%d/%d/%d/%d", dt.dt_year, dt.dt_mon, dt.dt_day,
	    dt.dt_hour, dt.dt_min, dt.dt_sec);
#endif
	clock->start = 1;

	year = 1900 + UNIX_YEAR_OFFSET + dt.dt_year;
	if (year < 1970)
		year += 100;
	/* simple sanity checks (2037 = time_t overflow) */
	if (year < MINYEAR || year > 2037 ||
	    dt.dt_mon < 1 || dt.dt_mon > 12 || dt.dt_day < 1 ||
	    dt.dt_day > 31 || dt.dt_hour > 23 || dt.dt_min > 59 ||
	    dt.dt_sec > 59) {
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

	dt.dt_year = year;
	time.tv_sec = clock_ymdhms_to_secs(&dt);
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
		printf("WARNING: clock %s %ld days",
		    time.tv_sec < base ? "lost" : "gained",
		    (long)deltat / SECDAY);
	}
 bad:
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * resettodr:
 *
 *	restores the time of day hardware after a time change.
 *
 *	Reset the TODR based on the time value; used when the TODR
 *	has a preposterous value and also when the time is reset
 *	by the stime system call.  Also called when the TODR goes past
 *	TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 *	to wrap the TODR around.
 */
void
resettodr()
{
	struct platform_clock *clock = platform.clock;
	struct clock_ymdhms dt;

	if (clock == NULL)
		panic("inittodr: no clock attached");		

	if (!clock->start)
		return;

	clock_secs_to_ymdhms(time.tv_sec, &dt);

	/* rt clock wants 2 digits XXX */
	dt.dt_year = (dt.dt_year - UNIX_YEAR_OFFSET) % 100;
#ifdef DEBUG
	printf("setclock: %d/%d/%d/%d/%d/%d\n", dt.dt_year, dt.dt_mon,
	    dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec);
#endif

	(*clock->rtc_set)(clock->self, &dt);
}

/*
 * microtime:
 *
 *	Return the best possible estimate of the time in the timeval to
 *	which tvp points.  We guarantee that the time will be greater than
 *	the value obtained by a previous call.
 */
void
microtime(struct timeval *tvp)
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;

	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

/*
 * delay:
 *
 *	Wait at least "n" microseconds.
 */
void
delay(int n)
{

        DELAY(n);
}
