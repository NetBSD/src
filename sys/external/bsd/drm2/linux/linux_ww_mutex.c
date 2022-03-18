/*	$NetBSD: linux_ww_mutex.c,v 1.14 2022/03/18 23:33:41 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_ww_mutex.c,v 1.14 2022/03/18 23:33:41 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/lockdebug.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/rbtree.h>

#include <linux/ww_mutex.h>
#include <linux/errno.h>

#define	WW_WANTLOCK(WW)							      \
	LOCKDEBUG_WANTLOCK((WW)->wwm_debug, (WW),			      \
	    (uintptr_t)__builtin_return_address(0), 0)
#define	WW_LOCKED(WW)							      \
	LOCKDEBUG_LOCKED((WW)->wwm_debug, (WW), NULL,			      \
	    (uintptr_t)__builtin_return_address(0), 0)
#define	WW_UNLOCKED(WW)							      \
	LOCKDEBUG_UNLOCKED((WW)->wwm_debug, (WW),			      \
	    (uintptr_t)__builtin_return_address(0), 0)

static int
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

static int
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

void
ww_acquire_init(struct ww_acquire_ctx *ctx, struct ww_class *class)
{

	ctx->wwx_class = class;
	ctx->wwx_owner = curlwp;
	ctx->wwx_ticket = atomic64_inc_return(&class->wwc_ticket);
	ctx->wwx_acquired = 0;
	ctx->wwx_acquire_done = false;
}

void
ww_acquire_done(struct ww_acquire_ctx *ctx)
{

	KASSERTMSG((ctx->wwx_owner == curlwp),
	    "ctx %p owned by %p, not self (%p)", ctx, ctx->wwx_owner, curlwp);

	ctx->wwx_acquire_done = true;
}

static void
ww_acquire_done_check(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{

	/*
	 * If caller has invoked ww_acquire_done, we must already hold
	 * this mutex.
	 */
	KASSERT(mutex_owned(&mutex->wwm_lock));
	KASSERTMSG((!ctx->wwx_acquire_done ||
		(mutex->wwm_state == WW_CTX && mutex->wwm_u.ctx == ctx)),
	    "ctx %p done acquiring locks, refusing to acquire %p",
	    ctx, mutex);
}

void
ww_acquire_fini(struct ww_acquire_ctx *ctx)
{

	KASSERTMSG((ctx->wwx_owner == curlwp),
	    "ctx %p owned by %p, not self (%p)", ctx, ctx->wwx_owner, curlwp);
	KASSERTMSG((ctx->wwx_acquired == 0), "ctx %p still holds %u locks",
	    ctx, ctx->wwx_acquired);

	ctx->wwx_acquired = ~0U;	/* Fail if called again. */
	ctx->wwx_owner = NULL;
}

#ifdef LOCKDEBUG
static void
ww_dump(const volatile void *cookie, lockop_printer_t pr)
{
	const volatile struct ww_mutex *mutex = cookie;

	pr("%-13s: ", "state");
	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		pr("unlocked\n");
		break;
	case WW_OWNED:
		pr("owned by lwp\n");
		pr("%-13s: %p\n", "owner", mutex->wwm_u.owner);
		pr("%-13s: %s\n", "waiters",
		    cv_has_waiters((void *)(intptr_t)&mutex->wwm_cv)
			? "yes" : "no");
		break;
	case WW_CTX:
		pr("owned via ctx\n");
		pr("%-13s: %p\n", "context", mutex->wwm_u.ctx);
		pr("%-13s: %p\n", "lwp",
		    mutex->wwm_u.ctx->wwx_owner);
		pr("%-13s: %s\n", "waiters",
		    cv_has_waiters((void *)(intptr_t)&mutex->wwm_cv)
			? "yes" : "no");
		break;
	case WW_WANTOWN:
		pr("owned via ctx\n");
		pr("%-13s: %p\n", "context", mutex->wwm_u.ctx);
		pr("%-13s: %p\n", "lwp",
		    mutex->wwm_u.ctx->wwx_owner);
		pr("%-13s: %s\n", "waiters", "yes (noctx)");
		break;
	default:
		pr("unknown\n");
		break;
	}
}

static lockops_t ww_lockops = {
	.lo_name = "Wait/wound mutex",
	.lo_type = LOCKOPS_SLEEP,
	.lo_dump = ww_dump,
};
#endif

/*
 * ww_mutex_init(mutex, class)
 *
 *	Initialize mutex in the given class.  Must precede any other
 *	ww_mutex_* operations.  After done, mutex must be destroyed
 *	with ww_mutex_destroy.
 */
void
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
#ifdef LOCKDEBUG
	mutex->wwm_debug = LOCKDEBUG_ALLOC(mutex, &ww_lockops,
	    (uintptr_t)__builtin_return_address(0));
#endif
}

/*
 * ww_mutex_destroy(mutex)
 *
 *	Destroy mutex initialized by ww_mutex_init.  Caller must not be
 *	with any other ww_mutex_* operations except after
 *	reinitializing with ww_mutex_init.
 */
void
ww_mutex_destroy(struct ww_mutex *mutex)
{

	KASSERT(mutex->wwm_state == WW_UNLOCKED);

#ifdef LOCKDEBUG
	LOCKDEBUG_FREE(mutex->wwm_debug, mutex);
#endif
	cv_destroy(&mutex->wwm_cv);
#if 0
	rb_tree_destroy(&mutex->wwm_waiters, &ww_acquire_ctx_rb_ops);
#endif
	KASSERT(mutex->wwm_state == WW_UNLOCKED);
	mutex_destroy(&mutex->wwm_lock);
}

/*
 * ww_mutex_is_locked(mutex)
 *
 *	True if anyone holds mutex locked at the moment, false if not.
 *	Answer is stale as soon returned unless mutex is held by
 *	caller.
 *
 *	XXX WARNING: This returns true if it is locked by ANYONE.  Does
 *	not mean `Do I hold this lock?' (answering which really
 *	requires an acquire context).
 */
bool
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

/*
 * ww_mutex_state_wait(mutex, state)
 *
 *	Wait for mutex, which must be in the given state, to transition
 *	to another state.  Uninterruptible; never fails.
 *
 *	Caller must hold mutex's internal lock.
 *
 *	May sleep.
 *
 *	Internal subroutine.
 */
static void
ww_mutex_state_wait(struct ww_mutex *mutex, enum ww_mutex_state state)
{

	KASSERT(mutex_owned(&mutex->wwm_lock));
	KASSERT(mutex->wwm_state == state);
	do cv_wait(&mutex->wwm_cv, &mutex->wwm_lock);
	while (mutex->wwm_state == state);
}

/*
 * ww_mutex_state_wait_sig(mutex, state)
 *
 *	Wait for mutex, which must be in the given state, to transition
 *	to another state, or fail if interrupted by a signal.  Return 0
 *	on success, -EINTR if interrupted by a signal.
 *
 *	Caller must hold mutex's internal lock.
 *
 *	May sleep.
 *
 *	Internal subroutine.
 */
static int
ww_mutex_state_wait_sig(struct ww_mutex *mutex, enum ww_mutex_state state)
{
	int ret;

	KASSERT(mutex_owned(&mutex->wwm_lock));
	KASSERT(mutex->wwm_state == state);
	do {
		/* XXX errno NetBSD->Linux */
		ret = -cv_wait_sig(&mutex->wwm_cv, &mutex->wwm_lock);
		if (ret) {
			KASSERTMSG((ret == -EINTR || ret == -ERESTART),
			    "ret=%d", ret);
			ret = -EINTR;
			break;
		}
	} while (mutex->wwm_state == state);

	KASSERTMSG((ret == 0 || ret == -EINTR), "ret=%d", ret);
	return ret;
}

/*
 * ww_mutex_lock_wait(mutex, ctx)
 *
 *	With mutex locked and in the WW_CTX or WW_WANTOWN state, owned
 *	by another thread with an acquire context, wait to acquire
 *	mutex.  While waiting, record ctx in the tree of waiters.  Does
 *	not update the mutex state otherwise.
 *
 *	Caller must not already hold mutex.  Caller must hold mutex's
 *	internal lock.  Uninterruptible; never fails.
 *
 *	May sleep.
 *
 *	Internal subroutine.
 */
static void
ww_mutex_lock_wait(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	struct ww_acquire_ctx *collision __diagused;

	KASSERT(mutex_owned(&mutex->wwm_lock));

	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx != ctx);
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
	while (!(((mutex->wwm_state == WW_CTX) ||
		    (mutex->wwm_state == WW_WANTOWN)) &&
		 (mutex->wwm_u.ctx == ctx)));

	rb_tree_remove_node(&mutex->wwm_waiters, ctx);
}

/*
 * ww_mutex_lock_wait_sig(mutex, ctx)
 *
 *	With mutex locked and in the WW_CTX or WW_WANTOWN state, owned
 *	by another thread with an acquire context, wait to acquire
 *	mutex and return 0, or return -EINTR if interrupted by a
 *	signal.  While waiting, record ctx in the tree of waiters.
 *	Does not update the mutex state otherwise.
 *
 *	Caller must not already hold mutex.  Caller must hold mutex's
 *	internal lock.
 *
 *	May sleep.
 *
 *	Internal subroutine.
 */
static int
ww_mutex_lock_wait_sig(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	struct ww_acquire_ctx *collision __diagused;
	int ret;

	KASSERT(mutex_owned(&mutex->wwm_lock));

	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx != ctx);
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
		if (ret) {
			KASSERTMSG((ret == -EINTR || ret == -ERESTART),
			    "ret=%d", ret);
			ret = -EINTR;
			goto out;
		}
	} while (!(((mutex->wwm_state == WW_CTX) ||
		    (mutex->wwm_state == WW_WANTOWN)) &&
		(mutex->wwm_u.ctx == ctx)));

out:	rb_tree_remove_node(&mutex->wwm_waiters, ctx);
	KASSERTMSG((ret == 0 || ret == -EINTR), "ret=%d", ret);
	return ret;
}

/*
 * ww_mutex_lock_noctx(mutex)
 *
 *	Acquire mutex without an acquire context.  Caller must not
 *	already hold the mutex.  Uninterruptible; never fails.
 *
 *	May sleep.
 *
 *	Internal subroutine, implementing ww_mutex_lock(..., NULL).
 */
static void
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
		    "locking %p against myself: %p", mutex, curlwp);
		ww_mutex_state_wait(mutex, WW_OWNED);
		goto retry;
	case WW_CTX:
		KASSERT(mutex->wwm_u.ctx != NULL);
		mutex->wwm_state = WW_WANTOWN;
		/* FALLTHROUGH */
	case WW_WANTOWN:
		KASSERTMSG((mutex->wwm_u.ctx->wwx_owner != curlwp),
		    "locking %p against myself: %p", mutex, curlwp);
		ww_mutex_state_wait(mutex, WW_WANTOWN);
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_OWNED);
	KASSERT(mutex->wwm_u.owner == curlwp);
	WW_LOCKED(mutex);
	mutex_exit(&mutex->wwm_lock);
}

/*
 * ww_mutex_lock_noctx_sig(mutex)
 *
 *	Acquire mutex without an acquire context and return 0, or fail
 *	and return -EINTR if interrupted by a signal.  Caller must not
 *	already hold the mutex.
 *
 *	May sleep.
 *
 *	Internal subroutine, implementing
 *	ww_mutex_lock_interruptible(..., NULL).
 */
static int
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
		    "locking %p against myself: %p", mutex, curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_OWNED);
		if (ret) {
			KASSERTMSG(ret == -EINTR, "ret=%d", ret);
			goto out;
		}
		goto retry;
	case WW_CTX:
		KASSERT(mutex->wwm_u.ctx != NULL);
		mutex->wwm_state = WW_WANTOWN;
		/* FALLTHROUGH */
	case WW_WANTOWN:
		KASSERTMSG((mutex->wwm_u.ctx->wwx_owner != curlwp),
		    "locking %p against myself: %p", mutex, curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_WANTOWN);
		if (ret) {
			KASSERTMSG(ret == -EINTR, "ret=%d", ret);
			goto out;
		}
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	KASSERT(mutex->wwm_state == WW_OWNED);
	KASSERT(mutex->wwm_u.owner == curlwp);
	WW_LOCKED(mutex);
	ret = 0;
out:	mutex_exit(&mutex->wwm_lock);
	KASSERTMSG((ret == 0 || ret == -EINTR), "ret=%d", ret);
	return ret;
}

/*
 * ww_mutex_lock(mutex, ctx)
 *
 *	Lock the mutex and return 0, or fail if impossible.
 *
 *	- If ctx is null, caller must not hold mutex, and ww_mutex_lock
 *	  always succeeds and returns 0.
 *
 *	- If ctx is nonnull, then:
 *	  . Fail with -EALREADY if caller already holds mutex.
 *	  . Fail with -EDEADLK if someone else holds mutex but there is
 *	    a cycle.
 *
 *	May sleep.
 */
int
ww_mutex_lock(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	int ret;

	/*
	 * We do not WW_WANTLOCK at the beginning because we may
	 * correctly already hold it, if we have a context, in which
	 * case we must return EALREADY to the caller.
	 */
	ASSERT_SLEEPABLE();

	if (ctx == NULL) {
		WW_WANTLOCK(mutex);
		ww_mutex_lock_noctx(mutex);
		ret = 0;
		goto out;
	}

	KASSERTMSG((ctx->wwx_owner == curlwp),
	    "ctx %p owned by %p, not self (%p)", ctx, ctx->wwx_owner, curlwp);
	KASSERTMSG((ctx->wwx_acquired != ~0U),
	    "ctx %p finished, can't be used any more", ctx);
	KASSERTMSG((ctx->wwx_class == mutex->wwm_class),
	    "ctx %p in class %p, mutex %p in class %p",
	    ctx, ctx->wwx_class, mutex, mutex->wwm_class);

	mutex_enter(&mutex->wwm_lock);
	ww_acquire_done_check(mutex, ctx);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		WW_WANTLOCK(mutex);
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		WW_WANTLOCK(mutex);
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking %p against myself: %p", mutex, curlwp);
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
	KASSERT((mutex->wwm_u.ctx == ctx) ||
	    (mutex->wwm_u.ctx->wwx_owner != curlwp));

	if (mutex->wwm_u.ctx == ctx) {
		/*
		 * We already own it.  Yes, this can happen correctly
		 * for objects whose locking order is determined by
		 * userland.
		 */
		ret = -EALREADY;
		goto out_unlock;
	}

	/*
	 * We do not own it.  We can safely assert to LOCKDEBUG that we
	 * want it.
	 */
	WW_WANTLOCK(mutex);

	if (mutex->wwm_u.ctx->wwx_ticket < ctx->wwx_ticket) {
		/*
		 * Owned by a higher-priority party.  Tell the caller
		 * to unlock everything and start over.
		 */
		KASSERTMSG((ctx->wwx_class == mutex->wwm_u.ctx->wwx_class),
		    "ww mutex class mismatch: %p != %p",
		    ctx->wwx_class, mutex->wwm_u.ctx->wwx_class);
		ret = -EDEADLK;
		goto out_unlock;
	}

	/*
	 * Owned by a lower-priority party.  Ask that party to wake us
	 * when it is done or it realizes it needs to back off.
	 */
	ww_mutex_lock_wait(mutex, ctx);

locked:	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx == ctx);
	WW_LOCKED(mutex);
	ctx->wwx_acquired++;
	ret = 0;
out_unlock:
	mutex_exit(&mutex->wwm_lock);
out:	KASSERTMSG((ret == 0 || ret == -EALREADY || ret == -EDEADLK),
	    "ret=%d", ret);
	return ret;
}

/*
 * ww_mutex_lock_interruptible(mutex, ctx)
 *
 *	Lock the mutex and return 0, or fail if impossible or
 *	interrupted.
 *
 *	- If ctx is null, caller must not hold mutex, and ww_mutex_lock
 *	  always succeeds and returns 0.
 *
 *	- If ctx is nonnull, then:
 *	  . Fail with -EALREADY if caller already holds mutex.
 *	  . Fail with -EDEADLK if someone else holds mutex but there is
 *	    a cycle.
 *	  . Fail with -EINTR if interrupted by a signal.
 *
 *	May sleep.
 */
int
ww_mutex_lock_interruptible(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{
	int ret;

	/*
	 * We do not WW_WANTLOCK at the beginning because we may
	 * correctly already hold it, if we have a context, in which
	 * case we must return EALREADY to the caller.
	 */
	ASSERT_SLEEPABLE();

	if (ctx == NULL) {
		WW_WANTLOCK(mutex);
		ret = ww_mutex_lock_noctx_sig(mutex);
		KASSERTMSG((ret == 0 || ret == -EINTR), "ret=%d", ret);
		goto out;
	}

	KASSERTMSG((ctx->wwx_owner == curlwp),
	    "ctx %p owned by %p, not self (%p)", ctx, ctx->wwx_owner, curlwp);
	KASSERTMSG((ctx->wwx_acquired != ~0U),
	    "ctx %p finished, can't be used any more", ctx);
	KASSERTMSG((ctx->wwx_class == mutex->wwm_class),
	    "ctx %p in class %p, mutex %p in class %p",
	    ctx, ctx->wwx_class, mutex, mutex->wwm_class);

	mutex_enter(&mutex->wwm_lock);
	ww_acquire_done_check(mutex, ctx);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		WW_WANTLOCK(mutex);
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		WW_WANTLOCK(mutex);
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking %p against myself: %p", mutex, curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_OWNED);
		if (ret) {
			KASSERTMSG(ret == -EINTR, "ret=%d", ret);
			goto out_unlock;
		}
		goto retry;
	case WW_CTX:
		break;
	case WW_WANTOWN:
		ret = ww_mutex_state_wait_sig(mutex, WW_WANTOWN);
		if (ret) {
			KASSERTMSG(ret == -EINTR, "ret=%d", ret);
			goto out_unlock;
		}
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}

	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx != NULL);
	KASSERT((mutex->wwm_u.ctx == ctx) ||
	    (mutex->wwm_u.ctx->wwx_owner != curlwp));

	if (mutex->wwm_u.ctx == ctx) {
		/*
		 * We already own it.  Yes, this can happen correctly
		 * for objects whose locking order is determined by
		 * userland.
		 */
		ret = -EALREADY;
		goto out_unlock;
	}

	/*
	 * We do not own it.  We can safely assert to LOCKDEBUG that we
	 * want it.
	 */
	WW_WANTLOCK(mutex);

	if (mutex->wwm_u.ctx->wwx_ticket < ctx->wwx_ticket) {
		/*
		 * Owned by a higher-priority party.  Tell the caller
		 * to unlock everything and start over.
		 */
		KASSERTMSG((ctx->wwx_class == mutex->wwm_u.ctx->wwx_class),
		    "ww mutex class mismatch: %p != %p",
		    ctx->wwx_class, mutex->wwm_u.ctx->wwx_class);
		ret = -EDEADLK;
		goto out_unlock;
	}

	/*
	 * Owned by a lower-priority party.  Ask that party to wake us
	 * when it is done or it realizes it needs to back off.
	 */
	ret = ww_mutex_lock_wait_sig(mutex, ctx);
	if (ret) {
		KASSERTMSG(ret == -EINTR, "ret=%d", ret);
		goto out_unlock;
	}

locked:	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx == ctx);
	WW_LOCKED(mutex);
	ctx->wwx_acquired++;
	ret = 0;
out_unlock:
	mutex_exit(&mutex->wwm_lock);
out:	KASSERTMSG((ret == 0 || ret == -EALREADY || ret == -EDEADLK ||
		ret == -EINTR), "ret=%d", ret);
	return ret;
}

/*
 * ww_mutex_lock_slow(mutex, ctx)
 *
 *	Slow path: After ww_mutex_lock* has failed with -EDEADLK, and
 *	after the caller has ditched all its locks, wait for the owner
 *	of mutex to relinquish mutex before the caller can start over
 *	acquiring locks again.
 *
 *	Uninterruptible; never fails.
 *
 *	May sleep.
 */
void
ww_mutex_lock_slow(struct ww_mutex *mutex, struct ww_acquire_ctx *ctx)
{

	/* Caller must not try to lock against self here.  */
	WW_WANTLOCK(mutex);
	ASSERT_SLEEPABLE();

	if (ctx == NULL) {
		ww_mutex_lock_noctx(mutex);
		return;
	}

	KASSERTMSG((ctx->wwx_owner == curlwp),
	    "ctx %p owned by %p, not self (%p)", ctx, ctx->wwx_owner, curlwp);
	KASSERTMSG((ctx->wwx_acquired != ~0U),
	    "ctx %p finished, can't be used any more", ctx);
	KASSERTMSG((ctx->wwx_acquired == 0),
	    "ctx %p still holds %u locks, not allowed in slow path",
	    ctx, ctx->wwx_acquired);
	KASSERTMSG((ctx->wwx_class == mutex->wwm_class),
	    "ctx %p in class %p, mutex %p in class %p",
	    ctx, ctx->wwx_class, mutex, mutex->wwm_class);

	mutex_enter(&mutex->wwm_lock);
	ww_acquire_done_check(mutex, ctx);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking %p against myself: %p", mutex, curlwp);
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
	KASSERTMSG((mutex->wwm_u.ctx->wwx_owner != curlwp),
	    "locking %p against myself: %p", mutex, curlwp);

	/*
	 * Owned by another party, of any priority.  Ask that party to
	 * wake us when it's done.
	 */
	ww_mutex_lock_wait(mutex, ctx);

locked:	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx == ctx);
	WW_LOCKED(mutex);
	ctx->wwx_acquired++;
	mutex_exit(&mutex->wwm_lock);
}

/*
 * ww_mutex_lock_slow(mutex, ctx)
 *
 *	Slow path: After ww_mutex_lock* has failed with -EDEADLK, and
 *	after the caller has ditched all its locks, wait for the owner
 *	of mutex to relinquish mutex before the caller can start over
 *	acquiring locks again, or fail with -EINTR if interrupted by a
 *	signal.
 *
 *	May sleep.
 */
int
ww_mutex_lock_slow_interruptible(struct ww_mutex *mutex,
    struct ww_acquire_ctx *ctx)
{
	int ret;

	WW_WANTLOCK(mutex);
	ASSERT_SLEEPABLE();

	if (ctx == NULL) {
		ret = ww_mutex_lock_noctx_sig(mutex);
		KASSERTMSG((ret == 0 || ret == -EINTR), "ret=%d", ret);
		goto out;
	}

	KASSERTMSG((ctx->wwx_owner == curlwp),
	    "ctx %p owned by %p, not self (%p)", ctx, ctx->wwx_owner, curlwp);
	KASSERTMSG((ctx->wwx_acquired != ~0U),
	    "ctx %p finished, can't be used any more", ctx);
	KASSERTMSG((ctx->wwx_acquired == 0),
	    "ctx %p still holds %u locks, not allowed in slow path",
	    ctx, ctx->wwx_acquired);
	KASSERTMSG((ctx->wwx_class == mutex->wwm_class),
	    "ctx %p in class %p, mutex %p in class %p",
	    ctx, ctx->wwx_class, mutex, mutex->wwm_class);

	mutex_enter(&mutex->wwm_lock);
	ww_acquire_done_check(mutex, ctx);
retry:	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		mutex->wwm_state = WW_CTX;
		mutex->wwm_u.ctx = ctx;
		goto locked;
	case WW_OWNED:
		KASSERTMSG((mutex->wwm_u.owner != curlwp),
		    "locking %p against myself: %p", mutex, curlwp);
		ret = ww_mutex_state_wait_sig(mutex, WW_OWNED);
		if (ret) {
			KASSERTMSG(ret == -EINTR, "ret=%d", ret);
			goto out_unlock;
		}
		goto retry;
	case WW_CTX:
		break;
	case WW_WANTOWN:
		ret = ww_mutex_state_wait_sig(mutex, WW_WANTOWN);
		if (ret) {
			KASSERTMSG(ret == -EINTR, "ret=%d", ret);
			goto out_unlock;
		}
		goto retry;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}

	KASSERT(mutex->wwm_state == WW_CTX);
	KASSERT(mutex->wwm_u.ctx != NULL);
	KASSERTMSG((mutex->wwm_u.ctx->wwx_owner != curlwp),
	    "locking %p against myself: %p", mutex, curlwp);

	/*
	 * Owned by another party, of any priority.  Ask that party to
	 * wake us when it's done.
	 */
	ret = ww_mutex_lock_wait_sig(mutex, ctx);
	if (ret) {
		KASSERTMSG(ret == -EINTR, "ret=%d", ret);
		goto out_unlock;
	}

locked:	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx == ctx);
	WW_LOCKED(mutex);
	ctx->wwx_acquired++;
	ret = 0;
out_unlock:
	mutex_exit(&mutex->wwm_lock);
out:	KASSERTMSG((ret == 0 || ret == -EINTR), "ret=%d", ret);
	return ret;
}

/*
 * ww_mutex_trylock(mutex)
 *
 *	Tro to acquire mutex and return 1, but if it can't be done
 *	immediately, return 0.
 */
int
ww_mutex_trylock(struct ww_mutex *mutex)
{
	int ret;

	mutex_enter(&mutex->wwm_lock);
	if (mutex->wwm_state == WW_UNLOCKED) {
		mutex->wwm_state = WW_OWNED;
		mutex->wwm_u.owner = curlwp;
		WW_WANTLOCK(mutex);
		WW_LOCKED(mutex);
		ret = 1;
	} else {
		/*
		 * It is tempting to assert that we do not hold the
		 * mutex here, because trylock when we hold the lock
		 * already generally indicates a bug in the design of
		 * the code.  However, it seems that Linux relies on
		 * this deep in ttm buffer reservation logic, so these
		 * assertions are disabled until we find another way to
		 * work around that or fix the bug that leads to it.
		 *
		 * That said: we should not be in the WW_WANTOWN state,
		 * which happens only while we're in the ww mutex logic
		 * waiting to acquire the lock.
		 */
#if 0
		KASSERTMSG(((mutex->wwm_state != WW_OWNED) ||
		    (mutex->wwm_u.owner != curlwp)),
		    "locking %p against myself: %p", mutex, curlwp);
		KASSERTMSG(((mutex->wwm_state != WW_CTX) ||
		    (mutex->wwm_u.ctx->wwx_owner != curlwp)),
		    "locking %p against myself: %p", mutex, curlwp);
#endif
		KASSERTMSG(((mutex->wwm_state != WW_WANTOWN) ||
		    (mutex->wwm_u.ctx->wwx_owner != curlwp)),
		    "locking %p against myself: %p", mutex, curlwp);
		ret = 0;
	}
	mutex_exit(&mutex->wwm_lock);

	return ret;
}

/*
 * ww_mutex_unlock_release(mutex)
 *
 *	Decrement the number of mutexes acquired in the current locking
 *	context of mutex, which must be held by the caller and in
 *	WW_CTX or WW_WANTOWN state, and clear the mutex's reference.
 *	Caller must hold the internal lock of mutex, and is responsible
 *	for notifying waiters.
 *
 *	Internal subroutine.
 */
static void
ww_mutex_unlock_release(struct ww_mutex *mutex)
{

	KASSERT(mutex_owned(&mutex->wwm_lock));
	KASSERT((mutex->wwm_state == WW_CTX) ||
	    (mutex->wwm_state == WW_WANTOWN));
	KASSERT(mutex->wwm_u.ctx != NULL);
	KASSERTMSG((mutex->wwm_u.ctx->wwx_owner == curlwp),
	    "ww_mutex %p ctx %p held by %p, not by self (%p)",
	    mutex, mutex->wwm_u.ctx, mutex->wwm_u.ctx->wwx_owner,
	    curlwp);
	KASSERT(mutex->wwm_u.ctx->wwx_acquired != ~0U);
	mutex->wwm_u.ctx->wwx_acquired--;
	mutex->wwm_u.ctx = NULL;
}

/*
 * ww_mutex_unlock(mutex)
 *
 *	Release mutex and wake the next caller waiting, if any.
 */
void
ww_mutex_unlock(struct ww_mutex *mutex)
{
	struct ww_acquire_ctx *ctx;

	mutex_enter(&mutex->wwm_lock);
	WW_UNLOCKED(mutex);
	KASSERTMSG(mutex->wwm_state != WW_UNLOCKED, "mutex %p", mutex);
	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
		panic("unlocking unlocked wait/wound mutex: %p", mutex);
	case WW_OWNED:
		/* Let the context lockers fight over it.  */
		mutex->wwm_u.owner = NULL;
		mutex->wwm_state = WW_UNLOCKED;
		break;
	case WW_CTX:
		ww_mutex_unlock_release(mutex);
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
		ww_mutex_unlock_release(mutex);
		/* Let the non-context lockers fight over it.  */
		mutex->wwm_state = WW_UNLOCKED;
		break;
	}
	cv_broadcast(&mutex->wwm_cv);
	mutex_exit(&mutex->wwm_lock);
}

/*
 * ww_mutex_locking_ctx(mutex)
 *
 *	Return the current acquire context of mutex.  Answer is stale
 *	as soon as returned unless mutex is held by caller.
 */
struct ww_acquire_ctx *
ww_mutex_locking_ctx(struct ww_mutex *mutex)
{
	struct ww_acquire_ctx *ctx;

	mutex_enter(&mutex->wwm_lock);
	switch (mutex->wwm_state) {
	case WW_UNLOCKED:
	case WW_OWNED:
		ctx = NULL;
		break;
	case WW_CTX:
	case WW_WANTOWN:
		ctx = mutex->wwm_u.ctx;
		break;
	default:
		panic("wait/wound mutex %p in bad state: %d",
		    mutex, (int)mutex->wwm_state);
	}
	mutex_exit(&mutex->wwm_lock);

	return ctx;
}
