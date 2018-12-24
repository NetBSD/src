/*	$NetBSD: threadpool.h,v 1.2 2018/12/24 21:40:48 thorpej Exp $	*/

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

typedef struct threadpool threadpool_t;
typedef struct threadpool_percpu threadpool_percpu_t;

typedef struct threadpool_job {
#ifdef _LP64
	void	*opaque[11];
#else
	void	*opaque[13];
#endif /* _LP64 */
} threadpool_job_t;

typedef void (*threadpool_job_fn_t)(threadpool_job_t *);

int	threadpool_get(threadpool_t **, pri_t);
void	threadpool_put(threadpool_t *, pri_t);

int	threadpool_percpu_get(threadpool_percpu_t **, pri_t);
void	threadpool_percpu_put(threadpool_percpu_t *, pri_t);
threadpool_t *
	threadpool_percpu_ref(threadpool_percpu_t *);
threadpool_t *
	threadpool_percpu_ref_remote(threadpool_percpu_t *,
	    struct cpu_info *);

void	threadpool_job_init(threadpool_job_t *, threadpool_job_fn_t,
	    kmutex_t *, const char *, ...) __printflike(4,5);
void	threadpool_job_destroy(threadpool_job_t *);
void	threadpool_job_done(threadpool_job_t *);

void	threadpool_schedule_job(threadpool_t *, threadpool_job_t *);
void	threadpool_cancel_job(threadpool_t *, threadpool_job_t *);
bool	threadpool_cancel_job_async(threadpool_t *, threadpool_job_t *);

#endif	/* _SYS_THREADPOOL_H_ */
