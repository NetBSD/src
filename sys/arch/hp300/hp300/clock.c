/*	$NetBSD: clock.c,v 1.39 2008/01/07 16:55:15 joerg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.39 2008/01/07 16:55:15 joerg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/timetc.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>

#include <hp300/hp300/clockreg.h>

#ifdef GPROF
#include <sys/gmon.h>
#endif

void	statintr(struct clockframe *);
static u_int mc6840_counter(struct timecounter *);

static int clkstd[1];

int clkint;			/* clock interval, as loaded */
uint32_t clkcounter;		/* for timecounter */

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
	static struct timecounter tc = {
		mc6840_counter,		/* get_timecount */
		NULL,			/* no poll_pps */
		~0,			/* counter mask */
		COUNTS_PER_SEC,		/* frequency */
		"mc6840",		/* name */
		100			/* quality */
	};

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

	tc_init(&tc);
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

u_int
mc6840_counter(struct timecounter *tc)
{
	volatile struct clkreg *clk;
	uint32_t ccounter, count;
	static uint32_t lastcount;
	int s;

	clk = (volatile struct clkreg *)clkstd[0];

	s = splclock();
	ccounter = clkcounter;
	/* XXX reading counter clears interrupt flag?? */
	__asm volatile (" clrl %0; movpw %1@(5),%0"
		      : "=d" (count) : "a" (clk));
	splx(s);

	count = ccounter + (clkint - count);
	if ((int32_t)(count - lastcount) < 0) {
		/* XXX wrapped; maybe hardclock() is blocked more than 1/HZ */
		count = lastcount + 1;
	}
	lastcount = count;

	return count;
}
