/*	$NetBSD: i80321_timer.c,v 1.5 2003/07/15 00:24:54 lukem Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Timer/clock support for the Intel i80321 I/O processor.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i80321_timer.c,v 1.5 2003/07/15 00:24:54 lukem Exp $");

#include "opt_perfctrs.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>

#include <machine/bus.h>
#include <arm/cpufunc.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <arm/xscale/xscalevar.h>

void	(*i80321_hardclock_hook)(void);

#define	COUNTS_PER_SEC		200000000	/* 200MHz */
#define	COUNTS_PER_USEC		(COUNTS_PER_SEC / 1000000)

static void *clock_ih;

static uint32_t counts_per_hz;

int	clockhandler(void *);

static __inline uint32_t
tmr0_read(void)
{
	uint32_t rv;

	__asm __volatile("mrc p6, 0, %0, c0, c1, 0"
		: "=r" (rv));
	return (rv);
}

static __inline void
tmr0_write(uint32_t val)
{

	__asm __volatile("mcr p6, 0, %0, c0, c1, 0"
		:
		: "r" (val));
}

static __inline uint32_t
tcr0_read(void)
{
	uint32_t rv;

	__asm __volatile("mrc p6, 0, %0, c2, c1, 0"
		: "=r" (rv));
	return (rv);
}

static __inline void
tcr0_write(uint32_t val)
{

	__asm __volatile("mcr p6, 0, %0, c2, c1, 0"
		:
		: "r" (val));
}

static __inline void
trr0_write(uint32_t val)
{

	__asm __volatile("mcr p6, 0, %0, c4, c1, 0"
		:
		: "r" (val));
}

static __inline void
tisr_write(uint32_t val)
{

	__asm __volatile("mcr p6, 0, %0, c6, c1, 0"
		:
		: "r" (val));
}

/*
 * i80321_calibrate_delay:
 *
 *	Calibrate the delay loop.
 */
void
i80321_calibrate_delay(void)
{

	/*
	 * Just use hz=100 for now -- we'll adjust it, if necessary,
	 * in cpu_initclocks().
	 */
	counts_per_hz = COUNTS_PER_SEC / 100;

	tmr0_write(0);			/* stop timer */
	tisr_write(TISR_TMR0);		/* clear interrupt */
	trr0_write(counts_per_hz);	/* reload value */
	tcr0_write(counts_per_hz);	/* current value */

	tmr0_write(TMRx_ENABLE|TMRx_RELOAD|TMRx_CSEL_CORE);
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	u_int oldirqstate;
#if defined(PERFCTRS)
	void *pmu_ih;
#endif

	if (hz < 50 || COUNTS_PER_SEC % hz) {
		aprint_error("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
	}
	tick = 1000000 / hz;	/* number of microseconds between interrupts */
	tickfix = 1000000 - (hz * tick);
	if (tickfix) {
		int ftp;

		ftp = min(ffs(tickfix), ffs(hz));
		tickfix >>= (ftp - 1);
		tickfixinterval = hz >> (ftp - 1);
	}

	/*
	 * We only have one timer available; stathz and profhz are
	 * always left as 0 (the upper-layer clock code deals with
	 * this situation).
	 */
	if (stathz != 0)
		aprint_error("Cannot get %d Hz statclock\n", stathz);
	stathz = 0;

	if (profhz != 0)
		aprint_error("Cannot get %d Hz profclock\n", profhz);
	profhz = 0;

	/* Report the clock frequency. */
	aprint_normal("clock: hz=%d stathz=%d profhz=%d\n", hz, stathz, profhz);

	oldirqstate = disable_interrupts(I32_bit);

	/* Hook up the clock interrupt handler. */
	clock_ih = i80321_intr_establish(ICU_INT_TMR0, IPL_CLOCK,
	    clockhandler, NULL);
	if (clock_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");

#if defined(PERFCTRS)
	pmu_ih = i80321_intr_establish(ICU_INT_PMU, IPL_STATCLOCK,
	    xscale_pmc_dispatch, NULL);
	if (pmu_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");
#endif

	/* Set up the new clock parameters. */

	tmr0_write(0);			/* stop timer */
	tisr_write(TISR_TMR0);		/* clear interrupt */

	counts_per_hz = COUNTS_PER_SEC / hz;

	trr0_write(counts_per_hz);	/* reload value */
	tcr0_write(counts_per_hz);	/* current value */

	tmr0_write(TMRx_ENABLE|TMRx_RELOAD|TMRx_CSEL_CORE);

	restore_interrupts(oldirqstate);
}

/*
 * setstatclockrate:
 *
 *	Set the rate of the statistics clock.
 *
 *	We assume that hz is either stathz or profhz, and that neither
 *	will change after being set by cpu_initclocks().  We could
 *	recalculate the intervals here, but that would be a pain.
 */
void
setstatclockrate(int hz)
{

	/*
	 * XXX Use TMR1?
	 */
}

/*
 * microtime:
 *
 *	Fill in the specified timeval struct with the current time
 *	accurate to the microsecond.
 */
void
microtime(struct timeval *tvp)
{
	static struct timeval lasttv;
	u_int oldirqstate;
	uint32_t counts;

	oldirqstate = disable_interrupts(I32_bit);

	counts = counts_per_hz - tcr0_read();

	/* Fill in the timeval struct. */
	*tvp = time;
	tvp->tv_usec += (counts / COUNTS_PER_USEC);

	/* Make sure microseconds doesn't overflow. */
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	/* Make sure the time has advanced. */
	if (tvp->tv_sec == lasttv.tv_sec &&
	    tvp->tv_usec <= lasttv.tv_usec) {
		tvp->tv_usec = lasttv.tv_usec + 1;
		if (tvp->tv_usec >= 1000000) {
			tvp->tv_usec -= 1000000;
			tvp->tv_sec++;
		}
	}

	lasttv = *tvp;

	restore_interrupts(oldirqstate);
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(u_int n)
{
	uint32_t cur, last, delta, usecs;

	/*
	 * This works by polling the timer and counting the
	 * number of microseconds that go by.
	 */
	last = tcr0_read();
	delta = usecs = 0;

	while (n > usecs) {
		cur = tcr0_read();

		/* Check to see if the timer has wrapped around. */
		if (last < cur)
			delta += (last + (counts_per_hz - cur));
		else
			delta += (last - cur);

		last = cur;

		if (delta >= COUNTS_PER_USEC) {
			usecs += delta / COUNTS_PER_USEC;
			delta %= COUNTS_PER_USEC;
		}
	}
}

/*
 * inittodr:
 *
 *	Initialize time from the time-of-day register.
 */
void
inittodr(time_t base)
{

	time.tv_sec = base;
	time.tv_usec = 0;
}

/*
 * resettodr:
 *
 *	Reset the time-of-day register with the current time.
 */
void
resettodr(void)
{
}

/*
 * clockhandler:
 *
 *	Handle the hardclock interrupt.
 */
int
clockhandler(void *arg)
{
	struct clockframe *frame = arg;

	tisr_write(TISR_TMR0);

	hardclock(frame);

	if (i80321_hardclock_hook != NULL)
		(*i80321_hardclock_hook)();

	return (1);
}
