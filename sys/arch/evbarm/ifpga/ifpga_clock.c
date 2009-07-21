/*	$NetBSD: ifpga_clock.c,v 1.14 2009/07/21 16:04:16 dyoung Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* 
 * The IFPGA has three timers.  Timer 0 is clocked by the system bus clock,
 * while timers 1 and 2 are clocked at 24MHz.  To keep things simple here,
 * we use timers 1 and 2 only.  All three timers are 16-bit counters that
 * are programmable in either periodic mode or in one-shot mode.
 */

/* Include header files */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ifpga_clock.c,v 1.14 2009/07/21 16:04:16 dyoung Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <arm/cpufunc.h>
#include <machine/intr.h>

#include <evbarm/ifpga/ifpgavar.h>
#include <evbarm/ifpga/ifpgamem.h>
#include <evbarm/ifpga/ifpgareg.h>

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
static int timer2min;		/* current, from above choices */
static int statprev;		/* previous value in stat timer */

#define TIMER_1_CLEAR (IFPGA_TIMER1_BASE + TIMERx_CLR)
#define TIMER_1_LOAD  (IFPGA_TIMER1_BASE + TIMERx_LOAD)
#define TIMER_1_VALUE (IFPGA_TIMER1_BASE + TIMERx_VALUE)
#define TIMER_1_CTRL  (IFPGA_TIMER1_BASE + TIMERx_CTRL)

#define TIMER_2_CLEAR (IFPGA_TIMER2_BASE + TIMERx_CLR)
#define TIMER_2_LOAD  (IFPGA_TIMER2_BASE + TIMERx_LOAD)
#define TIMER_2_VALUE (IFPGA_TIMER2_BASE + TIMERx_VALUE)
#define TIMER_2_CTRL  (IFPGA_TIMER2_BASE + TIMERx_CTRL)

#define COUNTS_PER_SEC (IFPGA_TIMER1_FREQ / 16)

static u_int	ifpga_get_timecount(struct timecounter *);

static struct timecounter ifpga_timecounter = {
	ifpga_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	COUNTS_PER_SEC,		/* frequency */
	"ifpga",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static volatile uint32_t ifpga_base;

extern struct ifpga_softc *ifpga_sc;
extern device_t ifpga_dev;

static int clock_started = 0;

static int load_timer(int, int);

static inline u_int
getclock(void)
{
	return bus_space_read_4(ifpga_sc->sc_iot, ifpga_sc->sc_tmr_ioh,
	    TIMER_1_VALUE);
}

static inline u_int
getstatclock(void)
{
	return bus_space_read_4(ifpga_sc->sc_iot, ifpga_sc->sc_tmr_ioh,
	    TIMER_2_VALUE);
}

/*
 * int clockhandler(struct clockframe *frame)
 *
 * Function called by timer 1 interrupts.
 * This just clears the interrupt condition and calls hardclock().
 */

static int
clockhandler(void *fr)
{
	struct clockframe *frame = (struct clockframe *)fr;

	bus_space_write_4(ifpga_sc->sc_iot, ifpga_sc->sc_tmr_ioh,
	    TIMER_1_CLEAR, 0);

	atomic_add_32(&ifpga_base, ifpga_sc->sc_clock_count);

	hardclock(frame);
	return 0;	/* Pass the interrupt on down the chain */
}


/*
 * int statclockhandler(struct clockframe *frame)
 *
 * Function called by timer 2 interrupts.
 * Add some random jitter to the clock, and then call statclock().
 */
 
static int
statclockhandler(void *fr)
{
	struct clockframe *frame = (struct clockframe *) fr;
	int newint, r, var;

	var = statvar;
	do {
		r = random() & (var - 1);
	} while (r == 0);
	newint = timer2min + r;

	if (newint & ~0x0000ffff)
		panic("statclockhandler: statclock variance too large");

	/*
	 * The timer was automatically reloaded with the previous latch
	 * value at the time of the interrupts.  Compensate now for the
	 * amount of time that has run off since then, plus one tick 
	 * roundoff.  This should keep us closer to the mean.
	 */

	r = (statprev - getstatclock() + 1);
	if (r < newint) {
		newint -= r;
		r = 0;
	}
	else 
		printf("statclockhandler: Statclock overrun\n");

	statprev = load_timer(IFPGA_TIMER2_BASE, newint);
	statclock(frame);
	if (r)
		/*
		 * We've completely overrun the previous interval,
		 * make sure we report the correct number of ticks. 
		 */
		statclock(frame);

	return 0;	/* Pass the interrupt on down the chain */
}

static int
load_timer(int base, int intvl)
{
	int control;

	if (intvl & ~0x0000ffff)
		panic("clock: Invalid interval");

	control = (TIMERx_CTRL_ENABLE | TIMERx_CTRL_MODE_PERIODIC | 
	    TIMERx_CTRL_PRESCALE_DIV16);

	bus_space_write_4(ifpga_sc->sc_iot, ifpga_sc->sc_tmr_ioh,
	    base + TIMERx_LOAD, intvl);
	bus_space_write_4(ifpga_sc->sc_iot, ifpga_sc->sc_tmr_ioh,
	    base + TIMERx_CTRL, control);
	bus_space_write_4(ifpga_sc->sc_iot, ifpga_sc->sc_tmr_ioh,
	    base + TIMERx_CLR, 0);
	return intvl;
}

/*
 * void setstatclockrate(int hz)
 *
 * We assume that hz is either stathz or profhz, and that neither will
 * change after being set by cpu_initclocks().  We could recalculate the
 * intervals here, but that would be a pain.
 */

void
setstatclockrate(int new_hz)
{
	if (new_hz == stathz)
		timer2min = statmin;
	else
		timer2min = profmin;
}

/*
 * void cpu_initclocks(void)
 *
 * Initialise the clocks.
 */
 
void
cpu_initclocks(void)
{
	int intvl;
	int statint;
	int profint;
	int minint;

	if (hz < 50 || COUNTS_PER_SEC % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}

	if (stathz == 0)
		stathz = hz;
	else if (stathz < 50 || COUNTS_PER_SEC % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}

	if (profhz == 0)
		profhz = stathz * 5;
	else if (profhz < stathz || COUNTS_PER_SEC % profhz) {
		printf("cannot get %d Hz profclock; using %d Hz\n", profhz,
		    stathz);
		profhz = stathz;
	}

	intvl = COUNTS_PER_SEC / hz;
	statint = COUNTS_PER_SEC / stathz;
	profint = COUNTS_PER_SEC / profhz;
	minint = statint / 2 + 100;
	while (statvar > minint)
		statvar >>= 1;

	/* Adjust interval counts, per note above.  */
	intvl--;
	statint--;
	profint--;

	/* Calculate the base reload values.  */
	statmin = statint - (statvar >> 1);
	profmin = profint - (statvar >> 1);
	timer2min = statmin;
	statprev = statint;

	/* Report the clock frequencies */
	printf("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

	/* Setup timer 1 and claim interrupt */
	ifpga_sc->sc_clockintr = ifpga_intr_establish(IFPGA_TIMER1_IRQ,
	    IPL_CLOCK, clockhandler, 0);
	if (ifpga_sc->sc_clockintr == NULL)
		panic("%s: Cannot install timer 1 interrupt handler",
		    device_xname(ifpga_dev));

	ifpga_sc->sc_clock_count
	    = load_timer(IFPGA_TIMER1_BASE, intvl);

	/*
	 * Use ticks per 256us for accuracy since ticks per us is often
	 * fractional e.g. @ 66MHz
	 */
	ifpga_sc->sc_clock_ticks_per_256us =
	    ((((ifpga_sc->sc_clock_count * hz) / 1000) * 256) / 1000);

	clock_started = 1;

	/* Set up timer 2 as statclk/profclk. */
	ifpga_sc->sc_statclockintr = ifpga_intr_establish(IFPGA_TIMER2_IRQ,
	    IPL_HIGH, statclockhandler, 0);
	if (ifpga_sc->sc_statclockintr == NULL)
		panic("%s: Cannot install timer 2 interrupt handler",
		    device_xname(ifpga_dev));
	load_timer(IFPGA_TIMER2_BASE, statint);

	tc_init(&ifpga_timecounter);
}

static u_int
ifpga_get_timecount(struct timecounter *tc)
{
	u_int base, counter;

	do {
		base = ifpga_base;
		counter = getclock();
	} while (base != ifpga_base);

	return base - counter;
}

/*
 * Estimated loop for n microseconds
 */

/* Need to re-write this to use the timers */

/* One day soon I will actually do this */

int delaycount = 50;

void
delay(u_int n)
{
	if (clock_started) {
		u_int starttime;
		u_int curtime;
		u_int delta = 0;
		u_int count_max = ifpga_sc->sc_clock_count;

		starttime = getclock();

		n *= IFPGA_TIMER1_FREQ / 1000000;

		do {
			n -= delta;
			curtime = getclock();
			delta = curtime - starttime;
			if (curtime < starttime)
				delta += count_max;
			starttime = curtime;
		} while (n > delta);
	} else {
		volatile u_int i;

		if (n == 0) return;
		while (n-- > 0) {
			/* XXX - Seriously gross hack */
			if (cputype == CPU_ID_SA110)
				for (i = delaycount; --i;)
					;
			else
				for (i = 8; --i;)
					;
		}
	}
}
