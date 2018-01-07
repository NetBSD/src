/*	$NetBSD: time.h,v 1.8 2018/01/07 20:02:52 christos Exp $	*/

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
 * $FreeBSD: src/sys/compat/opensolaris/sys/time.h,v 1.2 2007/11/28 21:44:17 jb Exp $
 */

#ifndef _OPENSOLARIS_SYS_TIME_H_
#define	_OPENSOLARIS_SYS_TIME_H_

#include_next <sys/time.h>

#define SEC		1
#define MILLISEC	1000
#define MICROSEC	1000000
#define NANOSEC		1000000000

#define	LBOLT	((gethrtime() * hz) / NANOSEC)

#define TIMESPEC_OVERFLOW(ts)                       \
	((ts)->tv_sec < INT32_MIN || (ts)->tv_sec > INT32_MAX)

#ifdef _KERNEL
#include <sys/systm.h>

static __inline hrtime_t
gethrtime(void) {

	struct timespec ts;
	hrtime_t nsec;

#if 1
	getnanouptime(&ts);
#else
	nanouptime(&ts);
#endif
	nsec = (hrtime_t)ts.tv_sec * NANOSEC + ts.tv_nsec;
	return (nsec);
}

#define	gethrestime_sec()	(time_second)
#define	gethrestime(ts)		getnanotime(ts)

#else

#ifdef CLOCK_REALTIME
int clock_gettime(clockid_t, struct timespec *)
#ifdef __RENAME
    __RENAME(__clock_gettime50)
#endif
;
#else
#include <stdio.h>	/* For NULL */
#endif

static __inline hrtime_t gethrtime(void) {
#ifdef CLOCK_REALTIME
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return (hrtime_t)(((int64_t) ts.tv_sec) * NANOSEC + ts.tv_nsec);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (hrtime_t)
	    ((((int64_t) tv.tv_sec) * MICROSEC + tv.tv_usec) * 1000);
#endif
}

#endif	/* _KERNEL */

#endif	/* !_OPENSOLARIS_SYS_TIME_H_ */
