/*	$NetBSD: clock.c,v 1.8 2005/12/24 20:07:03 perry Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.8 2005/12/24 20:07:03 perry Exp $");

#include "opt_ppcparam.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/clock_subr.h>
#include <machine/cpu.h>
#include <powerpc/marvell/watchdog.h>

#include "opt_kgdb.h"
#if (defined(KGDB) || defined(DDB)) && 0
#include <machine/db_machdep.h>
struct stop_time stop_time;
#endif

/*
 * Initially we assume a processor with a bus frequency of 12.5 MHz.
 */
u_long tbhz = 0;		/* global timebase ticks/sec */
u_long ticks_per_sec = 25000000;
u_long ns_per_tick = 40;
static long ticks_per_intr;

#if NRTC > 0
static inline int yeartoday(int);
#endif
void decr_intr(struct clockframe *frame);

static inline void
mttb(u_quad_t tb)
{
	__asm volatile ("mttbl %0; mttbu %1; mttbl %1+1"
	    ::	"r" (0), "r" (tb));
}

#if NRTC > 0
static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int clockinitted = 0;

static inline int
yeartoday(int year)
{
	return((year % 4) ? 365 : 366);
}
#endif /* NRTC */

#define SECPERDAY	(24*60*60)
#define SECPERNYEAR	(365*SECPERDAY)
#define SECPER4YEARS	(4*SECPERNYEAR+SECPERDAY)
#define EPOCHYEAR	1970

/*
 * Initialze the time of day register, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock (if any) provides the rest.
 */
#define MINYEAR			2002	/* minimum plausible year */
void
inittodr(base)
	time_t base;
{
#if NRTC > 0
	rtc_t rtc;
	int year;
	struct clock_ymdhms dt;
	time_t deltat;
	int badbase;
	int s;

	if (base < (MINYEAR-1970)*SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = (MINYEAR-1970)*SECYR;
		badbase = 1;
	} else
		badbase = 0;

	s = splclock();
	rtc_read(&rtc);
	(void)splx(s);

#if defined(DEBUG) && 0
	printf("inittodr: %02d%02d/%02d/%02d %02d:%02d:%02d\n",
		rtc.rtc_century, rtc.rtc_year, rtc.rtc_month, rtc.rtc_day,
		rtc.rtc_hour, rtc.rtc_minute, rtc.rtc_second);
#endif
	clockinitted = 1;

	year = (rtc.rtc_century * 100) + rtc.rtc_year;

	/* simple sanity checks (2037 = time_t overflow) */
	if (year < MINYEAR || year > 2037 ||
	    rtc.rtc_month < 1 || rtc.rtc_month > 12 || rtc.rtc_day < 1 ||
	    rtc.rtc_day > 31 || rtc.rtc_hour > 23 || rtc.rtc_minute > 59 ||
	    rtc.rtc_second > 59) {
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
	dt.dt_mon = rtc.rtc_month;
	dt.dt_day = rtc.rtc_day;
	dt.dt_hour = rtc.rtc_hour;
	dt.dt_min = rtc.rtc_minute;
	dt.dt_sec = rtc.rtc_second;
	time.tv_sec = clock_ymdhms_to_secs(&dt);

	if (!badbase) {
		/*
		 * See if we gained/lost two or more days;
		 * if so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;				/* all is well */
		printf("WARNING: clock %s %ld days\n",
		    time.tv_sec < base ? "lost" : "gained",
		    (long)deltat / SECDAY);
	}
bad:
	printf("WARNING: CHECK AND RESET THE DATE!\n");
#else	/* NRTC */
	time.tv_sec = base;
#endif /* NRTC */
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
#if NRTC > 0
	struct clock_ymdhms dt;
	rtc_t rtc;
	int s;

	if (!clockinitted)
		return;

	clock_secs_to_ymdhms(time.tv_sec, &dt);

	rtc.rtc_century = dt.dt_year / 100;
	rtc.rtc_year = dt.dt_year % 100;
	rtc.rtc_month = dt.dt_mon;
	rtc.rtc_day = dt.dt_day;
	rtc.rtc_hour = dt.dt_hour;
	rtc.rtc_minute = dt.dt_min;
	rtc.rtc_second = dt.dt_sec;
#if defined(DEBUG) && 0
	printf("resettodr: %02d%02d/%02d/%02d %02d:%02d:%02d\n",
		rtc.rtc_century, rtc.rtc_year, rtc.rtc_month, rtc.rtc_day,
		rtc.rtc_hour, rtc.rtc_minute, rtc.rtc_second);
#endif

	s = splclock();
	rtc_write(&rtc);
	(void)splx(s);
#endif /* NRTC */
}

#ifdef DEBUG
struct clockframe *clockframe = 0;
#endif

void
decr_intr(struct clockframe *frame)
{
	struct cpu_info * const ci = curcpu();
	u_quad_t tb;
	long decrtick;
	int nticks;
	int oipl;
#ifdef DEBUG
	struct clockframe *oframe;
#endif

	WATCHDOG_SERVICE();

	EXT_INTR_STATS_DEPTH();

	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

#ifdef DEBUG
	if (extintr_disable() & PSL_EE)
		panic("decr_intr: msr & PSL_EE");
	oframe = clockframe;
	clockframe = frame;
#endif

	/*
	 * Based on the actual time delay since the last decrementer reload,
	 * we arrange for earlier interrupt next time.
	 */
	__asm volatile ("mfdec %0" : "=r"(decrtick));
	for (nticks = 0; decrtick < 0; nticks++)
		decrtick += ticks_per_intr;
	__asm volatile ("mtdec %0" :: "r"(decrtick));

	uvmexp.intrs++;
	curcpu()->ci_ev_clock.ev_count++;

	EXT_INTR_STATS_CAUSE(0, 0, 0, SIBIT(SIR_HWCLOCK));

	if (ci->ci_cpl >= IPL_CLOCK) {
		/*
		 * we interrupted while at higher
		 * priority, so defer this tick.
		 */ 
		ci->ci_tickspending += nticks;
		EXT_INTR_STATS_PEND_IRQ(SIR_HWCLOCK);
	} else {
		int oframepri;
		EXT_INTR_STATS_DECL(tstart);

		EXT_INTR_STATS_COMMIT_IRQ(SIR_HWCLOCK);
                EXT_INTR_STATS_PRE(SIR_HWCLOCK, tstart);

		nticks += ci->ci_tickspending;
		ci->ci_tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
		tb = mftb();
		ci->ci_lasttb = tb + (decrtick - ticks_per_intr);

		oipl = ci->ci_cpl;
		ci->ci_cpl = IPL_CLOCK;
		SPL_STATS_LOG(IPL_CLOCK, 0);
		(void)extintr_enable();
		
		/*
		 * Do standard timer interrupt stuff.
		 * prevent softclock stuff until the last iteration.
		 */
		oframepri = frame->pri;
		frame->pri = oipl;
		if (oipl < IPL_SOFTCLOCK)
			frame->pri = IPL_SOFTCLOCK;
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = oipl;
		hardclock(frame);
		frame->pri = oframepri;

		(void)extintr_disable();
		ci->ci_cpl = oipl;
		SPL_STATS_LOG(oipl, 0);
		EXT_INTR_STATS_POST(SIR_HWCLOCK, tstart);
	}
	if (imask_test_v(&ipending, &imask[ci->ci_cpl]))
		intr_dispatch();

#ifdef DEBUG
	if ((frame->srr1 & PSL_EE) == 0)
		panic("decr_intr: frame->srr1 & PSL_EE == 0");
	clockframe = oframe;
#endif
}

void
cpu_initclocks()
{
}

void calc_delayconst(void);
void
calc_delayconst()
{
	/*
	 * Get this info during autoconf?				XXX
	 */
#ifdef CLOCKBASE
	ticks_per_sec = CLOCKBASE / 4;	/* from config file */
#endif
	cpu_timebase = ticks_per_sec;
	/*
	 * Should check for correct CPU here?		XXX
	 */
	ns_per_tick = 1000000000 / ticks_per_sec;
	ticks_per_intr = ticks_per_sec / hz;
	curcpu()->ci_lasttb = mftb();
	__asm volatile ("mtdec %0" :: "r"(ticks_per_intr));
}

/*
 * Fill in *tvp with current time with microsecond resolution.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	u_quad_t tb;
	u_long t;
	int msr;

	msr = extintr_disable();
	*tvp = time;
	tb = mftb();
	t = (u_long)(tb - curcpu()->ci_lasttb);
	extintr_restore(msr);

	t *= ns_per_tick;
	t /= 1000;
	t += tvp->tv_usec;
	while (t >= 1000000) {
		t -= 1000000;
		tvp->tv_sec++;
	}
	tvp->tv_usec = t;
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(n)
	unsigned n;
{
	u_quad_t tb;

#ifdef DEBUG
	if (!ticks_per_intr)
		panic("delay: !ticks_per_intr");
#endif
	if (n < 20)
		n = 20;
	
	tb = mftb();
	tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
	while (tb > mftb())
		;
}

/*
 * Nothing to do.
 */
void
setstatclockrate(arg)
	int arg;
{
	/* Do nothing */
}

#if (defined(KGDB) || defined(DDB)) && 0

/* Stop Time facility */

int stop_time_disable;

void
clock_stop_time(struct stop_time *stp)
{
	if (stop_time_disable) {
		stp->st_state |= STS_DISABLED;
		return;
	}
	if (stp->st_state & STS_STOPPED)
		return;

	stp->st_msr = extintr_disable();
	__asm volatile ("mfdec %0" : "=r"(stp->st_decr));
	stp->st_tb = mftb();
	stp->st_state = STS_STOPPED;
}

void
clock_restart_time(struct stop_time *stp)
{
	if ((stp->st_state & STS_STOPPED) == 0) {
		if (stp->st_state & STS_DISABLED)
			return;
		else {

#ifdef DIAGNOSTIC
			panic("clock_restart_time: was not stopped.");
#else
			return;
#endif
		}
	}
	stp->st_state = 0;
	mttb(stp->st_tb);
	__asm volatile ("mtdec %0" :: "r"(stp->st_decr));
	extintr_restore(stp->st_msr);
}

#endif	/* KGDB || DDB */
