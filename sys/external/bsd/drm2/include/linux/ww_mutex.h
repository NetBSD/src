/*	$NetBSD: ww_mutex.h,v 1.12 2018/08/27 06:06:41 riastradh Exp $	*/

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

#ifndef _ASM_WW_MUTEX_H_
#define _ASM_WW_MUTEX_H_

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/rbtree.h>

#include <linux/mutex.h>

struct ww_class {
	volatile uint64_t	wwc_ticket;
};

#define	DEFINE_WW_CLASS(CLASS)						      \
	struct ww_class CLASS = {					      \
		.wwc_ticket = 0,					      \
	}

struct ww_acquire_ctx {
	struct ww_class	*wwx_class __diagused;
	struct lwp	*wwx_owner __diagused;
	uint64_t	wwx_ticket;
	unsigned	wwx_acquired;
	bool		wwx_acquire_done;
	struct rb_node	wwx_rb_node;
};

struct ww_mutex {
	enum ww_mutex_state {
		WW_UNLOCKED,	/* nobody owns it */
		WW_OWNED,	/* owned by a lwp without a context */
		WW_CTX,		/* owned by a context */
		WW_WANTOWN,	/* owned by ctx, waiters w/o ctx waiting */
	}			wwm_state;
	union {
		struct lwp		*owner;
		struct ww_acquire_ctx	*ctx;
	}			wwm_u;
	/*
	 * XXX wwm_lock must *not* be first, so that the ww_mutex has a
	 * different address from the kmutex for LOCKDEBUG purposes.
	 */
	kmutex_t		wwm_lock;
	struct ww_class		*wwm_class;
	struct rb_tree		wwm_waiters;
	kcondvar_t		wwm_cv;
#ifdef LOCKDEBUG
	bool			wwm_debug;
#endif
};

/* XXX Make the nm output a little more greppable...  */
#define	ww_acquire_done		linux_ww_acquire_done
#define	ww_acquire_fini		linux_ww_acquire_fini
#define	ww_acquire_init		linux_ww_acquire_init
#define	ww_mutex_destroy	linux_ww_mutex_destroy
#define	ww_mutex_init		linux_ww_mutex_init
#define	ww_mutex_is_locked	linux_ww_mutex_is_locked
#define	ww_mutex_lock		linux_ww_mutex_lock
#define	ww_mutex_lock_interruptible linux_ww_mutex_lock_interruptible
#define	ww_mutex_lock_slow	linux_ww_mutex_lock_slow
#define	ww_mutex_lock_slow_interruptible linux_ww_mutex_lock_slow_interruptible
#define	ww_mutex_trylock	linux_ww_mutex_trylock
#define	ww_mutex_unlock		linux_ww_mutex_unlock

void	ww_acquire_init(struct ww_acquire_ctx *, struct ww_class *);
void	ww_acquire_done(struct ww_acquire_ctx *);
void	ww_acquire_fini(struct ww_acquire_ctx *);

void	ww_mutex_init(struct ww_mutex *, struct ww_class *);
void	ww_mutex_destroy(struct ww_mutex *);

/*
 * WARNING: ww_mutex_is_locked returns true if it is locked by ANYONE.
 * Does NOT mean `Do I hold this lock?' (answering which really
 * requires an acquire context).
 */
bool	ww_mutex_is_locked(struct ww_mutex *);

int	ww_mutex_lock(struct ww_mutex *, struct ww_acquire_ctx *);
int	ww_mutex_lock_interruptible(struct ww_mutex *,
	    struct ww_acquire_ctx *);
void	ww_mutex_lock_slow(struct ww_mutex *, struct ww_acquire_ctx *);
int	ww_mutex_lock_slow_interruptible(struct ww_mutex *,
	    struct ww_acquire_ctx *);
int	ww_mutex_trylock(struct ww_mutex *);
void	ww_mutex_unlock(struct ww_mutex *);

#endif  /* _ASM_WW_MUTEX_H_ */
