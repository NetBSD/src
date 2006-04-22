/*	$NetBSD: clock.c,v 1.35.6.1 2006/04/22 11:37:26 simonb Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * HPs use the MC6840 PTM with the following arrangement:
 *	Timers 1 and 3 are externally driver from a 25 MHz source.
 *	Output from timer 3 is tied to the input of timer 2.
 * The latter makes it possible to use timers 3 and 2 together to get
 * a 32-bit countdown timer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.35.6.1 2006/04/22 11:37:26 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>

#include <dev/clock_subr.h>

#include <hp300/hp300/clockreg.h>

#ifdef GPROF
#include <sys/gmon.h>
#endif

void	statintr(struct clockframe *);

static todr_chip_handle_t todr_handle;

static int clkstd[1];
static int clkint;		/* clock interval, as loaded */
/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
static int statvar = 1024 / 4;	/* {stat,prof}clock variance */
static int statmin;		/* statclock interval - variance/2 */
static int profmin;		/* profclock interval - variance/2 */
static int timer3min;		/* current, from above choices */
static int statprev;		/* previous value in stat timer */

void
todr_attach(todr_chip_handle_t handle)
{
	if (todr_handle != NULL)
		panic("todr_attach: multiple clocks");

	todr_handle = handle;
}

/*
 * Machine-dependent clock routines.
 *
 * A note on the real-time clock:
 * We actually load the clock with interval-1 instead of interval.
 * This is because the counter decrements to zero after N+1 enabled clock
 * periods where N is the value loaded into the counter.
 *
 * The frequencies of the HP300 clocks must be a multiple of four
 * microseconds (since the clock counts in 4 us units).
 */
#define	COUNTS_PER_SEC	(1000000 / CLK_RESOLUTION)

/*
 * Calibrate the delay constant, based on Chuck Cranor's
 * mvme68k delay calibration algorithm.
 */
void
hp300_calibrate_delay(void)
{
	extern int delay_divisor;
	volatile struct clkreg *clk;
	volatile u_char csr;
	int intvl;

	clkstd[0] = IIOV(0x5F8000);		/* XXX yuck */
	clk = (volatile struct clkreg *)clkstd[0];

	/*
	 * Calibrate delay() using the 4 usec counter.
	 * We adjust delay_divisor until we get the result we want.
	 * We assume we've been called at splhigh().
	 */
	for (delay_divisor = 140; delay_divisor > 1; delay_divisor--) {
		/* Reset clock chip */
		clk->clk_cr2 = CLK_CR1;
		clk->clk_cr1 = CLK_RESET;

		/*
		 * Prime the timer.  We're looking for
		 * 10,000 usec (10ms).  See interval comment
		 * above.
		 */
		intvl = (10000 / CLK_RESOLUTION) - 1;
		__asm volatile(" movpw %0,%1@(5)" : : "d" (intvl), "a" (clk));

		/* Enable the timer */
		clk->clk_cr2 = CLK_CR1;
		clk->clk_cr1 = CLK_IENAB;

		delay(10000);

		/* Timer1 interrupt flag high? */
		csr = clk->clk_sr;
		if (csr & CLK_INT1) {
			/*
			 * Got it.  Clear interrupt and get outta here.
			 */
			__asm volatile(" movpw %0@(5),%1" : :
			    "a" (clk), "d" (intvl));
			break;
		}

		/*
		 * Nope.  Poll for completion of the interval,
		 * clear interrupt, and try again.
		 */
		do {
			csr = clk->clk_sr;
		} while ((csr & CLK_INT1) == 0);

		__asm volatile(" movpw %0@(5),%1" : : "a" (clk), "d" (intvl));
	}

	/*
	 * Make sure the clock interrupt is disabled.  Otherwise,
	 * we can end up calling hardclock() before proc0 is set up,
	 * causing a bad pointer deref.
	 */
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_RESET;

	/*
	 * Sanity check the delay_divisor value.  If we totally lost,
	 * assume a 50MHz CPU;
	 */
	if (delay_divisor == 0)
		delay_divisor = 2048 / 50;

	/* Calculate CPU speed. */
	cpuspeed = 2048 / delay_divisor;
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only if
 * no alternative timer is available.
 */
void
cpu_initclocks(void)
{
	volatile struct clkreg *clk;
	int intvl, statint, profint, minint;

	if (todr_handle == NULL)
		panic("cpu_initclocks: no clock attached");

	clkstd[0] = IIOV(0x5F8000);		/* XXX grot */
	clk = (volatile struct clkreg *)clkstd[0];

	if (COUNTS_PER_SEC % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
	}
	/*
	 * Clock has several counters, so we can always use separate
	 * statclock.
	 */
	if (stathz == 0)		/* XXX should be set in param.c */
		stathz = hz;
	else if (COUNTS_PER_SEC % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}
	if (profhz == 0)		/* XXX should be set in param.c */
		profhz = stathz * 5;
	else if (profhz < stathz || COUNTS_PER_SEC % profhz) {
		printf("cannot get %d Hz profclock; using %d Hz\n",
		    profhz, stathz);
		profhz = stathz;
	}

	intvl = COUNTS_PER_SEC / hz;
	statint = COUNTS_PER_SEC / stathz;
	profint = COUNTS_PER_SEC / profhz;
	minint = statint / 2 + 100;
	while (statvar > minint)
		statvar >>= 1;

	tick = intvl * CLK_RESOLUTION;

	/* adjust interval counts, per note above */
	intvl--;
	statint--;
	profint--;

	/* calculate base reload values */
	clkint = intvl;
	statmin = statint - (statvar >> 1);
	profmin = profint - (statvar >> 1);
	timer3min = statmin;
	statprev = statint;

	/* finally, load hardware */
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_RESET;
	__asm volatile(" movpw %0,%1@(5)" : : "d" (intvl), "a" (clk));
	__asm volatile(" movpw %0,%1@(9)" : : "d" (0), "a" (clk));
	__asm volatile(" movpw %0,%1@(13)" : : "d" (statint), "a" (clk));
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_IENAB;
	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = CLK_IENAB;
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int newhz)
{

	if (newhz == stathz)
		timer3min = statmin;
	else
		timer3min = profmin;
}

/*
 * Statistics/profiling clock interrupt.  Compute a new interval.
 * Interrupt has already been cleared.
 *
 * DO THIS INLINE IN locore.s?
 */
void
statintr(struct clockframe *fp)
{
	volatile struct clkreg *clk;
	int newint, r, var;

	clk = (volatile struct clkreg *)clkstd[0];
	var = statvar;
	do {
		r = random() & (var - 1);
	} while (r == 0);
	newint = timer3min + r;

	/*
	 * The timer was automatically reloaded with the previous latch
	 * value at the time of the interrupt.  Compensate now for the
	 * amount of time that has run off since then (minimum of 2-12
	 * timer ticks depending on CPU type) plus one tick roundoff.
	 * This should keep us closer to the mean.
	 */
	__asm volatile(" clrl %0; movpw %1@(13),%0" : "=d" (r) : "a" (clk));
	newint -= (statprev - r + 1);

	__asm volatile(" movpw %0,%1@(13)" : : "d" (newint), "a" (clk));
	statprev = newint;
	statclock(fp);
}

/*
 * Return the best possible estimate of the current time.
 */
void
microtime(struct timeval *tvp)
{
	volatile struct clkreg *clk;
	int s, u, t, u2, s2;
	static struct timeval lasttime;

	/*
	 * Read registers from slowest-changing to fastest-changing,
	 * then re-read out to slowest.  If the values read before the
	 * innermost match those read after, the innermost value is
	 * consistent with the outer values.  If not, it may not be and
	 * we must retry.  Typically this loop runs only once; occasionally
	 * it runs twice, and only rarely does it run longer.
	 *
	 * (Using this loop avoids the need to block interrupts.)
	 */
	clk = (volatile struct clkreg *)clkstd[0];
	do {
		s = time.tv_sec;
		u = time.tv_usec;
		__asm volatile (" clrl %0; movpw %1@(5),%0"
			      : "=d" (t) : "a" (clk));
		u2 = time.tv_usec;
		s2 = time.tv_sec;
	} while (u != u2 || s != s2);

	u += (clkint - t) * CLK_RESOLUTION;
	while (u >= 1000000) {		/* normalize */
		s++;
		u -= 1000000;
	}
	if (s == lasttime.tv_sec &&
	    u <= lasttime.tv_usec &&
	    (u = lasttime.tv_usec + 1) >= 1000000) {
		s++;
		u -= 1000000;
	}
	tvp->tv_sec = s;
	tvp->tv_usec = u;
	lasttime = *tvp;
}

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(time_t base)
{
	struct timeval tv;
	int badbase = 0, waszero = (base == 0);

	if (base < 5 * SECYR) {
		/*
		 * If base is 0, assume filesystem time is just unknown
		 * in stead of preposterous. Don't bark.
		 */
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* not going to use it anyway, if the chip is readable */
		/* 1991/07/01	12:00:00 */
		base = 21*SECYR + 186*SECDAY + SECDAY/2;
		badbase = 1;
	}

	if (todr_gettime(todr_handle, &tv) != 0 ||
	    tv.tv_sec == 0) {
		printf("WARNING: bad date in battery clock");
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the clock.
		 */
		time.tv_sec = base;
		if (!badbase)
			resettodr();
	} else {
		int deltat;

		time = tv;
		deltat = time.tv_sec - base;

		if (deltat < 0)
			deltat = -deltat;
		if (waszero || deltat < 2 * SECDAY)
			return;
		printf("WARNING: clock %s %d days",
		    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the clock based on the current time.
 * Used when the current clock is preposterous, when the time is changed,
 * and when rebooting.  Do nothing if the time is not yet known, e.g.,
 * when crashing during autoconfig.
 */
void
resettodr(void)
{
	struct timeval tv;

	if (time.tv_sec == 0)
		return;

	tv = time;
	if (todr_settime(todr_handle, &tv) != 0)
		printf("resettodr: cannot set time in time-of-day clock\n");
}
