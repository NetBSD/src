/*	$NetBSD: nouveau_fence.c,v 1.8 2018/08/23 01:10:28 riastradh Exp $	*/

/*
 * Copyright (C) 2007 Ben Skeggs.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_fence.c,v 1.8 2018/08/23 01:10:28 riastradh Exp $");

#include <sys/types.h>
#include <sys/xcall.h>

#include <drm/drmP.h>

#include <asm/param.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>

#include "nouveau_drm.h"
#include "nouveau_dma.h"
#include "nouveau_fence.h"

#include <engine/fifo.h>

/*
 * struct fence_work
 *
 *	State for a work action scheduled when a fence is completed.
 *	Will call func(data) at some point after that happens.
 */
struct fence_work {
	struct work_struct base;
	struct list_head head;
	void (*func)(void *);
	void *data;
};

/*
 * nouveau_fence_channel_acquire(fence)
 *
 *	Try to return the channel associated with fence.
 */
static struct nouveau_channel *
nouveau_fence_channel_acquire(struct nouveau_fence *fence)
{
	struct nouveau_channel *chan;
	struct nouveau_fence_chan *fctx;

	/*
	 * Block cross-calls while we examine fence.  If we observe
	 * that fence->done is false, then the channel cannot be
	 * destroyed even by another CPU until after kpreempt_enable.
	 */
	kpreempt_disable();
	if (fence->done) {
		chan = NULL;
	} else {
		chan = fence->channel;
		fctx = chan->fence;
		atomic_inc_uint(&fctx->refcnt);
	}
	kpreempt_enable();

	return chan;
}

/*
 * nouveau_fence_gc_grab(fctx, list)
 *
 *	Move all of channel's done fences to list.
 *
 *	Caller must hold channel's fence lock.
 */
static void
nouveau_fence_gc_grab(struct nouveau_fence_chan *fctx, struct list_head *list)
{
	struct list_head *node, *next;

	BUG_ON(!spin_is_locked(&fctx->lock));

	list_for_each_safe(node, next, &fctx->done) {
		list_move_tail(node, list);
	}
}

/*
 * nouveau_fence_gc_free(list)
 *
 *	Unreference all of the fences in the list.
 *
 *	Caller MUST NOT hold the fences' channel's fence lock.
 */
static void
nouveau_fence_gc_free(struct list_head *list)
{
	struct nouveau_fence *fence, *next;

	list_for_each_entry_safe(fence, next, list, head) {
		list_del(&fence->head);
		nouveau_fence_unref(&fence);
	}
}

/*
 * nouveau_fence_channel_release(channel)
 *
 *	Release the channel acquired with nouveau_fence_channel_acquire.
 */
static void
nouveau_fence_channel_release(struct nouveau_channel *chan)
{
	struct nouveau_fence_chan *fctx = chan->fence;
	unsigned old, new;

	do {
		old = fctx->refcnt;
		if (old == 0) {
			spin_lock(&fctx->lock);
			if (atomic_dec_uint_nv(&fctx->refcnt) == 0)
				DRM_SPIN_WAKEUP_ALL(&fctx->waitqueue,
				    &fctx->lock);
			spin_unlock(&fctx->lock);
			return;
		}
		new = old - 1;
	} while (atomic_cas_uint(&fctx->refcnt, old, new) != old);
}

/*
 * nouveau_fence_signal(fence)
 *
 *	Schedule all the work for fence's completion, mark it done, and
 *	move it from the pending list to the done list.
 *
 *	Caller must hold fence's channel's fence lock.
 */
static void
nouveau_fence_signal(struct nouveau_fence *fence)
{
	struct nouveau_channel *chan __diagused = fence->channel;
	struct nouveau_fence_chan *fctx __diagused = chan->fence;
	struct fence_work *work, *temp;

	BUG_ON(!spin_is_locked(&fctx->lock));
	BUG_ON(fence->done);

	/* Schedule all the work for this fence.  */
	list_for_each_entry_safe(work, temp, &fence->work, head) {
		schedule_work(&work->base);
		list_del(&work->head);
	}

	/* Note that the fence is done.  */
	fence->done = true;

	/* Move it from the pending list to the done list.  */
	list_move_tail(&fence->head, &fctx->done);
}

static void
nouveau_fence_context_del_xc(void *a, void *b)
{
}

/*
 * nouveau_fence_context_del(fctx)
 *
 *	Artificially complete all fences in fctx, wait for their work
 *	to drain, and destroy the memory associated with fctx.
 */
void
nouveau_fence_context_del(struct nouveau_fence_chan *fctx)
{
	struct nouveau_fence *fence, *fnext;
	struct list_head done_list;
	int ret __diagused;

	INIT_LIST_HEAD(&done_list);

	/* Signal all the fences in fctx.  */
	spin_lock(&fctx->lock);
	list_for_each_entry_safe(fence, fnext, &fctx->pending, head) {
		nouveau_fence_signal(fence);
	}
	nouveau_fence_gc_grab(fctx, &done_list);
	spin_unlock(&fctx->lock);

	/* Release any fences that we signalled.  */
	nouveau_fence_gc_free(&done_list);

	/* Wait for the workqueue to drain.  */
	flush_scheduled_work();

	/* Wait for nouveau_fence_channel_acquire to complete on all CPUs.  */
	xc_wait(xc_broadcast(0, nouveau_fence_context_del_xc, NULL, NULL));

	/* Wait for any references to drain.  */
	spin_lock(&fctx->lock);
	DRM_SPIN_WAIT_NOINTR_UNTIL(ret, &fctx->waitqueue, &fctx->lock,
	    fctx->refcnt == 0);
	BUG_ON(ret);
	spin_unlock(&fctx->lock);

	/* Make sure there are no more fences on the list.  */
	BUG_ON(!list_empty(&fctx->done));
	BUG_ON(!list_empty(&fctx->flip));
	BUG_ON(!list_empty(&fctx->pending));

	/* Destroy the fence context.  */
	DRM_DESTROY_WAITQUEUE(&fctx->waitqueue);
	spin_lock_destroy(&fctx->lock);
}

/*
 * nouveau_fence_context_new(fctx)
 *
 *	Initialize the state fctx for all fences on a channel.
 */
void
nouveau_fence_context_new(struct nouveau_fence_chan *fctx)
{

	INIT_LIST_HEAD(&fctx->flip);
	INIT_LIST_HEAD(&fctx->pending);
	INIT_LIST_HEAD(&fctx->done);
	spin_lock_init(&fctx->lock);
	DRM_INIT_WAITQUEUE(&fctx->waitqueue, "nvfnchan");
	fctx->refcnt = 0;
}

/*
 * nouveau_fence_work_handler(kwork)
 *
 *	Work handler for nouveau_fence_work.
 */
static void
nouveau_fence_work_handler(struct work_struct *kwork)
{
	struct fence_work *work = container_of(kwork, typeof(*work), base);

	work->func(work->data);
	kfree(work);
}

/*
 * nouveau_fence_work(fence, func, data)
 *
 *	Arrange to call func(data) after fence is completed.  If fence
 *	is already completed, call it immediately.  If memory is
 *	scarce, synchronously wait for the fence and call it.
 */
void
nouveau_fence_work(struct nouveau_fence *fence,
		   void (*func)(void *), void *data)
{
	struct nouveau_channel *chan;
	struct nouveau_fence_chan *fctx;
	struct fence_work *work = NULL;

	if ((chan = nouveau_fence_channel_acquire(fence)) == NULL)
		goto now0;
	fctx = chan->fence;

	work = kmalloc(sizeof(*work), GFP_KERNEL);
	if (work == NULL) {
		WARN_ON(nouveau_fence_wait(fence, false, false));
		goto now1;
	}

	spin_lock(&fctx->lock);
	if (fence->done) {
		spin_unlock(&fctx->lock);
		goto now2;
	}
	INIT_WORK(&work->base, nouveau_fence_work_handler);
	work->func = func;
	work->data = data;
	list_add(&work->head, &fence->work);
	if (atomic_dec_uint_nv(&fctx->refcnt) == 0)
		DRM_SPIN_WAKEUP_ALL(&fctx->waitqueue, &fctx->lock);
	spin_unlock(&fctx->lock);
	return;

now2:	kfree(work);
now1:	nouveau_fence_channel_release(chan);
now0:	func(data);
}

/*
 * nouveau_fence_update(chan)
 *
 *	Test all fences on chan for completion.  For any that are
 *	completed, mark them as such and schedule work for them.
 *
 *	Caller must hold chan's fence lock.
 */
static void
nouveau_fence_update(struct nouveau_channel *chan)
{
	struct nouveau_fence_chan *fctx = chan->fence;
	struct nouveau_fence *fence, *fnext;

	BUG_ON(!spin_is_locked(&fctx->lock));
	list_for_each_entry_safe(fence, fnext, &fctx->pending, head) {
		if (fctx->read(chan) < fence->sequence)
			break;
		nouveau_fence_signal(fence);
	}
	BUG_ON(!spin_is_locked(&fctx->lock));
}

/*
 * nouveau_fence_emit(fence, chan)
 *
 *	- Initialize fence.
 *	- Set its timeout to 15 sec from now.
 *	- Assign it the next sequence number on channel.
 *	- Submit it to the device with the device-specific emit routine.
 *	- If that succeeds, add it to the list of pending fences on chan.
 */
int
nouveau_fence_emit(struct nouveau_fence *fence, struct nouveau_channel *chan)
{
	struct nouveau_fence_chan *fctx = chan->fence;
	int ret;

	fence->channel  = chan;
	fence->timeout  = jiffies + (15 * HZ);
	spin_lock(&fctx->lock);
	fence->sequence = ++fctx->sequence;
	spin_unlock(&fctx->lock);

	ret = fctx->emit(fence);
	if (!ret) {
		kref_get(&fence->kref);
		spin_lock(&fctx->lock);
		list_add_tail(&fence->head, &fctx->pending);
		spin_unlock(&fctx->lock);
	}

	return ret;
}

/*
 * nouveau_fence_done_locked(fence, chan)
 *
 *	Test whether fence, which must be on chan, is done.  If it is
 *	not marked as done, poll all fences on chan first.
 *
 *	Caller must hold chan's fence lock.
 */
static bool
nouveau_fence_done_locked(struct nouveau_fence *fence,
    struct nouveau_channel *chan)
{
	struct nouveau_fence_chan *fctx __diagused = chan->fence;

	BUG_ON(!spin_is_locked(&fctx->lock));
	BUG_ON(fence->channel != chan);

	/* If it's not done, poll it for changes.  */
	if (!fence->done)
		nouveau_fence_update(chan);

	/* Check, possibly again, whether it is done now.  */
	return fence->done;
}

/*
 * nouveau_fence_done(fence)
 *
 *	Test whether fence is done.  If it is not marked as done, poll
 *	all fences on its channel first.  Caller MUST NOT hold the
 *	fence lock.
 */
bool
nouveau_fence_done(struct nouveau_fence *fence)
{
	struct nouveau_channel *chan;
	struct nouveau_fence_chan *fctx;
	struct list_head done_list;
	bool done;

	if ((chan = nouveau_fence_channel_acquire(fence)) == NULL)
		return true;

	INIT_LIST_HEAD(&done_list);

	fctx = chan->fence;
	spin_lock(&fctx->lock);
	done = nouveau_fence_done_locked(fence, chan);
	nouveau_fence_gc_grab(fctx, &done_list);
	spin_unlock(&fctx->lock);

	nouveau_fence_channel_release(chan);

	nouveau_fence_gc_free(&done_list);

	return done;
}

/*
 * nouveau_fence_wait_uevent_handler(data, index)
 *
 *	Nouveau uevent handler for fence completion.  data is a
 *	nouveau_fence_chan pointer.  Simply wake up all threads waiting
 *	for completion of any fences on the channel.  Does not mark
 *	fences as completed -- threads must poll fences for completion.
 */
static int
nouveau_fence_wait_uevent_handler(void *data, int index)
{
	struct nouveau_fence_chan *fctx = data;

	spin_lock(&fctx->lock);
	DRM_SPIN_WAKEUP_ALL(&fctx->waitqueue, &fctx->lock);
	spin_unlock(&fctx->lock);

	return NVKM_EVENT_KEEP;
}

/*
 * nouveau_fence_wait_uevent(fence, chan, intr)
 *
 *	Wait using a nouveau event for completion of fence on chan.
 *	Wait interruptibly iff intr is true.
 *
 *	Return 0 if fence was signalled, negative error code on
 *	timeout (-EBUSY) or interrupt (-ERESTARTSYS) or other error.
 */
static int
nouveau_fence_wait_uevent(struct nouveau_fence *fence,
    struct nouveau_channel *chan, bool intr)
{
	struct nouveau_fifo *pfifo = nouveau_fifo(chan->drm->device);
	struct nouveau_fence_chan *fctx = chan->fence;
	struct nouveau_eventh *handler;
	struct list_head done_list;
	int ret = 0;

	BUG_ON(fence->channel != chan);

	ret = nouveau_event_new(pfifo->uevent, 0,
				nouveau_fence_wait_uevent_handler,
				fctx, &handler);
	if (ret)
		return ret;

	nouveau_event_get(handler);

	INIT_LIST_HEAD(&done_list);

	if (fence->timeout) {
		unsigned long timeout = fence->timeout - jiffies;

		if (time_before(jiffies, fence->timeout)) {
			spin_lock(&fctx->lock);
			if (intr) {
				DRM_SPIN_TIMED_WAIT_UNTIL(ret,
				    &fctx->waitqueue, &fctx->lock,
				    timeout,
				    nouveau_fence_done_locked(fence, chan));
			} else {
				DRM_SPIN_TIMED_WAIT_NOINTR_UNTIL(ret,
				    &fctx->waitqueue, &fctx->lock,
				    timeout,
				    nouveau_fence_done_locked(fence, chan));
			}
			nouveau_fence_gc_grab(fctx, &done_list);
			spin_unlock(&fctx->lock);
			if (ret < 0) {
				/* error */
			} else if (ret == 0) {
				/* timeout */
				ret = -EBUSY;
			} else {
				/* success */
				ret = 0;
			}
		} else {
			/* timeout */
			ret = -EBUSY;
		}
	} else {
		spin_lock(&fctx->lock);
		if (intr) {
			DRM_SPIN_WAIT_UNTIL(ret, &fctx->waitqueue,
			    &fctx->lock,
			    nouveau_fence_done_locked(fence, chan));
		} else {
			DRM_SPIN_WAIT_NOINTR_UNTIL(ret, &fctx->waitqueue,
			    &fctx->lock,
			    nouveau_fence_done_locked(fence, chan));
		}
		nouveau_fence_gc_grab(fctx, &done_list);
		spin_unlock(&fctx->lock);
	}

	nouveau_event_ref(NULL, &handler);

	nouveau_fence_gc_free(&done_list);

	if (unlikely(ret < 0))
		return ret;

	return 0;
}

/*
 * nouveau_fence_wait(fence, lazy, intr)
 *
 *	Wait for fence to complete.  Wait interruptibly iff intr is
 *	true.  If lazy is true, may sleep, either for a single tick or
 *	for an interrupt; otherwise will busy-wait.
 *
 *	Return 0 if fence was signalled, negative error code on
 *	timeout (-EBUSY) or interrupt (-ERESTARTSYS) or other error.
 */
int
nouveau_fence_wait(struct nouveau_fence *fence, bool lazy, bool intr)
{
	struct nouveau_channel *chan;
	struct nouveau_fence_priv *priv;
	unsigned long delay_usec = 1;
	int ret = 0;

	if ((chan = nouveau_fence_channel_acquire(fence)) == NULL)
		goto out0;

	priv = chan->drm->fence;
	while (priv && priv->uevent && lazy && !nouveau_fence_done(fence)) {
		ret = nouveau_fence_wait_uevent(fence, chan, intr);
		if (ret < 0)
			goto out1;
	}

	while (!nouveau_fence_done(fence)) {
		if (fence->timeout && time_after_eq(jiffies, fence->timeout)) {
			ret = -EBUSY;
			break;
		}

		if (lazy && delay_usec >= 1000*hztoms(1)) {
			/* XXX errno NetBSD->Linux */
			ret = -kpause("nvfencew", intr, 1, NULL);
			if (ret != -EWOULDBLOCK)
				break;
		} else {
			DELAY(delay_usec);
			delay_usec *= 2;
		}
	}

out1:	nouveau_fence_channel_release(chan);
out0:	return ret;
}

int
nouveau_fence_sync(struct nouveau_fence *fence, struct nouveau_channel *chan)
{
	struct nouveau_fence_chan *fctx = chan->fence;
	struct nouveau_channel *prev;
	int ret = 0;

	if (fence != NULL &&
	    (prev = nouveau_fence_channel_acquire(fence)) != NULL) {
		if (unlikely(prev != chan && !nouveau_fence_done(fence))) {
			ret = fctx->sync(fence, prev, chan);
			if (unlikely(ret))
				ret = nouveau_fence_wait(fence, true, false);
		}
		nouveau_fence_channel_release(prev);
	}

	return ret;
}

static void
nouveau_fence_del(struct kref *kref)
{
	struct nouveau_fence *fence = container_of(kref, typeof(*fence), kref);

	kfree(fence);
}

void
nouveau_fence_unref(struct nouveau_fence **pfence)
{

	if (*pfence)
		kref_put(&(*pfence)->kref, nouveau_fence_del);
	*pfence = NULL;
}

struct nouveau_fence *
nouveau_fence_ref(struct nouveau_fence *fence)
{

	if (fence)
		kref_get(&fence->kref);
	return fence;
}

int
nouveau_fence_new(struct nouveau_channel *chan, bool sysmem,
		  struct nouveau_fence **pfence)
{
	struct nouveau_fence *fence;
	int ret = 0;

	if (unlikely(!chan->fence))
		return -ENODEV;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return -ENOMEM;

	INIT_LIST_HEAD(&fence->work);
	fence->sysmem = sysmem;
	fence->done = false;
	kref_init(&fence->kref);

	ret = nouveau_fence_emit(fence, chan);
	if (ret)
		nouveau_fence_unref(&fence);

	*pfence = fence;
	return ret;
}
