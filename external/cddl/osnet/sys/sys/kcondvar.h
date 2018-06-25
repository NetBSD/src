/*	$NetBSD: kcondvar.h,v 1.3.44.1 2018/06/25 07:25:26 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _OPENSOLARIS_SYS_KCONDVAR_H_
#define	_OPENSOLARIS_SYS_KCONDVAR_H_

#include_next <sys/condvar.h>

#ifdef _KERNEL

typedef enum {
	CV_DEFAULT,
	CV_DRIVER
} kcv_type_t;

typedef enum {
	TR_CLOCK_TICK,
} time_res_t;

#define	cv_init(a, b, c, d)	cv_init(a, "zfscv")

static inline clock_t
cv_timedwait_hires(kcondvar_t *cvp, kmutex_t *mp, hrtime_t tim, hrtime_t res,
    int flag)
{
	extern int hz;
	int ticks = ((uint64_t)tim * hz) / 1000000000;

	return cv_timedwait(cvp, mp, ticks);
}

static inline clock_t
cv_reltimedwait(kcondvar_t *cvp, kmutex_t *mp, clock_t delta, time_res_t res)
{

	cv_wait(cvp, mp);
	return 0;
}

#else

extern	clock_t cv_timedwait_hires(kcondvar_t *, kmutex_t *, hrtime_t, hrtime_t,
    int);

#endif	/* _KERNEL */

#endif	/* _OPENSOLARIS_SYS_KCONDVAR_H_ */
