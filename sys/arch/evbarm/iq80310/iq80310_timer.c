/*	$NetBSD: iq80310_timer.c,v 1.14 2003/07/27 04:53:09 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: iq80310_timer.c,v 1.14 2003/07/27 04:53:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>

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

int	clockhandler(void *);

static __inline void
timer_enable(uint8_t bit)
{

	CPLD_WRITE(IQ80310_TIMER_ENABLE,
	    CPLD_READ(IQ80310_TIMER_ENABLE) | bit);
}

static __inline void
timer_disable(uint8_t bit)
{

	CPLD_WRITE(IQ80310_TIMER_ENABLE,
	    CPLD_READ(IQ80310_TIMER_ENABLE) & ~bit);
}

static __inline uint32_t
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

static __inline void
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
	 * Nothing to do, here; we can't change the statclock
	 * rate on the IQ80310.
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

	counts = timer_read();

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

todr_chip_handle_t todr_handle;

/*
 * todr_attach:
 *
 *	Set the specified time-of-day register as the system real-time clock.
 */
void
todr_attach(todr_chip_handle_t todr)
{

	if (todr_handle)
		panic("todr_attach: rtc already configured");
	todr_handle = todr;
}

/*
 * inittodr:
 *
 *	Initialize time from the time-of-day register.
 */
#define	MINYEAR		2003	/* minimum plausible year */
void
inittodr(time_t base)
{
	time_t deltat;
	int badbase;

	if (base < (MINYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR;
		badbase = 1;
	} else
		badbase = 0;

	if (todr_handle == NULL ||
	    todr_gettime(todr_handle, (struct timeval *)&time) != 0 ||
	    time.tv_sec == 0) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		time.tv_sec = base;
		time.tv_usec = 0;
		if (todr_handle != NULL && !badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		goto bad;
	}

	if (!badbase) {
		/*
		 * See if we tained/lost two or more days; if
		 * so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;		/* all is well */
		printf("WARNING: clock %s %ld days\n",
		    time.tv_sec < base ? "lost" : "gained",
		    (long)deltat / SECDAY);
	}
 bad:
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * resettodr:
 *
 *	Reset the time-of-day register with the current time.
 */
void
resettodr(void)
{

	if (time.tv_sec == 0)
		return;

	if (todr_handle != NULL &&
	    todr_settime(todr_handle, (struct timeval *)&time) != 0)
		printf("resettodr: failed to set time\n");
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
