/*	$NetBSD: srcu.h,v 1.3 2021/12/19 11:10:09 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_SRCU_H_
#define	_LINUX_SRCU_H_

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/mutex.h>

/* namespace */
#define	srcu_fini		linux_srcu_fini
#define	srcu_init		linux_srcu_init
#define	srcu_read_lock		linux_srcu_read_lock
#define	srcu_read_unlock	linux_srcu_read_unlock
#define	synchronize_srcu	linux_synchronize_srcu

struct lwp;
struct percpu;

struct srcu_struct {
	struct percpu		*srcu_percpu;	/* struct srcu_cpu */
	kmutex_t		srcu_lock;
	kcondvar_t		srcu_cv;
	struct lwp		*srcu_sync;
	int64_t			srcu_total;
	volatile unsigned	srcu_gen;
};

void	srcu_init(struct srcu_struct *, const char *);
void	srcu_fini(struct srcu_struct *);

int	srcu_read_lock(struct srcu_struct *);
void	srcu_read_unlock(struct srcu_struct *, int);

void	synchronize_srcu(struct srcu_struct *);

#endif	/* _LINUX_SRCU_H_ */
