/*	$NetBSD: linux_fence.c,v 1.3 2018/08/27 13:56:34 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_fence.c,v 1.3 2018/08/27 13:56:34 riastradh Exp $");

#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/queue.h>

#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/kref.h>
#include <linux/fence.h>
#include <linux/sched.h>
#include <linux/spinlock.h>

/*
 * fence_init(fence, ops, lock, context, seqno)
 *
 *	Initialize fence.  Caller should call fence_destroy when done,
 *	after all references have been released.
 */
void
fence_init(struct fence *fence, const struct fence_ops *ops, spinlock_t *lock,
    unsigned context, unsigned seqno)
{

	kref_init(&fence->refcount);
	fence->lock = lock;
	fence->flags = 0;
	fence->context = context;
	fence->seqno = seqno;
	fence->ops = ops;
	TAILQ_INIT(&fence->f_callbacks);
	cv_init(&fence->f_cv, "fence");
}

/*
 * fence_destroy(fence)
 *
 *	Clean up memory initialized with fence_init.  This is meant to
 *	be used after a fence release callback.
 */
void
fence_destroy(struct fence *fence)
{

	KASSERT(TAILQ_EMPTY(&fence->f_callbacks));
	cv_destroy(&fence->f_cv);
}

static void
fence_free_cb(struct rcu_head *rcu)
{
	struct fence *fence = container_of(rcu, struct fence, f_rcu);

	fence_destroy(fence);
	kfree(fence);
}

/*
 * fence_free(fence)
 *
 *	Schedule fence to be destroyed and then freed with kfree after
 *	any pending RCU read sections on all CPUs have completed.
 *	Caller must guarantee all references have been released.  This
 *	is meant to be used after a fence release callback.
 *
 *	NOTE: Callers assume kfree will be used.  We don't even use
 *	kmalloc to allocate these -- caller is expected to allocate
 *	memory with kmalloc to be initialized with fence_init.
 */
void
fence_free(struct fence *fence)
{

	call_rcu(&fence->f_rcu, &fence_free_cb);
}

/*
 * fence_context_alloc(n)
 *
 *	Return the first of a contiguous sequence of unique
 *	identifiers, at least until the system wraps around.
 */
unsigned
fence_context_alloc(unsigned n)
{
	static volatile unsigned next_context = 0;

	return atomic_add_int_nv(&next_context, n) - n;
}

/*
 * fence_get(fence)
 *
 *	Acquire a reference to fence.  The fence must not be being
 *	destroyed.  Return the fence.
 */
struct fence *
fence_get(struct fence *fence)
{

	kref_get(&fence->refcount);
	return fence;
}

/*
 * fence_get_rcu(fence)
 *
 *	Attempt to acquire a reference to a fence that may be about to
 *	be destroyed, during a read section.  Return the fence on
 *	success, or NULL on failure.
 */
struct fence *
fence_get_rcu(struct fence *fence)
{

	if (!kref_get_unless_zero(&fence->refcount))
		return NULL;
	return fence;
}

static void
fence_release(struct kref *refcount)
{
	struct fence *fence = container_of(refcount, struct fence, refcount);

	if (fence->ops->release)
		(*fence->ops->release)(fence);
	else
		fence_free(fence);
}

/*
 * fence_put(fence)
 *
 *	Release a reference to fence.  If this was the last one, call
 *	the fence's release callback.
 */
void
fence_put(struct fence *fence)
{

	kref_put(&fence->refcount, &fence_release);
}

/*
 * fence_ensure_signal_enabled(fence)
 *
 *	Internal subroutine.  If the fence was already signalled,
 *	return -ENOENT.  Otherwise, if the enable signalling callback
 *	has not been called yet, call it.  If fails, signal the fence
 *	and return -ENOENT.  If it succeeds, or if it had already been
 *	called, return zero to indicate success.
 *
 *	Caller must hold the fence's lock.
 */
static int
fence_ensure_signal_enabled(struct fence *fence)
{

	KASSERT(spin_is_locked(fence->lock));

	/* If the fence was already signalled, fail with -ENOENT.  */
	if (fence->flags & (1u << FENCE_FLAG_SIGNALED_BIT))
		return -ENOENT;

	/*
	 * If the enable signaling callback has been called, success.
	 * Otherwise, set the bit indicating it.
	 */
	if (test_and_set_bit(FENCE_FLAG_ENABLE_SIGNAL_BIT, &fence->flags))
		return 0;

	/* Otherwise, note that we've called it and call it.  */
	if (!(*fence->ops->enable_signaling)(fence)) {
		/* If it failed, signal and return -ENOENT.  */
		fence_signal_locked(fence);
		return -ENOENT;
	}

	/* Success!  */
	return 0;
}

/*
 * fence_add_callback(fence, fcb, fn)
 *
 *	If fence has been signalled, return -ENOENT.  If the enable
 *	signalling callback hasn't been called yet, call it; if it
 *	fails, return -ENOENT.  Otherwise, arrange to call fn(fence,
 *	fcb) when it is signalled, and return 0.
 *
 *	The fence uses memory allocated by the caller in fcb from the
 *	time of fence_add_callback either to the time of
 *	fence_remove_callback, or just before calling fn.
 */
int
fence_add_callback(struct fence *fence, struct fence_cb *fcb, fence_func_t fn)
{
	int ret;

	/* Optimistically try to skip the lock if it's already signalled.  */
	if (fence->flags & (1u << FENCE_FLAG_SIGNALED_BIT)) {
		ret = -ENOENT;
		goto out0;
	}

	/* Acquire the lock.  */
	spin_lock(fence->lock);

	/* Ensure signalling is enabled, or fail if we can't.  */
	ret = fence_ensure_signal_enabled(fence);
	if (ret)
		goto out1;

	/* Insert the callback.  */
	fcb->fcb_func = fn;
	TAILQ_INSERT_TAIL(&fence->f_callbacks, fcb, fcb_entry);
	fcb->fcb_onqueue = true;

	/* Release the lock and we're done.  */
out1:	spin_unlock(fence->lock);
out0:	return ret;
}

/*
 * fence_remove_callback(fence, fcb)
 *
 *	Remove the callback fcb from fence.  Return true if it was
 *	removed from the list, or false if it had already run and so
 *	was no longer queued anyway.  Caller must have already called
 *	fence_add_callback(fence, fcb).
 */
bool
fence_remove_callback(struct fence *fence, struct fence_cb *fcb)
{
	bool onqueue;

	spin_lock(fence->lock);
	onqueue = fcb->fcb_onqueue;
	if (onqueue) {
		TAILQ_REMOVE(&fence->f_callbacks, fcb, fcb_entry);
		fcb->fcb_onqueue = false;
	}
	spin_unlock(fence->lock);

	return onqueue;
}

/*
 * fence_enable_sw_signaling(fence)
 *
 *	If it hasn't been called yet and the fence hasn't been
 *	signalled yet, call the fence's enable_sw_signaling callback.
 *	If when that happens, the callback indicates failure by
 *	returning false, signal the fence.
 */
void
fence_enable_sw_signaling(struct fence *fence)
{

	spin_lock(fence->lock);
	(void)fence_ensure_signal_enabled(fence);
	spin_unlock(fence->lock);
}

/*
 * fence_is_signaled(fence)
 *
 *	Test whether the fence has been signalled.  If it has been
 *	signalled by fence_signal(_locked), return true.  If the
 *	signalled callback returns true indicating that some implicit
 *	external condition has changed, call the callbacks as if with
 *	fence_signal.
 */
bool
fence_is_signaled(struct fence *fence)
{
	bool signaled;

	spin_lock(fence->lock);
	signaled = fence_is_signaled_locked(fence);
	spin_unlock(fence->lock);

	return signaled;
}

/*
 * fence_is_signaled_locked(fence)
 *
 *	Test whether the fence has been signalled.  Like
 *	fence_is_signaleed, but caller already holds the fence's lock.
 */
bool
fence_is_signaled_locked(struct fence *fence)
{

	KASSERT(spin_is_locked(fence->lock));

	/* Check whether we already set the signalled bit.  */
	if (fence->flags & (1u << FENCE_FLAG_SIGNALED_BIT))
		return true;

	/* If there's a signalled callback, test it.  */
	if (fence->ops->signaled) {
		if ((*fence->ops->signaled)(fence)) {
			/*
			 * It's been signalled implicitly by some
			 * external phenomonen.  Act as though someone
			 * has called fence_signal.
			 */
			fence_signal_locked(fence);
			return true;
		}
	}

	return false;
}

/*
 * fence_signal(fence)
 *
 *	Signal the fence.  If it has already been signalled, return
 *	-EINVAL.  If it has not been signalled, call the enable
 *	signalling callback if it hasn't been called yet, and remove
 *	each registered callback from the queue and call it; then
 *	return 0.
 */
int
fence_signal(struct fence *fence)
{
	int ret;

	spin_lock(fence->lock);
	ret = fence_signal_locked(fence);
	spin_unlock(fence->lock);

	return ret;
}

/*
 * fence_signal_locked(fence)
 *
 *	Signal the fence.  Like fence_signal, but caller already holds
 *	the fence's lock.
 */
int
fence_signal_locked(struct fence *fence)
{
	struct fence_cb *fcb, *next;

	KASSERT(spin_is_locked(fence->lock));

	/* If it's been signalled, fail; otherwise set the signalled bit.  */
	if (test_and_set_bit(FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		return -EINVAL;

	/* Wake waiters.  */
	cv_broadcast(&fence->f_cv);

	/* Remove and call the callbacks.  */
	TAILQ_FOREACH_SAFE(fcb, &fence->f_callbacks, fcb_entry, next) {
		TAILQ_REMOVE(&fence->f_callbacks, fcb, fcb_entry);
		fcb->fcb_onqueue = false;
		(*fcb->fcb_func)(fence, fcb);
	}

	/* Success! */
	return 0;
}

/*
 * fence_wait_timeout(fence, intr, timeout)
 *
 *	Wait until fence is signalled; or until interrupt, if intr is
 *	true; or until timeout, if positive.  Return -ERESTARTSYS if
 *	interrupted, negative error code on any other error, zero on
 *	timeout, or positive number of ticks remaining if the fence is
 *	signalled before the timeout.  Works by calling the fence wait
 *	callback.
 *
 *	The timeout must be nonnegative and less than
 *	MAX_SCHEDULE_TIMEOUT.
 */
long
fence_wait_timeout(struct fence *fence, bool intr, long timeout)
{

	KASSERT(timeout >= 0);
	KASSERT(timeout < MAX_SCHEDULE_TIMEOUT);
	return (*fence->ops->wait)(fence, intr, timeout);
}

/*
 * fence_wait_timeout(fence, intr, timeout)
 *
 *	Wait until fence is signalled; or until interrupt, if intr is
 *	true.  Return -ERESTARTSYS if interrupted, negative error code
 *	on any other error, zero on sucess.  Works by calling the fence
 *	wait callback with MAX_SCHEDULE_TIMEOUT.
 */
long
fence_wait(struct fence *fence, bool intr)
{
	long ret;

	ret = (*fence->ops->wait)(fence, intr, MAX_SCHEDULE_TIMEOUT);
	KASSERT(ret != 0);

	return (ret < 0 ? ret : 0);
}

/*
 * fence_default_wait(fence, intr, timeout)
 *
 *	Default implementation of fence wait callback using a condition
 *	variable.  If the fence is already signalled, return timeout,
 *	or 0 if no timeout.  If the enable signalling callback hasn't
 *	been called, call it, and if it fails, act as if the fence had
 *	been signalled.  Otherwise, wait on the internal condvar.  If
 *	timeout is MAX_SCHEDULE_TIMEOUT, treat it as no timeout.
 */
long
fence_default_wait(struct fence *fence, bool intr, long timeout)
{
	int starttime = 0, now = 0, deadline = 0; /* XXXGCC */
	kmutex_t *lock = &fence->lock->sl_lock;
	long ret = 0;

	KASSERT(timeout >= 0);
	KASSERT(timeout <= MAX_SCHEDULE_TIMEOUT);

	/* Optimistically try to skip the lock if it's already signalled.  */
	if (fence->flags & (1u << FENCE_FLAG_SIGNALED_BIT))
		return (timeout < MAX_SCHEDULE_TIMEOUT ? timeout : 0);

	/* Acquire the lock.  */
	spin_lock(fence->lock);

	/* Ensure signalling is enabled, or fail if we can't.  */
	ret = fence_ensure_signal_enabled(fence);
	if (ret)
		goto out;

	/* Find out what our deadline is so we can handle spurious wakeup.  */
	if (timeout < MAX_SCHEDULE_TIMEOUT) {
		now = hardclock_ticks;
		starttime = now;
		deadline = starttime + timeout;
	}

	/* Wait until the signalled bit is set.  */
	while (!(fence->flags & (1u << FENCE_FLAG_SIGNALED_BIT))) {
		/*
		 * If there's a timeout and we've passed the deadline,
		 * give up.
		 */
		if (timeout < MAX_SCHEDULE_TIMEOUT) {
			now = hardclock_ticks;
			if (deadline <= now)
				break;
		}
		if (intr) {
			if (timeout < MAX_SCHEDULE_TIMEOUT) {
				ret = -cv_timedwait_sig(&fence->f_cv, lock,
				    deadline - now);
			} else {
				ret = -cv_wait_sig(&fence->f_cv, lock);
			}
		} else {
			if (timeout < MAX_SCHEDULE_TIMEOUT) {
				ret = -cv_timedwait(&fence->f_cv, lock,
				    deadline - now);
			} else {
				cv_wait(&fence->f_cv, lock);
				ret = 0;
			}
		}
		/* If the wait failed, give up.  */
		if (ret)
			break;
	}

out:
	/* All done.  Release the lock.  */
	spin_unlock(fence->lock);

	/* If cv_timedwait gave up, return 0 meaning timeout.  */
	if (ret == -EWOULDBLOCK)
		return 0;

	/* If there was a timeout and the deadline passed, return 0.  */
	if (timeout < MAX_SCHEDULE_TIMEOUT) {
		if (deadline <= now)
			return 0;
	}

	/* If we were interrupted, return -ERESTARTSYS.  */
	if (ret == -EINTR || ret == -ERESTART)
		return -ERESTARTSYS;

	/* If there was any other kind of error, fail.  */
	if (ret)
		return ret;

	/*
	 * Success!  Return the number of ticks left, at least 1, or 0
	 * if no timeout.
	 */
	return (timeout < MAX_SCHEDULE_TIMEOUT ? MIN(deadline - now, 1) : 0);
}
