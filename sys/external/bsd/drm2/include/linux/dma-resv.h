/*	$NetBSD: dma-resv.h,v 1.7 2021/12/19 10:37:47 riastradh Exp $	*/

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

#ifndef	_LINUX_DMA_RESV_H_
#define	_LINUX_DMA_RESV_H_

#include <linux/dma-fence.h>
#include <linux/rcupdate.h>
#include <linux/seqlock.h>
#include <linux/ww_mutex.h>

struct dma_resv {
	struct ww_mutex				lock;
	struct seqcount				seq;
	struct dma_fence __rcu			*fence_excl;
	struct dma_resv_list __rcu		*fence;

	struct dma_resv_list __rcu		*robj_prealloc;
};

struct dma_resv_list {
	struct rcu_head		rol_rcu;

	uint32_t		shared_count;
	uint32_t		shared_max;
	struct dma_fence __rcu	*shared[];
};

/* NetBSD addition */
struct dma_resv_poll {
	kmutex_t		rp_lock;
	struct selinfo		rp_selq;
	struct dma_fence_cb		rp_fcb;
	bool			rp_claimed;
};

#define	dma_resv_add_excl_fence		linux_dma_resv_add_excl_fence
#define	dma_resv_add_shared_fence	linux_dma_resv_add_shared_fence
#define	dma_resv_assert_held		linux_dma_resv_assert_held
#define	dma_resv_copy_fences		linux_dma_resv_copy_fences
#define	dma_resv_do_poll		linux_dma_resv_do_poll
#define	dma_resv_fini			linux_dma_resv_fini
#define	dma_resv_get_excl		linux_dma_resv_get_excl
#define	dma_resv_get_excl_rcu		linux_dma_resv_get_excl_rcu
#define	dma_resv_get_fences_rcu		linux_dma_resv_get_fences_rcu
#define	dma_resv_get_list		linux_dma_resv_get_list
#define	dma_resv_held			linux_dma_resv_held
#define	dma_resv_init			linux_dma_resv_init
#define	dma_resv_kqfilter		linux_dma_resv_kqfilter
#define	dma_resv_lock			linux_dma_resv_lock
#define	dma_resv_lock_interruptible	linux_dma_resv_lock_interruptible
#define	dma_resv_lock_slow		linux_dma_resv_lock_slow
#define	dma_resv_lock_slow_interruptible linux_dma_resv_lock_slow_interruptible
#define	dma_resv_reserve_shared		linux_dma_resv_reserve_shared
#define	dma_resv_test_signaled_rcu	linux_dma_resv_test_signaled_rcu
#define	dma_resv_trylock		linux_dma_resv_trylock
#define	dma_resv_unlock			linux_dma_resv_unlock
#define	dma_resv_wait_timeout_rcu	linux_dma_resv_wait_timeout_rcu
#define	dma_resv_poll_fini		linux_dma_resv_poll_fini
#define	dma_resv_poll_init		linux_dma_resv_poll_init
#define	reservation_ww_class		linux_reservation_ww_class

extern struct ww_class	reservation_ww_class;

void	dma_resv_init(struct dma_resv *);
void	dma_resv_fini(struct dma_resv *);
int	dma_resv_lock(struct dma_resv *,
	    struct ww_acquire_ctx *);
void	dma_resv_lock_slow(struct dma_resv *,
	    struct ww_acquire_ctx *);
int	dma_resv_lock_interruptible(struct dma_resv *,
	    struct ww_acquire_ctx *);
int	dma_resv_lock_slow_interruptible(struct dma_resv *,
	    struct ww_acquire_ctx *);
bool	dma_resv_trylock(struct dma_resv *) __must_check;
void	dma_resv_unlock(struct dma_resv *);
bool	dma_resv_held(struct dma_resv *);
void	dma_resv_assert_held(struct dma_resv *);
struct dma_fence *
	dma_resv_get_excl(struct dma_resv *);
struct dma_resv_list *
	dma_resv_get_list(struct dma_resv *);
int	dma_resv_reserve_shared(struct dma_resv *, unsigned int);
void	dma_resv_add_excl_fence(struct dma_resv *,
	    struct dma_fence *);
void	dma_resv_add_shared_fence(struct dma_resv *,
	    struct dma_fence *);

struct dma_fence *
	dma_resv_get_excl_rcu(const struct dma_resv *);
int	dma_resv_get_fences_rcu(const struct dma_resv *,
	    struct dma_fence **, unsigned *, struct dma_fence ***);

int	dma_resv_copy_fences(struct dma_resv *,
	    const struct dma_resv *);

bool	dma_resv_test_signaled_rcu(const struct dma_resv *,
	    bool);
long	dma_resv_wait_timeout_rcu(const struct dma_resv *,
	    bool, bool, unsigned long);

/* NetBSD additions */
void	dma_resv_poll_init(struct dma_resv_poll *);
void	dma_resv_poll_fini(struct dma_resv_poll *);
int	dma_resv_do_poll(const struct dma_resv *, int,
	    struct dma_resv_poll *);
int	dma_resv_kqfilter(const struct dma_resv *,
	    struct knote *, struct dma_resv_poll *);

static inline bool
dma_resv_has_excl_fence(const struct dma_resv *robj)
{
	return robj->fence_excl != NULL;
}

#endif	/* _LINUX_DMA_RESV_H_ */
