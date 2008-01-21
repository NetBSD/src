/*	$NetBSD: iq80310_timer.c,v 1.17.2.3 2008/01/21 09:36:13 yamt Exp $	*/

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
 * Timer/clock support for the Intel IQ80310.
 *
 * The IQ80310 has a 22-bit reloadable timer implemented in the CPLD.
 * We use it to provide a hardclock interrupt.  There is no RTC on
 * the IQ80310.
 *
 * The timer uses the SPCI clock.  The timer uses the 33MHz clock by
 * reading the SPCI_66EN signal and dividing the clock if necessary.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iq80310_timer.c,v 1.17.2.3 2008/01/21 09:36:13 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <arm/cpufunc.h>

#include <evbarm/iq80310/iq80310reg.h>
#include <evbarm/iq80310/iq80310var.h>
#include <evbarm/iq80310/obiovar.h>

/*
 * Some IQ80310-based designs have fewer bits in the timer counter.
 * Deal with them here.
 */
#if defined(IOP310_TEAMASA_NPWR)
#define	COUNTER_MASK		0x0007ffff
#else /* Default to stock IQ80310 */
#define	COUNTER_MASK		0x003fffff
#endif /* list of IQ80310-based designs */

#define	COUNTS_PER_SEC		33000000	/* 33MHz */
#define	COUNTS_PER_USEC		(COUNTS_PER_SEC / 1000000)

static void *clock_ih;

static uint32_t counts_per_hz;

static u_int	iq80310_get_timecount(struct timecounter *);

static struct timecounter iq80310_timecounter = {
	iq80310_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	COUNTS_PER_SEC,		/* frequency */
	"iq80310",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static volatile uint32_t iq80310_base;

int	clockhandler(void *);

static inline void
timer_enable(uint8_t bit)
{

	CPLD_WRITE(IQ80310_TIMER_ENABLE,
	    CPLD_READ(IQ80310_TIMER_ENABLE) | bit);
}

static inline void
timer_disable(uint8_t bit)
{

	CPLD_WRITE(IQ80310_TIMER_ENABLE,
	    CPLD_READ(IQ80310_TIMER_ENABLE) & ~bit);
}

static inline uint32_t
timer_read(void)
{
	uint32_t rv;
	uint8_t la0, la1, la2, la3;

	/*
	 * First read latches count.
	 *
	 * From RedBoot: harware bug that causes invalid counts to be
	 * latched.  The loop appears to work around the problem.
	 */
	do {
		la0 = CPLD_READ(IQ80310_TIMER_LA0);
	} while (la0 == 0);
	la1 = CPLD_READ(IQ80310_TIMER_LA1);
	la2 = CPLD_READ(IQ80310_TIMER_LA2);
	la3 = CPLD_READ(IQ80310_TIMER_LA3);

	rv  =  ((la0 & 0x40) >> 1) | (la0 & 0x1f);
	rv |= (((la1 & 0x40) >> 1) | (la1 & 0x1f)) << 6;
	rv |= (((la2 & 0x40) >> 1) | (la2 & 0x1f)) << 12;
	rv |= (la3 & 0x0f) << 18;

	return (rv);
}

static inline void
timer_write(uint32_t x)
{

	KASSERT((x & COUNTER_MASK) == x);

	CPLD_WRITE(IQ80310_TIMER_LA0, x & 0xff);
	CPLD_WRITE(IQ80310_TIMER_LA1, (x >> 8) & 0xff);
	CPLD_WRITE(IQ80310_TIMER_LA2, (x >> 16) & 0x3f);
}

/*
 * iq80310_calibrate_delay:
 *
 *	Calibrate the delay loop.
 */
void
iq80310_calibrate_delay(void)
{

	/*
	 * We'll use the CPLD timer for delay(), as well.  We go
	 * ahead and start it up now, just don't enable interrupts
	 * until cpu_initclocks().
	 *
	 * Just use hz=100 for now -- we'll adjust it, if necessary,
	 * in cpu_initclocks().
	 */
	counts_per_hz = COUNTS_PER_SEC / 100;

	timer_disable(TIMER_ENABLE_INTEN);
	timer_disable(TIMER_ENABLE_EN);

	timer_write(counts_per_hz);

	timer_enable(TIMER_ENABLE_EN);
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

	if (hz < 50 || COUNTS_PER_SEC % hz) {
		printf("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
	}

	/*
	 * We only have one timer available; stathz and profhz are
	 * always left as 0 (the upper-layer clock code deals with
	 * this situation).
	 */
	if (stathz != 0)
		printf("Cannot get %d Hz statclock\n", stathz);
	stathz = 0;

	if (profhz != 0)
		printf("Cannot get %d Hz profclock\n", profhz);
	profhz = 0;

	/* Report the clock frequency. */
	printf("clock: hz=%d stathz=%d profhz=%d\n", hz, stathz, profhz);

	/* Hook up the clock interrupt handler. */
	clock_ih = iq80310_intr_establish(XINT3_IRQ(XINT3_TIMER), IPL_CLOCK,
	    clockhandler, NULL);
	if (clock_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");
	
	/* Set up the new clock parameters. */
	oldirqstate = disable_interrupts(I32_bit);

	timer_disable(TIMER_ENABLE_EN);

	counts_per_hz = COUNTS_PER_SEC / hz;
	timer_write(counts_per_hz);

	timer_enable(TIMER_ENABLE_INTEN);
	timer_enable(TIMER_ENABLE_EN);

	restore_interrupts(oldirqstate);

	tc_init(&iq80310_timecounter);
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
setstatclockrate(int newhz)
{

	/*
	 * Nothing to do, here; we can't change the statclock
	 * rate on the IQ80310.
	 */
}

static u_int
iq80310_get_timecount(struct timecounter *tc)
{
	u_int oldirqstate, base, counter;

	oldirqstate = disable_interrupts(I32_bit);
	base = iq80310_base;
	counter = timer_read();
	restore_interrupts(oldirqstate);

	return base + counter;
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
	last = timer_read();
	delta = usecs = 0;

	while (n > usecs) {
		cur = timer_read();

		/* Check to see if the timer has wrapped around. */
		if (cur < last)
			delta += ((counts_per_hz - last) + cur);
		else
			delta += (cur - last);

		last = cur;

		if (delta >= COUNTS_PER_USEC) {
			usecs += delta / COUNTS_PER_USEC;
			delta %= COUNTS_PER_USEC;
		}
	}
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

	timer_disable(TIMER_ENABLE_INTEN);
	timer_enable(TIMER_ENABLE_INTEN);

	atomic_add_32(&iq80310_base, counts_per_hz);

	hardclock(frame);

	/*
	 * Don't run the snake on IOP310-based systems that
	 * don't have the 7-segment display.
	 */
#if !defined(IOP310_TEAMASA_NPWR)
	{
		static int snakefreq;

		if ((snakefreq++ & 15) == 0)
			iq80310_7seg_snake();
	}
#endif

	return (1);
}
