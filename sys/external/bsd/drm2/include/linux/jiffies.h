/*	$NetBSD: jiffies.h,v 1.7 2018/08/27 06:18:30 riastradh Exp $	*/

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

#ifndef _LINUX_JIFFIES_H_
#define _LINUX_JIFFIES_H_

#include <sys/param.h>
#include <sys/kernel.h>

#define	jiffies		hardclock_ticks
#define	jiffies_64	hardclock_ticks /* XXX */

/* XXX Er, what?  */
#define	MAX_JIFFY_OFFSET	((INT_MAX >> 1) - 1)

static inline uint64_t
nsecs_to_jiffies64(unsigned int msec)
{
	return 1000000*mstohz(msec);
}

static inline unsigned int
msecs_to_jiffies(unsigned int msec)
{
	return mstohz(msec);
}

static inline unsigned int
jiffies_to_msecs(unsigned int j)
{
	return hztoms(j);
}

static inline unsigned int
usecs_to_jiffies(unsigned int usec)
{
	const struct timeval tv = {
		.tv_sec = usec / 1000000,
		.tv_usec = usec % 1000000,
	};

	return tvtohz(&tv);
}

static inline unsigned int
timespec_to_jiffies(const struct timespec *ts)
{
	return tstohz(ts);
}

/* XXX long is the wrong type here times...  */

#define	__linux_time_compare(A, OP, B)	(((long)(A) - (long)(B)) OP 0)

static inline bool
time_after(unsigned long a, unsigned long b)
{
	return __linux_time_compare(a, >, b);
}

static inline bool
time_after_eq(unsigned long a, unsigned long b)
{
	return __linux_time_compare(a, >=, b);
}

static inline bool
time_before(unsigned long a, unsigned long b)
{
	return __linux_time_compare(a, <, b);
}

static inline bool
time_before_eq(unsigned long a, unsigned long b)
{
	return __linux_time_compare(a, <=, b);
}

#endif  /* _LINUX_JIFFIES_H_ */
