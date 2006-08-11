/*	$NetBSD: clock.c,v 1.6 2006/08/11 15:19:59 rjs Exp $	*/
/*      $OpenBSD: clock.c,v 1.3 1997/10/13 13:42:53 pefo Exp $	*/

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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <uvm/uvm_extern.h>

#include <dev/clock_subr.h>

#include <ibmnws/ibmnws/clockvar.h>

#define	MINYEAR	1990

void decr_intr(struct clockframe *);
void init_ibmnws_tc(void);
static u_int get_ibmnws_timecount(struct timecounter *);

/*
 * Initially we assume a processor with a bus frequency of 12.5 MHz.
 */
u_long ticks_per_sec;
u_long ns_per_tick;
static long ticks_per_intr;

struct device *clockdev;
const struct clockfns *clockfns;
int clockinitted;

static void ns1000_clock_init(struct device *);
static void ns1000_clock_get(struct device *, time_t, struct clocktime *);
static void ns1000_clock_set(struct device *, struct clocktime *);

const struct clockfns ns1000_clockfns = {
	ns1000_clock_init,
	ns1000_clock_get,
	ns1000_clock_set
};

static struct timecounter ibmnws_timecounter = {
	get_ibmnws_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0x7fffffff,		/* counter_mask */
	0,			/* frequency */
	"ibmnws_mftb",		/* name */
	0			/* quality */
};

void
ns1000_clock_init(struct device *dev)
{
}

void
ns1000_clock_get(struct device *dev, time_t base, struct clocktime *ct)
{
        ct->sec		= 0;
        ct->min		= 0;
        ct->hour	= 0;
        ct->dow  	= 0;
        ct->day  	= 0;
        ct->mon  	= 0;
        ct->year  	= 0;
}

void
ns1000_clock_set(struct device *dev, struct clocktime *ct)
{
}

void
clockattach(struct device *dev, const struct clockfns *fns)
{

	printf("\n");

	if (clockfns != NULL)
		panic("clockattach: multiple clocks");

	clockdev = dev;
	clockfns = fns;
}

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{
	struct cpu_info * const ci = curcpu();

	ticks_per_intr = ticks_per_sec / hz;
	__asm volatile ("mftb %0" : "=r"(ci->ci_lasttb));
	__asm volatile ("mtdec %0" :: "r"(ticks_per_intr));

	init_ibmnws_tc();
	/*
	 * The NS 1000 has no RTC hardware, so fake the clock functions
	 * to prevent odd failures...
	 */

	clockattach (NULL, &ns1000_clockfns);
}

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(time_t base)
{
	struct clocktime ct;
	int year;
	struct clock_ymdhms dt;
	time_t deltat;
	struct timespec ts;
	struct timeval time;
	int badbase;

	badbase = 0;
	time.tv_sec = 0;
	time.tv_usec = 0;

	if (base < (MINYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR + 186 * SECDAY + SECDAY / 2;
		badbase = 1;
	}

	(*clockfns->cf_get)(clockdev, base, &ct);
#ifdef DEBUG
	printf("readclock: %d/%d/%d/%d/%d/%d\n", ct.year, ct.mon, ct.day,
	    ct.hour, ct.min, ct.sec);
#endif
	clockinitted = 1;

	year = 1900 + ct.year;
	if (year < 1970)
		year += 100;

	/* simple sanity checks (2037 = time_t overflow) */
	if (year < MINYEAR || year > 2037 ||
	    ct.mon < 1 || ct.mon > 12 || ct.day < 1 ||
	    ct.day > 31 || ct.hour > 23 || ct.min > 59 || ct.sec > 59) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		ts.tv_sec = base;
		ts.tv_nsec = 0;
		tc_setclock(&ts);

		if (!badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		goto bad;
	}

	dt.dt_year = year;
	dt.dt_mon = ct.mon;
	dt.dt_day = ct.day;
	dt.dt_hour = ct.hour;
	dt.dt_min = ct.min;
	dt.dt_sec = ct.sec;
	time.tv_sec = clock_ymdhms_to_secs(&dt);
#ifdef DEBUG
	printf("=>%ld (%d)\n", (long int)time.tv_sec, (int)base);
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
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
void
resettodr(void)
{
	struct clock_ymdhms dt;
	struct clocktime ct;
	struct timeval time;

	if (!clockinitted)
		return;

	getmicrotime(&time);
	clock_secs_to_ymdhms(time.tv_sec, &dt);

	/* rt clock wants 2 digits */
	ct.year = dt.dt_year % 100;
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

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int arg)
{

	/* Nothing we can do */
}

void
decr_intr(struct clockframe *frame)
{
	struct cpu_info * const ci = curcpu();
	int msr;
	int pri;
	u_long tb;
	long ticks;
	int nticks;

	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

	/*
	 * Based on the actual time delay since the last decrementer reload,
	 * we arrange for earlier interrupt next time.
	 */
	__asm ("mfdec %0" : "=r"(ticks));
	for (nticks = 0; ticks < 0; nticks++)
		ticks += ticks_per_intr;
	__asm volatile ("mtdec %0" :: "r"(ticks));

	uvmexp.intrs++;
	ci->ci_ev_clock.ev_count++;

	pri = splclock();
	if (pri & SPL_CLOCK)
		ci->ci_tickspending += nticks;
	else {
		nticks += ci->ci_tickspending;
		ci->ci_tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
		__asm ("mftb %0" : "=r"(tb));
		ci->ci_lasttb = tb + ticks - ticks_per_intr;

		/*
		 * Reenable interrupts
		 */
		__asm volatile ("mfmsr %0; ori %0, %0, %1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
		
		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		frame->pri = pri | SINT_CLOCK;
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);
	}
	splx(pri);
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(unsigned int n)
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;
	
	tb = mftb();
	tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
	tbh = tb >> 32;
	tbl = tb;
	__asm volatile ("1: mftbu %0; cmplw %0,%1; blt 1b; bgt 2f;"
		      "mftb %0; cmplw %0,%2; blt 1b; 2:"
		      : "=r"(scratch) : "r"(tbh), "r"(tbl));
}

static u_int
get_ibmnws_timecount(struct timecounter *tc)
{
	u_long tb;
	int msr, scratch;
	
	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	__asm volatile ("mftb %0" : "=r"(tb));
	mtmsr(msr);

	return tb;
}

void
init_ibmnws_tc()
{
	/* from machdep initialization */
	ibmnws_timecounter.tc_frequency = ticks_per_sec;
	tc_init(&ibmnws_timecounter);
}
