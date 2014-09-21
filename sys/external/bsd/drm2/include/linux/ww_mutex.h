/*	$NetBSD: ww_mutex.h,v 1.4.4.1 2014/09/21 18:31:06 snj Exp $	*/

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
	uint64_t	wwx_ticket;
	unsigned	wwx_acquired;
	bool		wwx_acquire_done;
	struct rb_node	wwx_rb_node;
};

static inline int
ww_acquire_ctx_compare(void *cookie __unused, const void *va, const void *vb)
{
	const struct ww_acquire_ctx *const ctx_a = va;
	const struct ww_acquire_ctx *const ctx_b = vb;

	if (ctx_a->wwx_ticket < ctx_b->wwx_ticket)
		return -1;
	if (ctx_a->wwx_ticket > ctx_b->wwx_ticket)
		return -1;
	return 0;
}

static inline int
ww_acquire_ctx_compare_key(void *cookie __unused, const void *vn,
    const void *vk)
{
	const struct ww_acquire_ctx *const ctx = vn;
	const uint64_t *const ticketp = vk, ticket = *ticketp;

	if (ctx->wwx_ticket < ticket)
		return -1;
	if (ctx->wwx_ticket > ticket)
		return -1;
	return 0;
}

static const rb_tree_ops_t ww_acquire_ctx_rb_ops = {
	.rbto_compare_nodes = &ww_acquire_ctx_compare,
	.rbto_compare_key = &ww_acquire_ctx_compare_key,
	.rbto_node_offset = offsetof(struct ww_acquire_ctx, wwx_rb_node),
	.rbto_context = NULL,
};

static inline void
ww_acquire_init(struct ww_acquire_ctx *ctx, struct ww_class *class)
{

	ctx->wwx_class = class;
	ctx->wwx_ticket = atomic_inc_64_nv(&class->wwc_ticket);
	ctx->wwx_acquired = 0;
	ctx->wwx_acquire_done = false;
}

static inline void
ww_acquire_done(struct ww_acquire_ctx *ctx)
{

	ctx->wwx_acquire_done = true;
}

static inline void
ww_acquire_fini(struct ww_acquire_ctx *ctx)
{

	KASSERTMSG((ctx->wwx_acquired == 0), "ctx %p still holds %u locks",
	    ctx, ctx->wwx_acquired);
	ctx->wwx_acquired = ~0U;	/* Fail if called again. */
}

struct ww_mutex {
	kmutex_t		wwm_lock;
	enum ww_mutex_state {
		WW_UNLOCKED,
		WW_OWNED,
		WW_CTX,
		WW_WANTOWN,
	}			wwm_state;
	union {
		struct lwp		*owner;
		struct ww_acquire_ctx	*ctx;
	}			wwm_u;
	struct ww_class		*wwm_class;
	struct rb_tree		wwm_waiters;
	kcondvar_t		wwm_cv;
};

static inline void
ww_mutex_init(struct ww_mutex *mutex, struct ww_class *class)
{

	/*
	 * XXX Apparently Linux takes these with spin locks held.  That
	 * strikes me as a bad idea, but so it is...
	 */
	mutex_init(&mutex->wwm_lock, MUTEX_DEFAULT, IPL_VM);
	mutex->wwm_state = WW_UNLOCKED;
	mutex->wwm_class = class;
	rb_tree_init(&mutex->wwm_waiters, &ww_acquire_ctx_rb_ops);
	cv_init(&mutex->wwm_cv, "linuxwwm");
}

static inline void
ww_mutex_destroy(struct ww_mutex *mutex)
{

	cv_destroy(&mutex->wwm_cv);
#if 0
	rb_tree_destroy(&mutex->wwm_waiters, &ww_acquire_ctx_rb_ops);
#endif
	KASSERT(mutex->wwm_state == WW_UNLOCKED);
	mutex_destroy(&mutex->wwm_lock);
}

/*
 * XXX WARNING: This returns true if it is locked by ANYONE.  Does not
 * mean `Do I hold this lock?' (answering which really requires an
 * acquire context).
 */
static inline bool
ww_mutex_is_locked(struct ww_mutex *mutex)
{
	int locked;

	mutex_enter(&mutex->wwm_lock);
	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		locked = false;
		break;
	case WW_OWNED:
	case WW_CTX:
	case WW_WANTOWN:
		locked = true;
		break;
	default:
		panic("wait/wound mutex %p in bad state: %d", mutex,
		    (int)mutex->wwm_state);
	}
	mutex_exit(&mutex->wwm_lock);

	return locked;
}

static inline void
ww_mutex_state_wait(struct ww_mutex *mutex, enum ww_mutex_state state)
{

	KASSERT(mutex->wwm_state == state);
	do cv_wait(&mutex->wwm_cv, &mutex->wwm_lock);
	while (mutex->wwm_state == state);
}

static inline int
ww_mutex_state_wait_sig(struct ww_mutex *mutex, enum ww_mutex_state state)
{
	int ret;

	KASSERT(mutex->wwm_state == state);
	do {
		/* XXX errno NetBSD->Linux */
		ret = -cv_wait_sig(&mutex->wwm_cv, &mutex->wwm_lock);
		if (ret)
			break;
	} while (mutex->wwm_state == state);

	return ret;
}

static inline void
ww_mutex_lock_wait(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	struct ww_acquire_ctx *collision __diagused;

	KASSERT(mutex_owned(&mutex->wwm_lock));

	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERTMSG((ctx->wwx_class == mutex->wwm_u.ctx->wwx_class),
	    "ww mutex class mismatch: %p != %p",
	    ctx->wwx_class, mutex->wwm_u.ctx->wwx_class);
	KASSERTMSG((mutex->wwm_u.ctx->wwx_ticket != ctx->wwx_ticket),
	    "ticket number reused: %"PRId64" (%p) %"PRId64" (%p)",
	    ctx->wwx_ticket, ctx,
	    mutex->wwm_u.ctx->wwx_ticket, mutex->wwm_u.ctx);

	collision = rb_tree_insert_node(&mutex->wwm_waiters, ctx);
	KASSERTMSG((collision == ctx),
	    "ticket number reused: %"PRId64" (%p) %"PRId64" (%p)",
	    ctx->wwx_ticket, ctx, collision->wwx_ticket, collision);

	do cv_wait(&mutex->wwm_cv, &mutex->wwm_lock);
	while (!((mutex->wwm_state == WW_CTX) && (mutex->wwm_u.ctx == ctx)));

	rb_tree_remove_node(&mutex->wwm_waiters, ctx);
}

static inline int
ww_mutex_lock_wait_sig(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	struct ww_acquire_ctx *collision __diagused;
	int ret;

	KASSERT(mutex_owned(&mutex->wwm_lock));

	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERTMSG((ctx->wwx_class == mutex->wwm_u.ctx->wwx_class),
	    "ww mutex class mismatch: %p != %p",
	    ctx->wwx_class, mutex->wwm_u.ctx->wwx_class);
	KASSERTMSG((mutex->wwm_u.ctx->wwx_ticket != ctx->wwx_ticket),
	    "ticket number reused: %"PRId64" (%p) %"PRId64" (%p)",
	    ctx->wwx_ticket, ctx,
	    mutex->wwm_u.ctx->wwx_ticket, mutex->wwm_u.ctx);

	collision = rb_tree_insert_node(&mutex->wwm_waiters, ctx);
	KASSERTMSG((collision == ctx),
	    "ticket number reused: %"PRId64" (%p) %"PRId64" (%p)",
	    ctx->wwx_ticket, ctx, collision->wwx_ticket, collision);

	do {
		/* XXX errno NetBSD->Linux */
		ret = -cv_wait_sig(&mutex->wwm_cv, &mutex->wwm_lock);
		if (ret)
			goto out;
	} while (!((mutex->wwm_state == WW_CTX) && (mutex->wwm_u.ctx == ctx)));

out:	rb_tree_remove_node(&mutex->wwm_waiters, ctx);
	return ret;
}

static inline void
ww_mutex_lock_noctx(struct ww_mutex *mutex)
{

	mutex_enter(&mutex->wwm_lock);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_OWNED;
		mutex->wwm_u.owner = curlwp;
		break;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking against myself: %p", curlwp);
		ww_mutex_state_wait(mutex, WW_OWNED);
		goto retry;
	case WW_CTX:
		KASSERT(mutex->wwm_u.ctx != NULL);
		mutex->wwm_state = WW_WANTOWN;
	case WW_WANTOWN:
		ww_mutex_state_wait(mutex, WW_WANTOWN);
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_OWNED);
	KASSERT(mutex->wwm_u.owner == curlwp);
	mutex_exit(&mutex->wwm_lock);
}

static inline int
ww_mutex_lock_noctx_sig(struct ww_mutex *mutex)
{
	int ret;

	mutex_enter(&mutex->wwm_lock);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_OWNED;
		mutex->wwm_u.owner = curlwp;
		break;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking against myself: %p", curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_OWNED);
		if (ret)
			goto out;
		goto retry;
	case WW_CTX:
		KASSERT(mutex->wwm_u.ctx != NULL);
		mutex->wwm_state = WW_WANTOWN;
	case WW_WANTOWN:
		ret = ww_mutex_state_wait_sig(mutex, WW_WANTOWN);
		if (ret)
			goto out;
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_OWNED);
	KASSERT(mutex->wwm_u.owner == curlwp);
	ret = 0;
out:	mutex_exit(&mutex->wwm_lock);
	return ret;
}

static inline int
ww_mutex_lock(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{

	ASSERT_SLEEPABLE();

	if (ctx == NULL) {
		ww_mutex_lock_noctx(mutex);
		return 0;
	}

	KASSERT(!ctx->wwx_acquire_done);

	mutex_enter(&mutex->wwm_lock);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking against myself: %p", curlwp);
		ww_mutex_state_wait(mutex, WW_OWNED);
		goto retry;
	case WW_CTX:
		break;
	case WW_WANTOWN:
		ww_mutex_state_wait(mutex, WW_WANTOWN);
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx != NULL);
	if (mutex->wwm_u.ctx == ctx) {
		/*
		 * We already own it.  Yes, this can happen correctly
		 * for objects whose locking order is determined by
		 * userland.
		 */
		mutex_exit(&mutex->wwm_lock);
		return -EALREADY;
	} else if (mutex->wwm_u.ctx->wwx_ticket < ctx->wwx_ticket) {
		/*
		 * Owned by a higher-priority party.  Tell the caller
		 * to unlock everything and start over.
		 */
		KASSERTMSG((ctx->wwx_class == mutex->wwm_u.ctx->wwx_class),
		    "ww mutex class mismatch: %p != %p",
		    ctx->wwx_class, mutex->wwm_u.ctx->wwx_class);
		mutex_exit(&mutex->wwm_lock);
		return -EDEADLK;
	} else {
		/*
		 * Owned by a lower-priority party.  Ask that party to
		 * wake us when it is done or it realizes it needs to
		 * back off.
		 */
		ww_mutex_lock_wait(mutex, ctx);
	}
locked:	ctx->wwx_acquired++;
	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx == ctx);
	mutex_exit(&mutex->wwm_lock);
	return 0;
}

static inline int
ww_mutex_lock_interruptible(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	int ret;

	ASSERT_SLEEPABLE();

	if (ctx == NULL)
		return ww_mutex_lock_noctx_sig(mutex);

	KASSERT(!ctx->wwx_acquire_done);

	mutex_enter(&mutex->wwm_lock);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking against myself: %p", curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_OWNED);
		if (ret)
			goto out;
		goto retry;
	case WW_CTX:
		break;
	case WW_WANTOWN:
		ret = ww_mutex_state_wait_sig(mutex, WW_WANTOWN);
		if (ret)
			goto out;
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx != NULL);
	if (mutex->wwm_u.ctx == ctx) {
		/*
		 * We already own it.  Yes, this can happen correctly
		 * for objects whose locking order is determined by
		 * userland.
		 */
		mutex_exit(&mutex->wwm_lock);
		return -EALREADY;
	} else if (mutex->wwm_u.ctx->wwx_ticket < ctx->wwx_ticket) {
		/*
		 * Owned by a higher-priority party.  Tell the caller
		 * to unlock everything and start over.
		 */
		KASSERTMSG((ctx->wwx_class == mutex->wwm_u.ctx->wwx_class),
		    "ww mutex class mismatch: %p != %p",
		    ctx->wwx_class, mutex->wwm_u.ctx->wwx_class);
		mutex_exit(&mutex->wwm_lock);
		return -EDEADLK;
	} else {
		/*
		 * Owned by a lower-priority party.  Ask that party to
		 * wake us when it is done or it realizes it needs to
		 * back off.
		 */
		ret = ww_mutex_lock_wait_sig(mutex, ctx);
		if (ret)
			goto out;
	}
locked:	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx == ctx);
	ctx->wwx_acquired++;
	ret = 0;
out:	mutex_exit(&mutex->wwm_lock);
	return ret;
}

static inline void
ww_mutex_lock_slow(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{

	ASSERT_SLEEPABLE();

	if (ctx == NULL) {
		ww_mutex_lock_noctx(mutex);
		return;
	}

	KASSERT(!ctx->wwx_acquire_done);

	mutex_enter(&mutex->wwm_lock);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking against myself: %p", curlwp);
		ww_mutex_state_wait(mutex, WW_OWNED);
		goto retry;
	case WW_CTX:
		break;
	case WW_WANTOWN:
		ww_mutex_state_wait(mutex, WW_WANTOWN);
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx != NULL);
	/*
	 * Owned by another party, of any priority.  Ask that party to
	 * wake us when it's done.
	 */
	ww_mutex_lock_wait(mutex, ctx);
locked:	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx == ctx);
	ctx->wwx_acquired++;
	mutex_exit(&mutex->wwm_lock);
}

static inline int
ww_mutex_lock_slow_interruptible(struct ww_mutex *mutex,
    struct ww_acquire_ctx *ctx)
{
	int ret;

	ASSERT_SLEEPABLE();

	if (ctx == NULL)
		return ww_mutex_lock_noctx_sig(mutex);

	KASSERT(!ctx->wwx_acquire_done);

	mutex_enter(&mutex->wwm_lock);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking against myself: %p", curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_OWNED);
		if (ret)
			goto out;
		goto retry;
	case WW_CTX:
		break;
	case WW_WANTOWN:
		ret = ww_mutex_state_wait_sig(mutex, WW_WANTOWN);
		if (ret)
			goto out;
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx != NULL);
	/*
	 * Owned by another party, of any priority.  Ask that party to
	 * wake us when it's done.
	 */
	ret = ww_mutex_lock_wait_sig(mutex, ctx);
	if (ret)
		goto out;
locked:	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx == ctx);
	ctx->wwx_acquired++;
	ret = 0;
out:	mutex_exit(&mutex->wwm_lock);
	return ret;
}

static inline int
ww_mutex_trylock(struct ww_mutex *mutex)
{
	int ret;

	mutex_enter(&mutex->wwm_lock);
	if (mutex->wwm_state == WW_UNLOCKED) {
		mutex->wwm_state = WW_OWNED;
		mutex->wwm_u.owner = curlwp;
		ret = 1;
	} else {
		ret = 0;
	}
	mutex_exit(&mutex->wwm_lock);

	return ret;
}

static inline void
ww_mutex_unlock(struct ww_mutex *mutex)
{
	struct ww_acquire_ctx *ctx;

	mutex_enter(&mutex->wwm_lock);
	KASSERT(mutex->wwm_state != WW_UNLOCKED);
	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		panic("unlocking unlocked wait/wound mutex: %p", mutex);
	case WW_OWNED:
		/* Let the context lockers fight over it.  */
		mutex->wwm_u.owner = NULL;
		mutex->wwm_state = WW_UNLOCKED;
		break;
	case WW_CTX:
		KASSERT(mutex->wwm_u.ctx != NULL);
		mutex->wwm_u.ctx->wwx_acquired--;
		mutex->wwm_u.ctx = NULL;
		/*
		 * If there are any waiters with contexts, grant the
		 * lock to the highest-priority one.  Otherwise, just
		 * unlock it.
		 */
		if ((ctx = RB_TREE_MIN(&mutex->wwm_waiters)) != NULL) {
			mutex->wwm_state = WW_CTX;
			mutex->wwm_u.ctx = ctx;
		} else {
			mutex->wwm_state = WW_UNLOCKED;
		}
		break;
	case WW_WANTOWN:
		/* Let the non-context lockers fight over it.  */
		mutex->wwm_state = WW_UNLOCKED;
		break;
	}
	cv_broadcast(&mutex->wwm_cv);
	mutex_exit(&mutex->wwm_lock);
}

#endif  /* _ASM_WW_MUTEX_H_ */
