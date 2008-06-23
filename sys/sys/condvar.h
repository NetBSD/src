/*	$NetBSD: condvar.h,v 1.9.2.1 2008/06/23 04:32:02 wrstuden Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008 The NetBSD Foundation, Inc.
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

#ifndef _SYS_CONDVAR_H_
#define	_SYS_CONDVAR_H_

#include <sys/mutex.h>

/*
 * The condition variable implementation is private to kern_condvar.c but
 * the size of a kcondvar_t must remain constant.  cv_opaque is protected
 * both by the interlock passed to cv_wait() (enqueue only), and the sleep
 * queue lock acquired with sleeptab_lookup() (enqueue and dequeue). 
 * cv_wmesg is static and does not change throughout the life of the CV.
 */
typedef struct kcondvar {
	void		*cv_opaque[2];	/* sleep queue */
	const char	*cv_wmesg;	/* description for /bin/ps */
} kcondvar_t;

#ifdef _KERNEL

void	cv_init(kcondvar_t *, const char *);
void	cv_destroy(kcondvar_t *);

void	cv_wait(kcondvar_t *, kmutex_t *);
int	cv_wait_sig(kcondvar_t *, kmutex_t *);
int	cv_timedwait(kcondvar_t *, kmutex_t *, int);
int	cv_timedwait_sig(kcondvar_t *, kmutex_t *, int);

void	cv_signal(kcondvar_t *);
void	cv_broadcast(kcondvar_t *);

void	cv_wakeup(kcondvar_t *);

bool	cv_has_waiters(kcondvar_t *);
bool	cv_is_valid(kcondvar_t *);

/* The "lightning bolt", awoken once per second by the clock interrupt. */
extern kcondvar_t lbolt;

#endif	/* _KERNEL */

#endif /* _SYS_CONDVAR_H_ */
