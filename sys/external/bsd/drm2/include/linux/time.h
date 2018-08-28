/*	$NetBSD: time.h,v 1.5 2018/08/27 07:02:16 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_TIME_H_
#define _LINUX_TIME_H_

#include <sys/time.h>

#define NSEC_PER_MSEC	1000000L

/*
 * XXX get_seconds as implemented by Linux is a Y2038 bug waiting to
 * happen on 32-bit systems because it returns unsigned long.  Some
 * callers in Linux (implicitly) convert the result to time_t, though.
 * We'll pretend get_seconds returns time_t and make sure all our
 * callers treat it as if it did.
 */

static inline time_t
get_seconds(void)
{
	return time_second;
}

static inline void
getrawmonotonic(struct timespec *ts)
{
	getnanouptime(ts);
}

static inline void
do_gettimeofday(struct timeval *tv)
{
	microtime(tv);
}

static inline bool
timespec_valid(const struct timespec *ts)
{
	if (ts->tv_sec < 0)
		return false;
	if (1000000000L <= ts->tv_nsec)
		return false;
	return true;
}

static inline struct timespec
ns_to_timespec(const int64_t nsec)
{
	struct timespec ts;

	ts.tv_sec = (nsec / 1000000000L);
	ts.tv_nsec = (nsec % 1000000000L);
	if (ts.tv_nsec < 0) {
		ts.tv_sec -= 1;
		ts.tv_nsec += 1000000000L;
	}

	return ts;
}

static inline int64_t
timespec_to_ns(const struct timespec *ts)
{
	return (((int64_t)ts->tv_sec * 1000000000LL) + ts->tv_nsec);
}

static inline struct timeval
ns_to_timeval(int64_t nsec)
{
	struct timespec ts;
	struct timeval tv;

	ts = ns_to_timespec(nsec);
	TIMESPEC_TO_TIMEVAL(&tv, &ts);

	return tv;
}

static inline int64_t
timeval_to_ns(const struct timeval *tv)
{
	return (((int64_t)tv->tv_sec * 1000000000UL) + (tv->tv_usec * 1000ul));
}

static inline struct timespec
timespec_sub(struct timespec a, struct timespec b)
{
	struct timespec d;

	timespecsub(&a, &b, &d);

	return d;
}

static inline void
set_normalized_timespec(struct timespec *ts, time_t sec, int64_t nsec)
{
	while (nsec >= 1000000000L) {
		nsec -= 1000000000L;
		sec += 1;
	}
	while (nsec < 0) {
		nsec += 1000000000L;
		sec -= 1;
	}
	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

#endif  /* _LINUX_TIME_H_ */
