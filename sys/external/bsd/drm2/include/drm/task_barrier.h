/*	$NetBSD: task_barrier.h,v 1.1 2021/12/19 10:58:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef _DRM_TASK_BARRIER_H_
#define _DRM_TASK_BARRIER_H_

#include <sys/condvar.h>
#include <sys/mutex.h>

struct task_barrier {
	kmutex_t	tb_lock;
	kcondvar_t	tb_cv;
	unsigned	tb_cur;
	unsigned	tb_gen;
	unsigned	tb_max;
};

static inline void
task_barrier_init(struct task_barrier *tb)
{

	mutex_init(&tb->tb_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&tb->tb_cv, "taskbar");
	tb->tb_cur = 0;
	tb->tb_gen = 0;
	tb->tb_max = 0;
}

static inline void
task_barrier_destroy(struct task_barrier *tb)
{

	KASSERT(tb->tb_cur == 0);
	cv_destroy(&tb->tb_cv);
	mutex_destroy(&tb->tb_lock);
}

static inline void
task_barrier_add_task(struct task_barrier *tb)
{

	tb->tb_max++;
}

static inline void
task_barrier_rem_task(struct task_barrier *tb)
{

	tb->tb_max--;
}

static inline void
task_barrier_enter(struct task_barrier *tb)
{

	mutex_enter(&tb->tb_lock);
	KASSERT(tb->tb_cur < tb->tb_max);
	if (++tb->tb_cur < tb->tb_max) {
		unsigned gen = tb->tb_gen;
		do {
			cv_wait(&tb->tb_cv, &tb->tb_lock);
		} while (gen == tb->tb_gen);
	} else {
		tb->tb_gen++;
		cv_broadcast(&tb->tb_cv);
	}
	mutex_exit(&tb->tb_lock);
}

static inline void
task_barrier_exit(struct task_barrier *tb)
{

	mutex_enter(&tb->tb_lock);
	KASSERT(tb->tb_cur > 0);
	if (--tb->tb_cur > 0) {
		unsigned gen = tb->tb_gen;
		do {
			cv_wait(&tb->tb_cv, &tb->tb_lock);
		} while (gen == tb->tb_gen);
	} else {
		tb->tb_gen++;
		cv_broadcast(&tb->tb_cv);
	}
}

static inline void
task_barrier_full(struct task_barrier *tb)
{

	task_barrier_enter(tb);
	task_barrier_exit(tb);
}

#endif	/* _DRM_TASK_BARRIER_H_ */
