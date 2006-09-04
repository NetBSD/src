/*	$NetBSD: clock.c,v 1.14 2006/09/04 20:30:40 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.14 2006/09/04 20:30:40 tsutsui Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <mips/locore.h>

#include <cobalt/cobalt/clockvar.h>

void (*timer_start)(void *);
long (*timer_read)(void *);
void *timer_cookie;

#ifdef ENABLE_INT5_STATCLOCK
/*
 * Statistics clock variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
static const uint32_t statvar = 1024;
static uint32_t statint;	/* number of clock ticks for stathz */
static uint32_t statmin;	/* minimum stat clock count in ticks */
static uint32_t statprev;/* last value of we set statclock to */
static u_int statcountperusec;	/* number of ticks per usec at current stathz */
#endif

void
cpu_initclocks(void)
{

#ifdef ENABLE_INT5_STATCLOCK
	if (stathz == 0)
		stathz = hz;

	if (profhz == 0)
		profhz = hz * 5;

	setstatclockrate(stathz);
#endif

	/* start timer interrups for hardclock */
	if (timer_start == NULL)
		panic("cpu_initclocks(): no timer configured");
	(*timer_start)(timer_cookie);

#ifdef ENABLE_INT5_STATCLOCK
	/* enable statclock intr (CPU INT5) */
	_splnone();
#endif
}

void
setstatclockrate(int newhz)
{
#ifdef ENABLE_INT5_STATCLOCK
	uint32_t countpersecond, statvarticks;

	statprev = mips3_cp0_count_read();

	statint = ((curcpu()->ci_cpu_freq + newhz / 2) / newhz) / 2;

	/* Get the total ticks a second */
	countpersecond = statint * newhz;

	/* now work out how many ticks per usec */
	statcountperusec = countpersecond / 1000000;

	/* calculate a variance range of statvar */
	statvarticks = statcountperusec * statvar;

	/* minimum is statint - 50% of variant */
	statmin = statint - (statvarticks / 2);

	mips3_cp0_compare_write(statprev + statint);
#endif
}

#ifdef ENABLE_INT5_STATCLOCK
void
statclockintr(struct clockframe *cfp)
{
	uint32_t curcount, statnext, delta, r;
	int lost;

	lost = 0;

	do {
		r = (uint32_t)random() & (statvar - 1);
	} while (r == 0);
	statnext = statprev + statmin + (r * statcountperusec);

	mips3_cp0_compare_write(statnext);
	curcount = mips3_cp0_count_read();
	delta = statnext - curcount;

	while (__predict_false((int32_t)delta < 0)) {
		lost++;
		delta += statint;
	}
	if (__predict_false(lost > 0)) {
		statnext = curcount + delta;
		mips3_cp0_compare_write(statnext);
		for (; lost > 0; lost--)
			statclock(cfp);
	}
	statclock(cfp);

	statprev = statnext;
}
#endif

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
