/*	$NetBSD: clock.c,v 1.9.8.2 2006/05/24 10:56:39 yamt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.9.8.2 2006/05/24 10:56:39 yamt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <mips/locore.h>

#include <cobalt/cobalt/clockvar.h>

static	todr_chip_handle_t todr_handle;

void (*timer_start)(void *);
long (*timer_read)(void *);
void *timer_cookie;

/*
 * Common parts of todclock autoconfiguration.
 */
void
todr_attach(todr_chip_handle_t handle)
{

	if (todr_handle)
		panic("todr_attach: too many todclocks configured");

	todr_handle = handle;
}

void
cpu_initclocks(void)
{

	/* start timer */
	if (timer_start == NULL)
		panic("cpu_initclocks(): no timer configured");
	(*timer_start)(timer_cookie);

	return;
}

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(time_t base)
{
	int badbase, waszero;

	if (todr_handle == NULL)
		panic("inittodr: no todclock configured");

	badbase = 0;
	waszero = (base == 0);

	if (base < 5 * SECYR) {
		/*
		 * If base is 0, assume filesystem time is just unknown
		 * in stead of preposterous. Don't bark.
		 */
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* not going to use it anyway, if the chip is readable */
		/* 2006/4/1	12:00:00 */
		base = 36 * SECYR + 91 * SECDAY + SECDAY / 2;
		badbase = 1;
	}

	if (todr_gettime(todr_handle, &time) != 0 ||
	    time.tv_sec == 0) {
		printf("WARNING: bad date in battery clock");
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the clock.
		 */
		time.tv_sec = base;
		if (!badbase)
			resettodr();
	} else {
		int deltat = time.tv_sec - base;

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

	if (time.tv_sec == 0)
		return;

	if (todr_settime(todr_handle, &time) != 0)
		printf("resettodr: cannot set time in time-of-day clock\n");
}

void
setstatclockrate(int arg)
{
	/* XXX */

	return;
}

void
microtime(struct timeval *tvp)
{
	int s;
	static struct timeval lasttime;

	s = splclock();

	*tvp = time;

	if (timer_read)
		tvp->tv_usec += (*timer_read)(timer_cookie);

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

void
delay(unsigned int n)
{
	uint32_t cur, last, delta, usecs;

	last = mips3_cp0_count_read();
	delta = usecs = 0;

	while (n > usecs) {
		cur = mips3_cp0_count_read();

		/* Check to see if the timer has wrapped around. */
		if (cur < last)
			delta += ((curcpu()->ci_cycles_per_hz - last) + cur);
		else
			delta += (cur - last);

		last = cur;

		if (delta >= curcpu()->ci_divisor_delay) {
			usecs += delta / curcpu()->ci_divisor_delay;
			delta %= curcpu()->ci_divisor_delay;
		}
	}
}
