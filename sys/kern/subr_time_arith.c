/*	$NetBSD: subr_time_arith.c,v 1.2 2025/04/01 23:02:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2000, 2004, 2005, 2007, 2008, 2009, 2020
 *     The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christopher G. Demetriou, by Andrew Doran, and by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 *	@(#)kern_time.c 8.4 (Berkeley) 5/26/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_time_arith.c,v 1.2 2025/04/01 23:02:29 riastradh Exp $");

#include <sys/types.h>

#include <sys/errno.h>
#include <sys/time.h>
#include <sys/timearith.h>

#if defined(_KERNEL)

#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/limits.h>

#elif defined(_TIME_TESTING)

#include <assert.h>
#include <limits.h>
#include <stdbool.h>

extern int hz;
extern int tick;

#define	KASSERT		assert

#endif

/*
 * Compute number of ticks in the specified amount of time.
 */
int
tvtohz(const struct timeval *tv)
{
	unsigned long ticks;
	long sec, usec;

	/*
	 * If the number of usecs in the whole seconds part of the time
	 * difference fits in a long, then the total number of usecs will
	 * fit in an unsigned long.  Compute the total and convert it to
	 * ticks, rounding up and adding 1 to allow for the current tick
	 * to expire.  Rounding also depends on unsigned long arithmetic
	 * to avoid overflow.
	 *
	 * Otherwise, if the number of ticks in the whole seconds part of
	 * the time difference fits in a long, then convert the parts to
	 * ticks separately and add, using similar rounding methods and
	 * overflow avoidance.  This method would work in the previous
	 * case, but it is slightly slower and assumes that hz is integral.
	 *
	 * Otherwise, round the time difference down to the maximum
	 * representable value.
	 *
	 * If ints are 32-bit, then the maximum value for any timeout in
	 * 10ms ticks is 248 days.
	 */
	sec = tv->tv_sec;
	usec = tv->tv_usec;

	KASSERT(usec >= 0);
	KASSERT(usec < 1000000);

	/* catch overflows in conversion time_t->int */
	if (tv->tv_sec > INT_MAX)
		return INT_MAX;
	if (tv->tv_sec < 0)
		return 0;

	if (sec < 0 || (sec == 0 && usec == 0)) {
		/*
		 * Would expire now or in the past.  Return 0 ticks.
		 * This is different from the legacy tvhzto() interface,
		 * and callers need to check for it.
		 */
		ticks = 0;
	} else if (sec <= (LONG_MAX / 1000000))
		ticks = (((sec * 1000000) + (unsigned long)usec + (tick - 1))
		    / tick) + 1;
	else if (sec <= (LONG_MAX / hz))
		ticks = (sec * hz) +
		    (((unsigned long)usec + (tick - 1)) / tick) + 1;
	else
		ticks = LONG_MAX;

	if (ticks > INT_MAX)
		ticks = INT_MAX;

	return ((int)ticks);
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.). We don't
 * timeout the 0,0 value because this means to disable the
 * timer or the interval.
 */
int
itimerfix(struct timeval *tv)
{

	if (tv->tv_usec < 0 || tv->tv_usec >= 1000000)
		return EINVAL;
	if (tv->tv_sec < 0)
		return ETIMEDOUT;
	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < tick)
		tv->tv_usec = tick;
	return 0;
}

int
itimespecfix(struct timespec *ts)
{

	if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000)
		return EINVAL;
	if (ts->tv_sec < 0)
		return ETIMEDOUT;
	if (ts->tv_sec == 0 && ts->tv_nsec != 0 && ts->tv_nsec < tick * 1000)
		ts->tv_nsec = tick * 1000;
	return 0;
}

/*
 * timespecaddok(tsp, usp)
 *
 *	True if tsp + usp can be computed without overflow, i.e., if it
 *	is OK to do timespecadd(tsp, usp, ...).
 */
bool
timespecaddok(const struct timespec *tsp, const struct timespec *usp)
{
	enum { TIME_MIN = __type_min(time_t), TIME_MAX = __type_max(time_t) };
	time_t a = tsp->tv_sec;
	time_t b = usp->tv_sec;
	bool carry;

	/*
	 * Caller is responsible for guaranteeing valid timespec
	 * inputs.  Any user-controlled inputs must be validated or
	 * adjusted.
	 */
	KASSERT(tsp->tv_nsec >= 0);
	KASSERT(usp->tv_nsec >= 0);
	KASSERT(tsp->tv_nsec < 1000000000L);
	KASSERT(usp->tv_nsec < 1000000000L);
	__CTASSERT(1000000000L <= __type_max(long) - 1000000000L);

	/*
	 * Fail if a + b + carry overflows TIME_MAX, or if a + b
	 * overflows TIME_MIN because timespecadd adds the carry after
	 * computing a + b.
	 *
	 * Break it into two mutually exclusive and exhaustive cases:
	 * I. a >= 0
	 * II. a < 0
	 */
	carry = (tsp->tv_nsec + usp->tv_nsec >= 1000000000L);
	if (a >= 0) {
		/*
		 * Case I: a >= 0.  If b < 0, then b + 1 <= 0, so
		 *
		 *	a + b + 1 <= a + 0 <= TIME_MAX,
		 *
		 * and
		 *
		 *	a + b >= 0 + b = b >= TIME_MIN,
		 *
		 * so this can't overflow.
		 *
		 * If b >= 0, then a + b + carry >= a + b >= 0, so
		 * negative results and thus results below TIME_MIN are
		 * impossible; we need only avoid
		 *
		 *	a + b + carry > TIME_MAX,
		 *
		 * which we will do by rejecting if
		 *
		 *	b > TIME_MAX - a - carry,
		 *
		 * which in turn is incidentally always false if b < 0
		 * so we don't need extra logic to discriminate on the
		 * b >= 0 and b < 0 cases.
		 *
		 * Since 0 <= a <= TIME_MAX, we know
		 *
		 *	0 <= TIME_MAX - a <= TIME_MAX,
		 *
		 * and hence
		 *
		 *	-1 <= TIME_MAX - a - 1 < TIME_MAX.
		 *
		 * So we can compute TIME_MAX - a - carry (i.e., either
		 * TIME_MAX - a or TIME_MAX - a - 1) safely without
		 * overflow.
		 */
		if (b > TIME_MAX - a - carry)
			return false;
	} else {
		/*
		 * Case II: a < 0.  If b >= 0, then since a + 1 <= 0,
		 * we have
		 *
		 *	a + b + 1 <= b <= TIME_MAX,
		 *
		 * and
		 *
		 *	a + b >= a >= TIME_MIN,
		 *
		 * so this can't overflow.
		 *
		 * If b < 0, then the intermediate a + b is negative
		 * and the outcome a + b + 1 is nonpositive, so we need
		 * only avoid
		 *
		 *	a + b < TIME_MIN,
		 *
		 * which we will do by rejecting if
		 *
		 *	a < TIME_MIN - b.
		 *
		 * (Reminder: The carry is added afterward in
		 * timespecadd, so to avoid overflow it is not enough
		 * to merely reject a + b + carry < TIME_MIN.)
		 *
		 * It is safe to compute the difference TIME_MIN - b
		 * because b is negative, so the result lies in
		 * (TIME_MIN, 0].
		 */
		if (b < 0 && a < TIME_MIN - b)
			return false;
	}

	return true;
}

/*
 * timespecsubok(tsp, usp)
 *
 *	True if tsp - usp can be computed without overflow, i.e., if it
 *	is OK to do timespecsub(tsp, usp, ...).
 */
bool
timespecsubok(const struct timespec *tsp, const struct timespec *usp)
{
	enum { TIME_MIN = __type_min(time_t), TIME_MAX = __type_max(time_t) };
	time_t a = tsp->tv_sec, b = usp->tv_sec;
	bool borrow;

	/*
	 * Caller is responsible for guaranteeing valid timespec
	 * inputs.  Any user-controlled inputs must be validated or
	 * adjusted.
	 */
	KASSERT(tsp->tv_nsec >= 0);
	KASSERT(usp->tv_nsec >= 0);
	KASSERT(tsp->tv_nsec < 1000000000L);
	KASSERT(usp->tv_nsec < 1000000000L);
	__CTASSERT(1000000000L <= __type_max(long) - 1000000000L);

	/*
	 * Fail if a - b - borrow overflows TIME_MIN, or if a - b
	 * overflows TIME_MAX because timespecsub subtracts the borrow
	 * after computing a - b.
	 *
	 * Break it into two mutually exclusive and exhaustive cases:
	 * I. a < 0
	 * II. a >= 0
	 */
	borrow = (tsp->tv_nsec - usp->tv_nsec < 0);
	if (a < 0) {
		/*
		 * Case I: a < 0.  If b < 0, then -b - 1 >= 0, so
		 *
		 *	a - b - 1 >= a + 0 >= TIME_MIN,
		 *
		 * and, since a <= -1, provided that TIME_MIN <=
		 * -TIME_MAX - 1 so that TIME_MAX <= -TIME_MIN - 1 (in
		 * fact, equality holds, under the assumption of
		 * two's-complement arithmetic),
		 *
		 *	a - b <= -1 - b = -b - 1 <= TIME_MAX,
		 *
		 * so this can't overflow.
		 */
		__CTASSERT(TIME_MIN <= -TIME_MAX - 1);

		/*
		 * If b >= 0, then a - b - borrow <= a - b < 0, so
		 * positive results and thus results above TIME_MAX are
		 * impossible; we need only avoid
		 *
		 *	a - b - borrow < TIME_MIN,
		 *
		 * which we will do by rejecting if
		 *
		 *	a < TIME_MIN + b + borrow.
		 *
		 * The right-hand side is safe to evaluate for any
		 * values of b and borrow as long as TIME_MIN +
		 * TIME_MAX + 1 <= TIME_MAX, i.e., TIME_MIN <= -1.
		 * (Note: If time_t were unsigned, this would fail!)
		 *
		 * Note: Unlike Case I in timespecaddok, this criterion
		 * does not work for b < 0, nor can the roles of a and
		 * b in the inequality be reversed (e.g., -b < TIME_MIN
		 * - a + borrow) without extra cases like checking for
		 * b = TEST_MIN.
		 */
		__CTASSERT(TIME_MIN < -1);
		if (b >= 0 && a < TIME_MIN + b + borrow)
			return false;
	} else {
		/*
		 * Case II: a >= 0.  If b >= 0, then
		 *
		 *	a - b <= a <= TIME_MAX,
		 *
		 * and, provided TIME_MIN <= -TIME_MAX - 1 (in fact,
		 * equality holds, under the assumption of
		 * two's-complement arithmetic)
		 *
		 *	a - b - 1 >= -b - 1 >= -TIME_MAX - 1 >= TIME_MIN,
		 *
		 * so this can't overflow.
		 */
		__CTASSERT(TIME_MIN <= -TIME_MAX - 1);

		/*
		 * If b < 0, then a - b >= a >= 0, so negative results
		 * and thus results below TIME_MIN are impossible; we
		 * need only avoid
		 *
		 *	a - b > TIME_MAX,
		 *
		 * which we will do by rejecting if
		 *
		 *	a > TIME_MAX + b.
		 *
		 * (Reminder: The borrow is subtracted afterward in
		 * timespecsub, so to avoid overflow it is not enough
		 * to merely reject a - b - borrow > TIME_MAX.)
		 *
		 * It is safe to compute the sum TIME_MAX + b because b
		 * is negative, so the result lies in [0, TIME_MAX).
		 */
		if (b < 0 && a > TIME_MAX + b)
			return false;
	}

	return true;
}

/*
 * itimer_transition(it, now, next, &overruns)
 *
 *	Given:
 *
 *	- it: the current state of an itimer (it_value = last expiry
 *	  time, it_interval = periodic rescheduling interval), and
 *
 *	- now: the current time on the itimer's clock;
 *
 *	compute:
 *
 *	- next: the next time the itimer should be scheduled for, and
 *	- overruns: the number of overruns if we're firing late.
 *
 *	XXX This should maybe also say whether the itimer should expire
 *	at all.
 */
void
itimer_transition(const struct itimerspec *restrict it,
    const struct timespec *restrict now,
    struct timespec *restrict next,
    int *restrict overrunsp)
{
	uint64_t last_val, next_val, interval, now_ns;
	int backwards;

	/*
	 * Zero the outputs so we can test assertions in userland
	 * without undefined behaviour.
	 */
	timespecclear(next);
	*overrunsp = 0;

	/*
	 * Paranoia: Caller should guarantee this.
	 */
	if (!timespecisset(&it->it_interval)) {
		timespecclear(next);
		return;
	}

	backwards = (timespeccmp(&it->it_value, now, >));

	/* Nonnegative interval guaranteed by itimerfix.  */
	KASSERT(it->it_interval.tv_sec >= 0);
	KASSERT(it->it_interval.tv_nsec >= 0);

	/* Handle the easy case of non-overflown timers first. */
	if (!backwards &&
	    timespecaddok(&it->it_value, &it->it_interval)) {
		timespecadd(&it->it_value, &it->it_interval,
		    next);
	} else {
		now_ns = timespec2ns(now);
		last_val = timespec2ns(&it->it_value);
		interval = timespec2ns(&it->it_interval);

		next_val = now_ns +
		    (now_ns - last_val + interval - 1) % interval;

		if (backwards)
			next_val += interval;
		else
			*overrunsp = (now_ns - last_val) / interval;

		next->tv_sec = next_val / 1000000000;
		next->tv_nsec = next_val % 1000000000;
	}
}
