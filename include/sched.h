/*	$NetBSD: sched.h,v 1.4 2003/07/08 05:41:51 itojun Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SCHED_H_
#define _SCHED_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <sys/sched.h>

/* Required by POSIX 1003.1, section 13.1, lines 12-13. */
#include <time.h>	

/* Functions */

__BEGIN_DECLS
/* 
 * These are permitted to fail and return ENOSYS if
 * _POSIX_PRIORITY_SCHEDULING is not defined.
 */
int	sched_setparam(pid_t, const struct sched_param *);
int	sched_getparam(pid_t, struct sched_param *);
int	sched_setscheduler(pid_t, int, const struct sched_param *);
int	sched_getscheduler(pid_t);
int	sched_get_priority_max(int);
int	sched_get_priority_min(int);
int	sched_rr_get_interval(pid_t, struct timespec *);


/* Not optional in the presence of _POSIX_THREADS */
int	sched_yield(void);
__END_DECLS

#if defined(_NETBSD_SOURCE)

/*
 * Stuff that for historical reasons is in <sched.h>, but not defined
 * by any standard.
 */

__BEGIN_DECLS
pid_t	 clone __P((int (*)(void *), void *, int, void *));
pid_t	__clone __P((int (*)(void *), void *, int, void *));
__END_DECLS

#endif /* _NETBSD_SOURCE */

#endif /* _SCHED_H_ */
