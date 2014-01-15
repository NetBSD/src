/*	$NetBSD: ktime.h,v 1.1.2.3 2014/01/15 21:25:39 riastradh Exp $	*/

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

union ktime {
	int64_t kt_nsec;
	struct {
#if _BYTE_ORDER == _BIG_ENDIAN
		int32_t ktsn_sec;
		int32_t ktsn_nsec;
#else
		int32_t ktsn_nsec;
		int32_t ktsn_sec;
#endif
	} kt_sec_nsec;
};

typedef union ktime ktime_t;

static inline int64_t
ktime_to_ns(ktime_t kt)
{
	return kt.kt_nsec;
}

static inline ktime_t
ns_to_ktime(int64_t nsec)
{
	ktime_t kt;

	kt.kt_nsec = nsec;

	return kt;
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

static inline int64_t
timespec_to_ns(struct timespec *t)
{
	return (t->tv_sec * 1000000000ul) + t->tv_nsec;
}

static inline int64_t
timeval_to_ns(struct timeval *tv)
{
	return (tv->tv_sec * 1000000000ul) + (tv->tv_usec * 1000ul);
}

static inline struct timespec
ns_to_timespec(int64_t nsec)
{
	struct timespec t;

	t.tv_sec = (nsec / 1000000000ul);
	t.tv_nsec = (nsec % 1000000000ul);

	return t;
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
ktime_get(void)
{
	struct timespec ts;
	ktime_t kt;

	/* XXX nanotime or nanouptime?  */
	nanouptime(&ts);

	/* XXX Silently truncate?  */
	kt.kt_sec_nsec.ktsn_sec = ts.tv_sec & 0xffffffffUL;
	kt.kt_sec_nsec.ktsn_sec = ts.tv_nsec;

	return kt;
}

static inline ktime_t
ktime_get_monotonic_offset(void)
{
	return ns_to_ktime(0);	/* XXX Obviously wrong!  Revisit.  */
}

#endif  /* _LINUX_KTIME_H_ */
