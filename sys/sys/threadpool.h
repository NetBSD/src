/*	$NetBSD: threadpool.h,v 1.7 2020/04/25 07:23:21 mlelstv Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef	_SYS_THREADPOOL_H_
#define	_SYS_THREADPOOL_H_

#if !defined(_KERNEL)
#error "not supposed to be exposed to userland"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/queue.h>

struct threadpool;
struct threadpool_job;
struct threadpool_percpu;
struct threadpool_thread;

typedef void threadpool_job_fn_t(struct threadpool_job *);

struct threadpool_job {
	kmutex_t			*job_lock;
	struct threadpool_thread	*job_thread;
	TAILQ_ENTRY(threadpool_job)	job_entry;
	volatile unsigned int		job_refcnt;
	kcondvar_t			job_cv;
	threadpool_job_fn_t		*job_fn;
	char				job_name[MAXCOMLEN];
};

void	threadpools_init(void);

int	threadpool_get(struct threadpool **, pri_t);
void	threadpool_put(struct threadpool *, pri_t);

int	threadpool_percpu_get(struct threadpool_percpu **, pri_t);
void	threadpool_percpu_put(struct threadpool_percpu *, pri_t);
struct threadpool *
	threadpool_percpu_ref(struct threadpool_percpu *);
struct threadpool *
	threadpool_percpu_ref_remote(struct threadpool_percpu *,
	    struct cpu_info *);

void	threadpool_job_init(struct threadpool_job *, threadpool_job_fn_t,
	    kmutex_t *, const char *, ...) __printflike(4,5);
void	threadpool_job_destroy(struct threadpool_job *);
void	threadpool_job_done(struct threadpool_job *);

void	threadpool_schedule_job(struct threadpool *, struct threadpool_job *);
void	threadpool_cancel_job(struct threadpool *, struct threadpool_job *);
bool	threadpool_cancel_job_async(struct threadpool *,
	    struct threadpool_job *);

#endif	/* _SYS_THREADPOOL_H_ */
