/*	$NetBSD: linux_work.c,v 1.36 2018/08/27 15:05:16 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_work.c,v 1.36 2018/08/27 15:05:16 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/queue.h>

#include <linux/workqueue.h>

struct workqueue_struct {
	kmutex_t			wq_lock;
	kcondvar_t			wq_cv;
	TAILQ_HEAD(, delayed_work)	wq_delayed;
	TAILQ_HEAD(, work_struct)	wq_queue;
	struct work_struct		*wq_current_work;
	int				wq_flags;
	struct lwp			*wq_lwp;
	uint64_t			wq_gen;
	bool				wq_requeued:1;
	bool				wq_dying:1;
};

static void __dead	linux_workqueue_thread(void *);
static void		linux_workqueue_timeout(void *);
static struct workqueue_struct *
			acquire_work(struct work_struct *,
			    struct workqueue_struct *);
static void		release_work(struct work_struct *,
			    struct workqueue_struct *);
static void		wait_for_current_work(struct work_struct *,
			    struct workqueue_struct *);
static void		dw_callout_init(struct workqueue_struct *,
			    struct delayed_work *);
static void		dw_callout_destroy(struct workqueue_struct *,
			    struct delayed_work *);
static void		cancel_delayed_work_done(struct workqueue_struct *,
			    struct delayed_work *);

static specificdata_key_t workqueue_key __read_mostly;

struct workqueue_struct	*system_wq __read_mostly;
struct workqueue_struct	*system_long_wq __read_mostly;
struct workqueue_struct	*system_power_efficient_wq __read_mostly;

/*
 * linux_workqueue_init()
 *
 *	Initialize the Linux workqueue subsystem.  Return 0 on success,
 *	NetBSD error on failure.
 */
int
linux_workqueue_init(void)
{
	int error;

	error = lwp_specific_key_create(&workqueue_key, NULL);
	if (error)
		goto fail0;

	system_wq = alloc_ordered_workqueue("lnxsyswq", 0);
	if (system_wq == NULL) {
		error = ENOMEM;
		goto fail1;
	}

	system_long_wq = alloc_ordered_workqueue("lnxlngwq", 0);
	if (system_long_wq == NULL) {
		error = ENOMEM;
		goto fail2;
	}

	system_power_efficient_wq = alloc_ordered_workqueue("lnxpwrwq", 0);
	if (system_long_wq == NULL) {
		error = ENOMEM;
		goto fail3;
	}

	return 0;

fail4: __unused
	destroy_workqueue(system_power_efficient_wq);
fail3:	destroy_workqueue(system_long_wq);
fail2:	destroy_workqueue(system_wq);
fail1:	lwp_specific_key_delete(workqueue_key);
fail0:	KASSERT(error);
	return error;
}

/*
 * linux_workqueue_fini()
 *
 *	Destroy the Linux workqueue subsystem.  Never fails.
 */
void
linux_workqueue_fini(void)
{

	destroy_workqueue(system_power_efficient_wq);
	destroy_workqueue(system_long_wq);
	destroy_workqueue(system_wq);
	lwp_specific_key_delete(workqueue_key);
}

/*
 * Workqueues
 */

/*
 * alloc_ordered_workqueue(name, flags)
 *
 *	Create a workqueue of the given name.  No flags are currently
 *	defined.  Return NULL on failure, pointer to struct
 *	workqueue_struct object on success.
 */
struct workqueue_struct *
alloc_ordered_workqueue(const char *name, int flags)
{
	struct workqueue_struct *wq;
	int error;

	KASSERT(flags == 0);

	wq = kmem_zalloc(sizeof(*wq), KM_SLEEP);

	mutex_init(&wq->wq_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&wq->wq_cv, name);
	TAILQ_INIT(&wq->wq_delayed);
	TAILQ_INIT(&wq->wq_queue);
	wq->wq_current_work = NULL;
	wq->wq_flags = 0;
	wq->wq_lwp = NULL;
	wq->wq_gen = 0;
	wq->wq_requeued = false;
	wq->wq_dying = false;

	error = kthread_create(PRI_NONE,
	    KTHREAD_MPSAFE|KTHREAD_TS|KTHREAD_MUSTJOIN, NULL,
	    &linux_workqueue_thread, wq, &wq->wq_lwp, "%s", name);
	if (error)
		goto fail0;

	return wq;

fail0:	KASSERT(TAILQ_EMPTY(&wq->wq_queue));
	KASSERT(TAILQ_EMPTY(&wq->wq_delayed));
	cv_destroy(&wq->wq_cv);
	mutex_destroy(&wq->wq_lock);
	kmem_free(wq, sizeof(*wq));
	return NULL;
}

/*
 * destroy_workqueue(wq)
 *
 *	Destroy a workqueue created with wq.  Cancel any pending
 *	delayed work.  Wait for all queued work to complete.
 *
 *	May sleep.
 */
void
destroy_workqueue(struct workqueue_struct *wq)
{

	/*
	 * Cancel all delayed work.  We do this first because any
	 * delayed work that that has already timed out, which we can't
	 * cancel, may have queued new work.
	 */
	mutex_enter(&wq->wq_lock);
	while (!TAILQ_EMPTY(&wq->wq_delayed)) {
		struct delayed_work *const dw = TAILQ_FIRST(&wq->wq_delayed);

		KASSERT(dw->work.work_queue == wq);
		KASSERTMSG((dw->dw_state == DELAYED_WORK_SCHEDULED ||
			dw->dw_state == DELAYED_WORK_RESCHEDULED ||
			dw->dw_state == DELAYED_WORK_CANCELLED),
		    "delayed work %p in bad state: %d",
		    dw, dw->dw_state);

		/*
		 * Mark it cancelled and try to stop the callout before
		 * it starts.
		 *
		 * If it's too late and the callout has already begun
		 * to execute, then it will notice that we asked to
		 * cancel it and remove itself from the queue before
		 * returning.
		 *
		 * If we stopped the callout before it started,
		 * however, then we can safely destroy the callout and
		 * dissociate it from the workqueue ourselves.
		 */
		dw->dw_state = DELAYED_WORK_CANCELLED;
		if (!callout_halt(&dw->dw_callout, &wq->wq_lock))
			cancel_delayed_work_done(wq, dw);
	}
	mutex_exit(&wq->wq_lock);

	/*
	 * At this point, no new work can be put on the queue.
	 */

	/* Tell the thread to exit.  */
	mutex_enter(&wq->wq_lock);
	wq->wq_dying = true;
	cv_broadcast(&wq->wq_cv);
	mutex_exit(&wq->wq_lock);

	/* Wait for it to exit.  */
	(void)kthread_join(wq->wq_lwp);

	KASSERT(wq->wq_dying);
	KASSERT(!wq->wq_requeued);
	KASSERT(wq->wq_flags == 0);
	KASSERT(wq->wq_current_work == NULL);
	KASSERT(TAILQ_EMPTY(&wq->wq_queue));
	KASSERT(TAILQ_EMPTY(&wq->wq_delayed));
	cv_destroy(&wq->wq_cv);
	mutex_destroy(&wq->wq_lock);

	kmem_free(wq, sizeof(*wq));
}

/*
 * Work thread and callout
 */

/*
 * linux_workqueue_thread(cookie)
 *
 *	Main function for a workqueue's worker thread.  Waits until
 *	there is work queued, grabs a batch of work off the queue,
 *	executes it all, bumps the generation number, and repeats,
 *	until dying.
 */
static void __dead
linux_workqueue_thread(void *cookie)
{
	struct workqueue_struct *const wq = cookie;
	TAILQ_HEAD(, work_struct) tmp;

	lwp_setspecific(workqueue_key, wq);

	mutex_enter(&wq->wq_lock);
	for (;;) {
		/*
		 * Wait until there's activity.  If there's no work and
		 * we're dying, stop here.
		 */
		while (TAILQ_EMPTY(&wq->wq_queue) && !wq->wq_dying)
			cv_wait(&wq->wq_cv, &wq->wq_lock);
		if (TAILQ_EMPTY(&wq->wq_queue)) {
			KASSERT(wq->wq_dying);
			break;
		}

		/* Grab a batch of work off the queue.  */
		KASSERT(!TAILQ_EMPTY(&wq->wq_queue));
		TAILQ_INIT(&tmp);
		TAILQ_CONCAT(&tmp, &wq->wq_queue, work_entry);

		/* Process each work item in the batch.  */
		while (!TAILQ_EMPTY(&tmp)) {
			struct work_struct *const work = TAILQ_FIRST(&tmp);

			KASSERT(work->work_queue == wq);
			TAILQ_REMOVE(&tmp, work, work_entry);
			KASSERT(wq->wq_current_work == NULL);
			wq->wq_current_work = work;

			mutex_exit(&wq->wq_lock);
			(*work->func)(work);
			mutex_enter(&wq->wq_lock);

			KASSERT(wq->wq_current_work == work);
			KASSERT(work->work_queue == wq);
			if (wq->wq_requeued)
				wq->wq_requeued = false;
			else
				release_work(work, wq);
			wq->wq_current_work = NULL;
			cv_broadcast(&wq->wq_cv);
		}

		/* Notify flush that we've completed a batch of work.  */
		wq->wq_gen++;
		cv_broadcast(&wq->wq_cv);
	}
	mutex_exit(&wq->wq_lock);

	kthread_exit(0);
}

/*
 * linux_workqueue_timeout(cookie)
 *
 *	Delayed work timeout callback.
 *
 *	- If scheduled, queue it.
 *	- If rescheduled, callout_schedule ourselves again.
 *	- If cancelled, destroy the callout and release the work from
 *        the workqueue.
 */
static void
linux_workqueue_timeout(void *cookie)
{
	struct delayed_work *const dw = cookie;
	struct workqueue_struct *const wq = dw->work.work_queue;

	KASSERT(wq != NULL);

	mutex_enter(&wq->wq_lock);
	KASSERT(dw->work.work_queue == wq);
	switch (dw->dw_state) {
	case DELAYED_WORK_IDLE:
		panic("delayed work callout uninitialized: %p", dw);
	case DELAYED_WORK_SCHEDULED:
		dw_callout_destroy(wq, dw);
		TAILQ_INSERT_TAIL(&wq->wq_queue, &dw->work, work_entry);
		cv_broadcast(&wq->wq_cv);
		break;
	case DELAYED_WORK_RESCHEDULED:
		KASSERT(dw->dw_resched >= 0);
		callout_schedule(&dw->dw_callout, dw->dw_resched);
		dw->dw_state = DELAYED_WORK_SCHEDULED;
		dw->dw_resched = -1;
		break;
	case DELAYED_WORK_CANCELLED:
		cancel_delayed_work_done(wq, dw);
		/* Can't touch dw any more.  */
		goto out;
	default:
		panic("delayed work callout in bad state: %p", dw);
	}
	KASSERT(dw->dw_state == DELAYED_WORK_IDLE ||
	    dw->dw_state == DELAYED_WORK_SCHEDULED);
out:	mutex_exit(&wq->wq_lock);
}

/*
 * current_work()
 *
 *	If in a workqueue worker thread, return the work it is
 *	currently executing.  Otherwise return NULL.
 */
struct work_struct *
current_work(void)
{
	struct workqueue_struct *wq = lwp_getspecific(workqueue_key);

	/* If we're not a workqueue thread, then there's no work.  */
	if (wq == NULL)
		return NULL;

	/*
	 * Otherwise, this should be possible only while work is in
	 * progress.  Return the current work item.
	 */
	KASSERT(wq->wq_current_work != NULL);
	return wq->wq_current_work;
}

/*
 * Work
 */

/*
 * INIT_WORK(work, fn)
 *
 *	Initialize work for use with a workqueue to call fn in a worker
 *	thread.  There is no corresponding destruction operation.
 */
void
INIT_WORK(struct work_struct *work, void (*fn)(struct work_struct *))
{

	work->work_queue = NULL;
	work->func = fn;
}

/*
 * acquire_work(work, wq)
 *
 *	Try to associate work with wq.  If work is already on a
 *	workqueue, return that workqueue.  Otherwise, set work's queue
 *	to wq, issue a memory barrier to match any prior release_work,
 *	and return NULL.
 *
 *	Caller must hold wq's lock.
 */
static struct workqueue_struct *
acquire_work(struct work_struct *work, struct workqueue_struct *wq)
{
	struct workqueue_struct *wq0;

	KASSERT(mutex_owned(&wq->wq_lock));

	wq0 = atomic_cas_ptr(&work->work_queue, NULL, wq);
	if (wq0 == NULL) {
		membar_enter();
		KASSERT(work->work_queue == wq);
	}

	return wq0;
}

/*
 * release_work(work, wq)
 *
 *	Issue a memory barrier to match any subsequent acquire_work and
 *	dissociate work from wq.
 *
 *	Caller must hold wq's lock and work must be associated with wq.
 */
static void
release_work(struct work_struct *work, struct workqueue_struct *wq)
{

	KASSERT(work->work_queue == wq);
	KASSERT(mutex_owned(&wq->wq_lock));

	membar_exit();
	work->work_queue = NULL;
}

/*
 * schedule_work(work)
 *
 *	If work is not already queued on system_wq, queue it to be run
 *	by system_wq's worker thread when it next can.  True if it was
 *	newly queued, false if it was already queued.  If the work was
 *	already running, queue it to run again.
 *
 *	Caller must ensure work is not queued to run on a different
 *	workqueue.
 */
bool
schedule_work(struct work_struct *work)
{

	return queue_work(system_wq, work);
}

/*
 * queue_work(wq, work)
 *
 *	If work is not already queued on wq, queue it to be run by wq's
 *	worker thread when it next can.  True if it was newly queued,
 *	false if it was already queued.  If the work was already
 *	running, queue it to run again.
 *
 *	Caller must ensure work is not queued to run on a different
 *	workqueue.
 */
bool
queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	struct workqueue_struct *wq0;
	bool newly_queued;

	KASSERT(wq != NULL);

	mutex_enter(&wq->wq_lock);
	if (__predict_true((wq0 = acquire_work(work, wq)) == NULL)) {
		/*
		 * It wasn't on any workqueue at all.  Put it on this
		 * one, and signal the worker thread that there is work
		 * to do.
		 */
		TAILQ_INSERT_TAIL(&wq->wq_queue, work, work_entry);
		newly_queued = true;
		cv_broadcast(&wq->wq_cv);
	} else {
		/*
		 * It was on a workqueue, which had better be this one.
		 * Requeue it if it has been taken off the queue to
		 * execute and hasn't been requeued yet.  The worker
		 * thread should already be running, so no need to
		 * signal it.
		 */
		KASSERT(wq0 == wq);
		if (wq->wq_current_work == work && !wq->wq_requeued) {
			/*
			 * It has been taken off the queue to execute,
			 * and it hasn't been put back on the queue
			 * again.  Put it back on the queue.  No need
			 * to signal the worker thread because it will
			 * notice when it reacquires the lock after
			 * doing the work.
			 */
			TAILQ_INSERT_TAIL(&wq->wq_queue, work, work_entry);
			wq->wq_requeued = true;
			newly_queued = true;
		} else {
			/* It is still on the queue; nothing to do.  */
			newly_queued = false;
		}
	}
	mutex_exit(&wq->wq_lock);

	return newly_queued;
}

/*
 * cancel_work(work)
 *
 *	If work was queued, remove it from the queue and return true.
 *	If work was not queued, return false.  Note that work may
 *	already be running; if it hasn't been requeued, then
 *	cancel_work will return false, and either way, cancel_work will
 *	NOT wait for the work to complete.
 */
bool
cancel_work(struct work_struct *work)
{
	struct workqueue_struct *wq;
	bool cancelled_p = false;

	/* If there's no workqueue, nothing to cancel.   */
	if ((wq = work->work_queue) == NULL)
		goto out;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work->work_queue != wq)) {
		/*
		 * It has finished execution or been cancelled by
		 * another thread, and has been moved off the
		 * workqueue, so it's too to cancel.
		 */
		cancelled_p = false;
	} else if (wq->wq_current_work == work) {
		/*
		 * It has already begun execution, so it's too late to
		 * cancel now.
		 */
		cancelled_p = false;
	} else {
		/*
		 * It is still on the queue.  Take it off the queue and
		 * report successful cancellation.
		 */
		TAILQ_REMOVE(&wq->wq_queue, work, work_entry);
		cancelled_p = true;
	}
	mutex_exit(&wq->wq_lock);

out:	return cancelled_p;
}

/*
 * cancel_work_sync(work)
 *
 *	If work was queued, remove it from the queue and return true.
 *	If work was not queued, return false.  Note that work may
 *	already be running; if it hasn't been requeued, then
 *	cancel_work will return false; either way, if work was
 *	currently running, wait for it to complete.
 *
 *	May sleep.
 */
bool
cancel_work_sync(struct work_struct *work)
{
	struct workqueue_struct *wq;
	bool cancelled_p = false;

	/* If there's no workqueue, nothing to cancel.   */
	if ((wq = work->work_queue) == NULL)
		goto out;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work->work_queue != wq)) {
		/*
		 * It has finished execution or been cancelled by
		 * another thread, and has been moved off the
		 * workqueue, so it's too to cancel.
		 */
		cancelled_p = false;
	} else if (wq->wq_current_work == work) {
		/*
		 * It has already begun execution, so it's too late to
		 * cancel now.  Wait for it to complete.
		 */
		wait_for_current_work(work, wq);
		cancelled_p = false;
	} else {
		/*
		 * It is still on the queue.  Take it off the queue and
		 * report successful cancellation.
		 */
		TAILQ_REMOVE(&wq->wq_queue, work, work_entry);
		cancelled_p = true;
	}
	mutex_exit(&wq->wq_lock);

out:	return cancelled_p;
}

/*
 * wait_for_current_work(work, wq)
 *
 *	wq must be currently executing work.  Wait for it to finish.
 */
static void
wait_for_current_work(struct work_struct *work, struct workqueue_struct *wq)
{
	uint64_t gen;

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(work->work_queue == wq);
	KASSERT(wq->wq_current_work == work);

	/* Wait only one generation in case it gets requeued quickly.  */
	gen = wq->wq_gen;
	do {
		cv_wait(&wq->wq_cv, &wq->wq_lock);
	} while (wq->wq_current_work == work && wq->wq_gen == gen);
}

/*
 * Delayed work
 */

/*
 * INIT_DELAYED_WORK(dw, fn)
 *
 *	Initialize dw for use with a workqueue to call fn in a worker
 *	thread after a delay.  There is no corresponding destruction
 *	operation.
 */
void
INIT_DELAYED_WORK(struct delayed_work *dw, void (*fn)(struct work_struct *))
{

	INIT_WORK(&dw->work, fn);
	dw->dw_state = DELAYED_WORK_IDLE;
	dw->dw_resched = -1;

	/*
	 * Defer callout_init until we are going to schedule the
	 * callout, which can then callout_destroy it, because
	 * otherwise since there's no DESTROY_DELAYED_WORK or anything
	 * we have no opportunity to call callout_destroy.
	 */
}

/*
 * schedule_delayed_work(dw, ticks)
 *
 *	If it is not currently scheduled, schedule dw to run after
 *	ticks on system_wq.  If currently executing and not already
 *	rescheduled, reschedule it.  True if it was newly scheduled,
 *	false if it was already scheduled.
 *
 *	If ticks == 0, queue it to run as soon as the worker can,
 *	without waiting for the next callout tick to run.
 */
bool
schedule_delayed_work(struct delayed_work *dw, unsigned long ticks)
{

	return queue_delayed_work(system_wq, dw, ticks);
}

/*
 * dw_callout_init(wq, dw)
 *
 *	Initialize the callout of dw and transition to
 *	DELAYED_WORK_SCHEDULED.  Caller must use callout_schedule.
 */
static void
dw_callout_init(struct workqueue_struct *wq, struct delayed_work *dw)
{

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(dw->work.work_queue == wq);
	KASSERT(dw->dw_state == DELAYED_WORK_IDLE);

	callout_init(&dw->dw_callout, CALLOUT_MPSAFE);
	callout_setfunc(&dw->dw_callout, &linux_workqueue_timeout, dw);
	TAILQ_INSERT_HEAD(&wq->wq_delayed, dw, dw_entry);
	dw->dw_state = DELAYED_WORK_SCHEDULED;
}

/*
 * dw_callout_destroy(wq, dw)
 *
 *	Destroy the callout of dw and transition to DELAYED_WORK_IDLE.
 */
static void
dw_callout_destroy(struct workqueue_struct *wq, struct delayed_work *dw)
{

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(dw->work.work_queue == wq);
	KASSERT(dw->dw_state == DELAYED_WORK_SCHEDULED ||
	    dw->dw_state == DELAYED_WORK_RESCHEDULED ||
	    dw->dw_state == DELAYED_WORK_CANCELLED);

	TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
	callout_destroy(&dw->dw_callout);
	dw->dw_resched = -1;
	dw->dw_state = DELAYED_WORK_IDLE;
}

/*
 * cancel_delayed_work_done(wq, dw)
 *
 *	Complete cancellation of a delayed work: transition from
 *	DELAYED_WORK_CANCELLED to DELAYED_WORK_IDLE and off the
 *	workqueue.  Caller must not touch dw after this returns.
 */
static void
cancel_delayed_work_done(struct workqueue_struct *wq, struct delayed_work *dw)
{

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(dw->work.work_queue == wq);
	KASSERT(dw->dw_state == DELAYED_WORK_CANCELLED);

	dw_callout_destroy(wq, dw);
	release_work(&dw->work, wq);
	/* Can't touch dw after this point.  */
}

/*
 * queue_delayed_work(wq, dw, ticks)
 *
 *	If it is not currently scheduled, schedule dw to run after
 *	ticks on wq.  If currently executing and not already
 *	rescheduled, reschedule it.
 *
 *	If ticks == 0, queue it to run as soon as the worker can,
 *	without waiting for the next callout tick to run.
 */
bool
queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	struct workqueue_struct *wq0;
	bool newly_queued;

	mutex_enter(&wq->wq_lock);
	if (__predict_true((wq0 = acquire_work(&dw->work, wq)) == NULL)) {
		/*
		 * It wasn't on any workqueue at all.  Schedule it to
		 * run on this one.
		 */
		KASSERT(dw->dw_state == DELAYED_WORK_IDLE);
		if (ticks == 0) {
			TAILQ_INSERT_TAIL(&wq->wq_queue, &dw->work,
			    work_entry);
			cv_broadcast(&wq->wq_cv);
		} else {
			/*
			 * Initialize a callout and schedule to run
			 * after a delay.
			 */
			dw_callout_init(wq, dw);
			callout_schedule(&dw->dw_callout, MIN(INT_MAX, ticks));
		}
		newly_queued = true;
	} else {
		/*
		 * It was on a workqueue, which had better be this one.
		 *
		 * - If it has already begun to run, and it is not yet
		 *   scheduled to run again, schedule it again.
		 *
		 * - If the callout is cancelled, reschedule it.
		 *
		 * - Otherwise, leave it alone.
		 */
		KASSERT(wq0 == wq);
		if (wq->wq_current_work != &dw->work || !wq->wq_requeued) {
			/*
			 * It is either scheduled, on the queue but not
			 * in progress, or in progress but not on the
			 * queue.
			 */
			switch (dw->dw_state) {
			case DELAYED_WORK_IDLE:
				/*
				 * It is not scheduled to run, and it
				 * is not on the queue if it is
				 * running.
				 */
				if (ticks == 0) {
					/*
					 * If it's in progress, put it
					 * on the queue to run as soon
					 * as the worker thread gets to
					 * it.  No need for a wakeup
					 * because either the worker
					 * thread already knows it is
					 * on the queue, or will check
					 * once it is done executing.
					 */
					if (wq->wq_current_work == &dw->work) {
						KASSERT(!wq->wq_requeued);
						TAILQ_INSERT_TAIL(&wq->wq_queue,
						    &dw->work, work_entry);
						wq->wq_requeued = true;
					}
				} else {
					/*
					 * Initialize a callout and
					 * schedule it to run after the
					 * specified delay.
					 */
					dw_callout_init(wq, dw);
					callout_schedule(&dw->dw_callout,
					    MIN(INT_MAX, ticks));
				}
				break;
			case DELAYED_WORK_SCHEDULED:
			case DELAYED_WORK_RESCHEDULED:
				/*
				 * It is already scheduled to run after
				 * a delay.  Leave it be.
				 */
				break;
			case DELAYED_WORK_CANCELLED:
				/*
				 * It was scheduled and the callout has
				 * begun to execute, but it was
				 * cancelled.  Reschedule it.
				 */
				dw->dw_state = DELAYED_WORK_RESCHEDULED;
				dw->dw_resched = MIN(INT_MAX, ticks);
				break;
			default:
				panic("invalid delayed work state: %d",
				    dw->dw_state);
			}
		} else {
			/*
			 * It is in progress and it has been requeued.
			 * It cannot be scheduled to run after a delay
			 * at this point.  We just leave it be.
			 */
			KASSERTMSG((dw->dw_state == DELAYED_WORK_IDLE),
			    "delayed work %p in wrong state: %d",
			    dw, dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return newly_queued;
}

/*
 * mod_delayed_work(wq, dw, ticks)
 *
 *	Schedule dw to run after ticks.  If currently scheduled,
 *	reschedule it.  If currently executing, reschedule it.  If
 *	ticks == 0, run without delay.
 *
 *	True if it modified the timer of an already scheduled work,
 *	false if it newly scheduled the work.
 */
bool
mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	struct workqueue_struct *wq0;
	bool timer_modified;

	mutex_enter(&wq->wq_lock);
	if ((wq0 = acquire_work(&dw->work, wq)) == NULL) {
		/*
		 * It wasn't on any workqueue at all.  Schedule it to
		 * run on this one.
		 */
		KASSERT(dw->dw_state == DELAYED_WORK_IDLE);
		if (ticks == 0) {
			/*
			 * Run immediately: put it on the queue and
			 * signal the worker thread.
			 */
			TAILQ_INSERT_TAIL(&wq->wq_queue, &dw->work,
			    work_entry);
			cv_broadcast(&wq->wq_cv);
		} else {
			/*
			 * Initialize a callout and schedule to run
			 * after a delay.
			 */
			dw_callout_init(wq, dw);
			callout_schedule(&dw->dw_callout, MIN(INT_MAX, ticks));
		}
		timer_modified = false;
	} else {
		/* It was on a workqueue, which had better be this one.  */
		KASSERT(wq0 == wq);
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It is not scheduled: it is on the queue or
			 * it is running or both.
			 */
			if (wq->wq_current_work != &dw->work ||
			    wq->wq_requeued) {
				/*
				 * It is on the queue, and it may or
				 * may not be running.
				 */
				if (ticks == 0) {
					/*
					 * We ask it to run
					 * immediately.  Leave it on
					 * the queue.
					 */
				} else {
					/*
					 * Take it off the queue and
					 * schedule a callout to run it
					 * after a delay.
					 */
					if (wq->wq_requeued) {
						wq->wq_requeued = false;
					} else {
						KASSERT(wq->wq_current_work !=
						    &dw->work);
					}
					TAILQ_REMOVE(&wq->wq_queue, &dw->work,
					    work_entry);
					dw_callout_init(wq, dw);
					callout_schedule(&dw->dw_callout,
					    MIN(INT_MAX, ticks));
				}
				timer_modified = true;
			} else {
				/*
				 * It is currently running and has not
				 * been requeued.
				 */
				if (ticks == 0) {
					/*
					 * We ask it to run
					 * immediately.  Put it on the
					 * queue again.
					 */
					wq->wq_requeued = true;
					TAILQ_INSERT_TAIL(&wq->wq_queue,
					    &dw->work, work_entry);
				} else {
					/*
					 * Schedule a callout to run it
					 * after a delay.
					 */
					dw_callout_init(wq, dw);
					callout_schedule(&dw->dw_callout,
					    MIN(INT_MAX, ticks));
				}
				timer_modified = false;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
			/*
			 * It is scheduled to run after a delay.  Try
			 * to stop it and reschedule it; if we can't,
			 * either reschedule it or cancel it to put it
			 * on the queue, and inform the callout.
			 */
			if (callout_stop(&dw->dw_callout)) {
				/* Can't stop, callout has begun.  */
				if (ticks == 0) {
					/*
					 * We don't actually need to do
					 * anything.  The callout will
					 * queue it as soon as it gets
					 * the lock.
					 */
				} else {
					/* Ask the callout to reschedule.  */
					dw->dw_state = DELAYED_WORK_RESCHEDULED;
					dw->dw_resched = MIN(INT_MAX, ticks);
				}
			} else {
				/* We stopped the callout before it began.  */
				if (ticks == 0) {
					/*
					 * Run immediately: destroy the
					 * callout, put it on the
					 * queue, and signal the worker
					 * thread.
					 */
					dw_callout_destroy(wq, dw);
					TAILQ_INSERT_TAIL(&wq->wq_queue,
					    &dw->work, work_entry);
					cv_broadcast(&wq->wq_cv);
				} else {
					/*
					 * Reschedule the callout.  No
					 * state change.
					 */
					callout_schedule(&dw->dw_callout,
					    MIN(INT_MAX, ticks));
				}
			}
			timer_modified = true;
			break;
		case DELAYED_WORK_RESCHEDULED:
			/*
			 * Someone rescheduled it after the callout
			 * started but before the poor thing even had a
			 * chance to acquire the lock.
			 */
			if (ticks == 0) {
				/*
				 * We can just switch back to
				 * DELAYED_WORK_SCHEDULED so that the
				 * callout will queue the work as soon
				 * as it gets the lock.
				 */
				dw->dw_state = DELAYED_WORK_SCHEDULED;
				dw->dw_resched = -1;
			} else {
				/* Change the rescheduled time.  */
				dw->dw_resched = ticks;
			}
			timer_modified = true;
			break;
		case DELAYED_WORK_CANCELLED:
			/*
			 * Someone cancelled it after the callout
			 * started but before the poor thing even had a
			 * chance to acquire the lock.
			 */
			if (ticks == 0) {
				/*
				 * We can just switch back to
				 * DELAYED_WORK_SCHEDULED so that the
				 * callout will queue the work as soon
				 * as it gets the lock.
				 */
				dw->dw_state = DELAYED_WORK_SCHEDULED;
			} else {
				/* Reschedule it.  */
				dw->dw_state = DELAYED_WORK_RESCHEDULED;
				dw->dw_resched = MIN(INT_MAX, ticks);
			}
			timer_modified = true;
			break;
		default:
			panic("invalid delayed work state: %d", dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return timer_modified;
}

/*
 * cancel_delayed_work(dw)
 *
 *	If work was scheduled or queued, remove it from the schedule or
 *	queue and return true.  If work was not scheduled or queued,
 *	return false.  Note that work may already be running; if it
 *	hasn't been rescheduled or requeued, then cancel_delayed_work
 *	will return false, and either way, cancel_delayed_work will NOT
 *	wait for the work to complete.
 */
bool
cancel_delayed_work(struct delayed_work *dw)
{
	struct workqueue_struct *wq;
	bool cancelled_p;

	/* If there's no workqueue, nothing to cancel.   */
	if ((wq = dw->work.work_queue) == NULL)
		return false;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(dw->work.work_queue != wq)) {
		cancelled_p = false;
	} else {
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It is either on the queue or already running
			 * or both.
			 */
			if (wq->wq_current_work != &dw->work ||
			    wq->wq_requeued) {
				/*
				 * It is on the queue, and it may or
				 * may not be running.  Remove it from
				 * the queue.
				 */
				TAILQ_REMOVE(&wq->wq_queue, &dw->work,
				    work_entry);
				if (wq->wq_current_work == &dw->work) {
					/*
					 * If it is running, then it
					 * must have been requeued in
					 * this case, so mark it no
					 * longer requeued.
					 */
					KASSERT(wq->wq_requeued);
					wq->wq_requeued = false;
				}
				cancelled_p = true;
			} else {
				/*
				 * Too late, it's already running, but
				 * at least it hasn't been requeued.
				 */
				cancelled_p = false;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
			/*
			 * If it is scheduled, mark it cancelled and
			 * try to stop the callout before it starts.
			 *
			 * If it's too late and the callout has already
			 * begun to execute, tough.
			 *
			 * If we stopped the callout before it started,
			 * however, then destroy the callout and
			 * dissociate it from the workqueue ourselves.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			cancelled_p = true;
			if (!callout_stop(&dw->dw_callout))
				cancel_delayed_work_done(wq, dw);
			break;
		case DELAYED_WORK_RESCHEDULED:
			/*
			 * If it is being rescheduled, the callout has
			 * already fired.  We must ask it to cancel.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			dw->dw_resched = -1;
			cancelled_p = true;
			break;
		case DELAYED_WORK_CANCELLED:
			/*
			 * If it is being cancelled, the callout has
			 * already fired.  There is nothing more for us
			 * to do.  Someone else claims credit for
			 * cancelling it.
			 */
			cancelled_p = false;
			break;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return cancelled_p;
}

/*
 * cancel_delayed_work_sync(dw)
 *
 *	If work was scheduled or queued, remove it from the schedule or
 *	queue and return true.  If work was not scheduled or queued,
 *	return false.  Note that work may already be running; if it
 *	hasn't been rescheduled or requeued, then cancel_delayed_work
 *	will return false; either way, wait for it to complete.
 */
bool
cancel_delayed_work_sync(struct delayed_work *dw)
{
	struct workqueue_struct *wq;
	bool cancelled_p;

	/* If there's no workqueue, nothing to cancel.  */
	if ((wq = dw->work.work_queue) == NULL)
		return false;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(dw->work.work_queue != wq)) {
		cancelled_p = false;
	} else {
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It is either on the queue or already running
			 * or both.
			 */
			if (wq->wq_current_work == &dw->work) {
				/*
				 * Too late, it's already running.
				 * First, make sure it's not requeued.
				 * Then wait for it to complete.
				 */
				if (wq->wq_requeued) {
					TAILQ_REMOVE(&wq->wq_queue, &dw->work,
					    work_entry);
					wq->wq_requeued = false;
				}
				wait_for_current_work(&dw->work, wq);
				cancelled_p = false;
			} else {
				/* Got in before it started.  Remove it.  */
				TAILQ_REMOVE(&wq->wq_queue, &dw->work,
				    work_entry);
				cancelled_p = true;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
			/*
			 * If it is scheduled, mark it cancelled and
			 * try to stop the callout before it starts.
			 *
			 * If it's too late and the callout has already
			 * begun to execute, we must wait for it to
			 * complete.  But we got in soon enough to ask
			 * the callout not to run, so we successfully
			 * cancelled it in that case.
			 *
			 * If we stopped the callout before it started,
			 * then we must destroy the callout and
			 * dissociate it from the workqueue ourselves.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			if (!callout_halt(&dw->dw_callout, &wq->wq_lock))
				cancel_delayed_work_done(wq, dw);
			cancelled_p = true;
			break;
		case DELAYED_WORK_RESCHEDULED:
			/*
			 * If it is being rescheduled, the callout has
			 * already fired.  We must ask it to cancel and
			 * wait for it to complete.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			dw->dw_resched = -1;
			(void)callout_halt(&dw->dw_callout, &wq->wq_lock);
			cancelled_p = true;
			break;
		case DELAYED_WORK_CANCELLED:
			/*
			 * If it is being cancelled, the callout has
			 * already fired.  We need only wait for it to
			 * complete.  Someone else, however, claims
			 * credit for cancelling it.
			 */
			(void)callout_halt(&dw->dw_callout, &wq->wq_lock);
			cancelled_p = false;
			break;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return cancelled_p;
}

/*
 * Flush
 */

/*
 * flush_scheduled_work()
 *
 *	Wait for all work queued on system_wq to complete.  This does
 *	not include delayed work.
 */
void
flush_scheduled_work(void)
{

	flush_workqueue(system_wq);
}

/*
 * flush_workqueue_locked(wq)
 *
 *	Wait for all work queued on wq to complete.  This does not
 *	include delayed work.
 *
 *	Caller must hold wq's lock.
 */
static void
flush_workqueue_locked(struct workqueue_struct *wq)
{
	uint64_t gen;

	KASSERT(mutex_owned(&wq->wq_lock));

	/* Get the current generation number.  */
	gen = wq->wq_gen;

	/*
	 * If there's a batch of work in progress, we must wait for the
	 * worker thread to finish that batch.
	 */
	if (wq->wq_current_work != NULL)
		gen++;

	/*
	 * If there's any work yet to be claimed from the queue by the
	 * worker thread, we must wait for it to finish one more batch
	 * too.
	 */
	if (!TAILQ_EMPTY(&wq->wq_queue))
		gen++;

	/* Wait until the generation number has caught up.  */
	while (wq->wq_gen < gen)
		cv_wait(&wq->wq_cv, &wq->wq_lock);
}

/*
 * flush_workqueue(wq)
 *
 *	Wait for all work queued on wq to complete.  This does not
 *	include delayed work.
 */
void
flush_workqueue(struct workqueue_struct *wq)
{

	mutex_enter(&wq->wq_lock);
	flush_workqueue_locked(wq);
	mutex_exit(&wq->wq_lock);
}

/*
 * flush_work(work)
 *
 *	If work is queued or currently executing, wait for it to
 *	complete.
 */
void
flush_work(struct work_struct *work)
{
	struct workqueue_struct *wq;

	/* If there's no workqueue, nothing to flush.  */
	if ((wq = work->work_queue) == NULL)
		return;

	flush_workqueue(wq);
}

/*
 * flush_delayed_work(dw)
 *
 *	If dw is scheduled to run after a delay, cancel it.  If dw is
 *	queued or currently executing, wait for it to complete.
 */
void
flush_delayed_work(struct delayed_work *dw)
{
	struct workqueue_struct *wq;

	/* If there's no workqueue, nothing to flush.  */
	if ((wq = dw->work.work_queue) == NULL)
		return;

	mutex_enter(&wq->wq_lock);
	if (__predict_true(dw->work.work_queue == wq)) {
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It has a workqueue assigned and the callout
			 * is idle, so it must be in progress or on the
			 * queue.  In that case, wait for it to
			 * complete.  Waiting for the whole queue to
			 * flush is overkill, but doesn't hurt.
			 */
			flush_workqueue_locked(wq);
			break;
		case DELAYED_WORK_SCHEDULED:
			/*
			 * If it is scheduled, mark it cancelled and
			 * try to stop the callout before it starts.
			 *
			 * If it's too late and the callout has already
			 * begun to execute, we must wait for it to
			 * complete.  But we got in soon enough to ask
			 * the callout not to run.
			 *
			 * If we stopped the callout before it started,
			 * then we must destroy the callout and
			 * dissociate it from the workqueue ourselves.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			if (!callout_halt(&dw->dw_callout, &wq->wq_lock))
				cancel_delayed_work_done(wq, dw);
			break;
		case DELAYED_WORK_RESCHEDULED:
			/*
			 * If it is being rescheduled, the callout has
			 * already fired.  We must ask it to cancel and
			 * wait for it to complete.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			dw->dw_resched = -1;
			(void)callout_halt(&dw->dw_callout, &wq->wq_lock);
			break;
		case DELAYED_WORK_CANCELLED:
			/*
			 * If it is being cancelled, the callout has
			 * already fired.  We need only wait for it to
			 * complete.
			 */
			(void)callout_halt(&dw->dw_callout, &wq->wq_lock);
			break;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);
}
