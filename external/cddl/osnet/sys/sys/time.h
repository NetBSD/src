/*	$NetBSD: time.h,v 1.12 2018/06/03 05:55:08 chs Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/time.h 296530 2016-03-08 18:28:24Z mav $
 */

#ifndef _OPENSOLARIS_SYS_TIME_H_
#define	_OPENSOLARIS_SYS_TIME_H_

#include_next <sys/time.h>

#define SEC		1
#define MILLISEC	1000
#define MICROSEC	1000000
#define NANOSEC		1000000000
#define TIME_MAX	LLONG_MAX

#define	MSEC2NSEC(m)	((hrtime_t)(m) * (NANOSEC / MILLISEC))
#define	NSEC2MSEC(n)	((n) / (NANOSEC / MILLISEC))

#define	NSEC2SEC(n)	((n) / (NANOSEC / SEC))
#define	SEC2NSEC(m)	((hrtime_t)(m) * (NANOSEC / SEC))

#ifndef __defined_hr_t
#define __defined_hr_t
typedef longlong_t	hrtime_t;
#endif

#if defined(__i386__) || defined(__powerpc__)
#define	TIMESPEC_OVERFLOW(ts)						\
	((ts)->tv_sec < INT32_MIN || (ts)->tv_sec > INT32_MAX)
#else
#define	TIMESPEC_OVERFLOW(ts)						\
	((ts)->tv_sec < INT64_MIN || (ts)->tv_sec > INT64_MAX)
#endif

#define	SEC_TO_TICK(sec)	((sec) * hz)
#define	NSEC_TO_TICK(nsec)	((nsec) / (NANOSEC / hz))

#ifdef _KERNEL

static __inline hrtime_t
gethrtime(void) {

	struct timespec ts;
	hrtime_t nsec;

	getnanouptime(&ts);
	nsec = (hrtime_t)ts.tv_sec * NANOSEC + ts.tv_nsec;
	return (nsec);
}

#define	gethrestime_sec()	(time_second)
#define	gethrestime(ts)		getnanotime(ts)
#define	gethrtime_waitfree()	gethrtime()

static inline int64_t
ddi_get_lbolt64(void)
{
	struct timespec ts;
	const int hz = 100;

	getnanouptime(&ts);
	return (int64_t)(SEC_TO_TICK(ts.tv_sec) + NSEC_TO_TICK(ts.tv_nsec));
}

#define ddi_get_lbolt()		(clock_t)ddi_get_lbolt64()

#else

#ifdef __NetBSD__
int clock_gettime(clockid_t, struct timespec *)
    __RENAME(__clock_gettime50);
#endif
#ifdef __linux__
#include <time.h>
#endif

static __inline hrtime_t gethrtime(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return (((u_int64_t) ts.tv_sec) * NANOSEC + ts.tv_nsec);
}

#define	ddi_get_lbolt()		(gethrtime() >> 23)
#define	ddi_get_lbolt64()	(gethrtime() >> 23)

#endif	/* _KERNEL */

#endif	/* !_OPENSOLARIS_SYS_TIME_H_ */
