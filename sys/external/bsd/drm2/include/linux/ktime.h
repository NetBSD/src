/*	$NetBSD: ktime.h,v 1.6 2018/08/27 07:27:27 riastradh Exp $	*/

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

#ifndef _LINUX_KTIME_H_
#define _LINUX_KTIME_H_

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/time.h>

#include <linux/time.h>

union ktime {
	int64_t kt_nsec;
};

typedef union ktime ktime_t;

static inline int64_t
ktime_to_ns(ktime_t kt)
{
	return kt.kt_nsec;
}

static inline int64_t
ktime_to_us(ktime_t kt)
{
	return ktime_to_ns(kt)/1000;
}

static inline ktime_t
ns_to_ktime(int64_t nsec)
{
	return (ktime_t) { .kt_nsec = nsec };
}

static inline ktime_t
ktime_add_ns(ktime_t kt, int64_t nsec)
{
	return ns_to_ktime(ktime_to_ns(kt) + nsec);
}

static inline ktime_t
ktime_sub(ktime_t a, ktime_t b)
{
	return ns_to_ktime(ktime_to_ns(a) - ktime_to_ns(b));
}

static inline ktime_t
ktime_sub_ns(ktime_t a, int64_t nsec)
{
	return ns_to_ktime(ktime_to_ns(a) - nsec);
}

static inline struct timespec
ktime_to_timespec(ktime_t kt)
{
	return ns_to_timespec(ktime_to_ns(kt));
}

static inline struct timeval
ktime_to_timeval(ktime_t kt)
{
	return ns_to_timeval(ktime_to_ns(kt));
}

static inline ktime_t
timespec_to_ktime(struct timespec ts)
{
	/* XXX Silently truncate?  */
	return ns_to_ktime(1000000000*(int64_t)ts.tv_sec + ts.tv_nsec);
}

static inline ktime_t
ktime_get(void)
{
	struct timespec ts;

	nanouptime(&ts);

	return timespec_to_ktime(ts);
}

static inline ktime_t
ktime_get_real(void)
{
	struct timespec ts;

	nanotime(&ts);

	return timespec_to_ktime(ts);
}

static inline uint64_t
ktime_get_raw_ns(void)
{

	/* XXX */
	return ktime_to_ns(ktime_get());
}

static inline ktime_t
ktime_get_monotonic_offset(void)
{
	return timespec_to_ktime(boottime);
}

static inline ktime_t
ktime_mono_to_real(ktime_t kt)
{
	struct timespec ts = ktime_to_timespec(kt);

	timespecadd(&ts, &boottime, &ts);

	return timespec_to_ktime(ts);
}

static inline int64_t
ktime_us_delta(ktime_t a, ktime_t b)
{
	return ktime_to_us(ktime_sub(a, b));
}

static inline bool
time_in_range(unsigned long x, unsigned long a, unsigned long b)
{
	return ((a <= x) && (x <= b));
}

#endif  /* _LINUX_KTIME_H_ */
