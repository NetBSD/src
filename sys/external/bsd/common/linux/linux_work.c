/*	$NetBSD: linux_work.c,v 1.15 2018/08/27 14:58:24 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_work.c,v 1.15 2018/08/27 14:58:24 riastradh Exp $");

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
static void		queue_delayed_work_anew(struct workqueue_struct *,
			    struct delayed_work *, unsigned long);

static specificdata_key_t workqueue_key __read_mostly;

struct workqueue_struct	*system_wq __read_mostly;
struct workqueue_struct	*system_long_wq __read_mostly;
struct workqueue_struct	*system_power_efficient_wq __read_mostly;

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

struct workqueue_struct *
alloc_ordered_workqueue(const char *name, int flags)
{
	struct workqueue_struct *wq;
	int error;

	KASSERT(flags == 0);

	wq = kmem_alloc(sizeof(*wq), KM_SLEEP);

	mutex_init(&wq->wq_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&wq->wq_cv, name);
	TAILQ_INIT(&wq->wq_delayed);
	TAILQ_INIT(&wq->wq_queue);
	wq->wq_current_work = NULL;

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

void
destroy_workqueue(struct workqueue_struct *wq)
{

	/*
	 * Cancel all delayed work.  We do this first because any
	 * delayed work that that has already timed out, which we can't
	 * cancel, may have queued new work.
	 */
	for (;;) {
		struct delayed_work *dw = NULL;

		mutex_enter(&wq->wq_lock);
		if (!TAILQ_EMPTY(&wq->wq_delayed)) {
			dw = TAILQ_FIRST(&wq->wq_delayed);
			if (!callout_halt(&dw->dw_callout, &wq->wq_lock))
				TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
		}
		mutex_exit(&wq->wq_lock);

		if (dw == NULL)
			break;
		cancel_delayed_work_sync(dw);
	}

	/* Tell the thread to exit.  */
	mutex_enter(&wq->wq_lock);
	wq->wq_dying = true;
	cv_broadcast(&wq->wq_cv);
	mutex_exit(&wq->wq_lock);

	/* Wait for it to exit.  */
	(void)kthread_join(wq->wq_lwp);

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

static void __dead
linux_workqueue_thread(void *cookie)
{
	struct workqueue_struct *const wq = cookie;
	TAILQ_HEAD(, work_struct) tmp;

	lwp_setspecific(workqueue_key, wq);

	mutex_enter(&wq->wq_lock);
	for (;;) {
		/* Wait until there's activity.  If we're dying, stop.  */
		while (TAILQ_EMPTY(&wq->wq_queue) && !wq->wq_dying)
			cv_wait(&wq->wq_cv, &wq->wq_lock);
		if (wq->wq_dying)
			break;

		/* Grab a batch of work off the queue.  */
		KASSERT(!TAILQ_EMPTY(&wq->wq_queue));
		TAILQ_INIT(&tmp);
		TAILQ_CONCAT(&tmp, &wq->wq_queue, work_entry);

		/* Process each work item in the batch.  */
		while (!TAILQ_EMPTY(&tmp)) {
			struct work_struct *const work = TAILQ_FIRST(&tmp);

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
				work->work_queue = NULL;
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

static void
linux_workqueue_timeout(void *cookie)
{
	struct delayed_work *const dw = cookie;
	struct workqueue_struct *const wq = dw->work.work_queue;

	KASSERT(wq != NULL);

	mutex_enter(&wq->wq_lock);
	switch (dw->dw_state) {
	case DELAYED_WORK_IDLE:
		panic("delayed work callout uninitialized: %p", dw);
	case DELAYED_WORK_SCHEDULED:
		dw->dw_state = DELAYED_WORK_IDLE;
		callout_destroy(&dw->dw_callout);
		TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
		TAILQ_INSERT_TAIL(&wq->wq_queue, &dw->work, work_entry);
		cv_broadcast(&wq->wq_cv);
		break;
	case DELAYED_WORK_RESCHEDULED:
		dw->dw_state = DELAYED_WORK_SCHEDULED;
		break;
	case DELAYED_WORK_CANCELLED:
		dw->dw_state = DELAYED_WORK_IDLE;
		callout_destroy(&dw->dw_callout);
		TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
		break;
	default:
		panic("delayed work callout in bad state: %p", dw);
	}
	KASSERT(dw->dw_state == DELAYED_WORK_IDLE ||
	    dw->dw_state == DELAYED_WORK_SCHEDULED);
	mutex_exit(&wq->wq_lock);
}

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

void
INIT_WORK(struct work_struct *work, void (*fn)(struct work_struct *))
{

	work->work_queue = NULL;
	work->func = fn;
}

bool
schedule_work(struct work_struct *work)
{

	return queue_work(system_wq, work);
}

bool
queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	struct workqueue_struct *wq0;
	bool newly_queued;

	KASSERT(wq != NULL);

	mutex_enter(&wq->wq_lock);
	if (__predict_true((wq0 = atomic_cas_ptr(&work->work_queue, NULL, wq))
		== NULL)) {
		TAILQ_INSERT_TAIL(&wq->wq_queue, work, work_entry);
		newly_queued = true;
	} else {
		KASSERT(wq0 == wq);
		newly_queued = false;
	}
	mutex_exit(&wq->wq_lock);

	return newly_queued;
}

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
		cancelled_p = false;
	} else if (wq->wq_current_work == work) {
		cancelled_p = false;
	} else {
		TAILQ_REMOVE(&wq->wq_queue, work, work_entry);
		cancelled_p = true;
	}
	mutex_exit(&wq->wq_lock);

out:	return cancelled_p;
}

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
		cancelled_p = false;
	} else if (wq->wq_current_work == work) {
		do {
			cv_wait(&wq->wq_cv, &wq->wq_lock);
		} while (wq->wq_current_work == work);
		cancelled_p = false;
	} else {
		TAILQ_REMOVE(&wq->wq_queue, work, work_entry);
		cancelled_p = true;
	}
	mutex_exit(&wq->wq_lock);

out:	return cancelled_p;
}

/*
 * Delayed work
 */

void
INIT_DELAYED_WORK(struct delayed_work *dw, void (*fn)(struct work_struct *))
{

	INIT_WORK(&dw->work, fn);
	dw->dw_state = DELAYED_WORK_IDLE;

	/*
	 * Defer callout_init until we are going to schedule the
	 * callout, which can then callout_destroy it, because
	 * otherwise since there's no DESTROY_DELAYED_WORK or anything
	 * we have no opportunity to call callout_destroy.
	 */
}

bool
schedule_delayed_work(struct delayed_work *dw, unsigned long ticks)
{

	return queue_delayed_work(system_wq, dw, ticks);
}

static void
queue_delayed_work_anew(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(dw->work.work_queue == wq);
	KASSERT((dw->dw_state == DELAYED_WORK_IDLE) ||
	    (dw->dw_state == DELAYED_WORK_SCHEDULED));

	if (ticks == 0) {
		if (dw->dw_state == DELAYED_WORK_SCHEDULED) {
			callout_destroy(&dw->dw_callout);
			TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
		} else {
			KASSERT(dw->dw_state == DELAYED_WORK_IDLE);
		}
		TAILQ_INSERT_TAIL(&wq->wq_queue, &dw->work, work_entry);
		dw->dw_state = DELAYED_WORK_IDLE;
	} else {
		if (dw->dw_state == DELAYED_WORK_IDLE) {
			callout_init(&dw->dw_callout, CALLOUT_MPSAFE);
			callout_reset(&dw->dw_callout, MIN(INT_MAX, ticks),
			    &linux_workqueue_timeout, dw);
			TAILQ_INSERT_HEAD(&wq->wq_delayed, dw, dw_entry);
		} else {
			KASSERT(dw->dw_state == DELAYED_WORK_SCHEDULED);
		}
		dw->dw_state = DELAYED_WORK_SCHEDULED;
	}
}

bool
queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	struct workqueue_struct *wq0;
	bool newly_queued;

	mutex_enter(&wq->wq_lock);
	if (__predict_true((wq0 = atomic_cas_ptr(&dw->work.work_queue, NULL,
			wq)) == NULL)) {
		KASSERT(dw->dw_state == DELAYED_WORK_IDLE);
		queue_delayed_work_anew(wq, dw, ticks);
		newly_queued = true;
	} else {
		KASSERT(wq0 == wq);
		newly_queued = false;
	}
	mutex_exit(&wq->wq_lock);

	return newly_queued;
}

bool
mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	struct workqueue_struct *wq0;
	bool timer_modified;

	mutex_enter(&wq->wq_lock);
	if ((wq0 = atomic_cas_ptr(&dw->work.work_queue, NULL, wq)) == NULL) {
		KASSERT(dw->dw_state == DELAYED_WORK_IDLE);
		queue_delayed_work_anew(wq, dw, ticks);
		timer_modified = false;
	} else {
		KASSERT(wq0 == wq);
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			if (wq->wq_current_work != &dw->work) {
				/* Work is queued, but hasn't started yet.  */
				TAILQ_REMOVE(&wq->wq_queue, &dw->work,
				    work_entry);
				queue_delayed_work_anew(wq, dw, ticks);
				timer_modified = true;
			} else {
				/*
				 * Too late.  Queue it anew.  If that
				 * would skip the callout because it's
				 * immediate, notify the workqueue.
				 */
				wq->wq_requeued = ticks == 0;
				queue_delayed_work_anew(wq, dw, ticks);
				timer_modified = false;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
			if (callout_stop(&dw->dw_callout)) {
				/*
				 * Too late to stop, but we got in
				 * before the callout acquired the
				 * lock.  Reschedule it and tell it
				 * we've done so.
				 */
				dw->dw_state = DELAYED_WORK_RESCHEDULED;
				callout_schedule(&dw->dw_callout,
				    MIN(INT_MAX, ticks));
			} else {
				/* Stopped it.  Queue it anew.  */
				queue_delayed_work_anew(wq, dw, ticks);
			}
			timer_modified = true;
			break;
		case DELAYED_WORK_RESCHEDULED:
		case DELAYED_WORK_CANCELLED:
			/*
			 * Someone modified the timer _again_, or
			 * cancelled it, after the callout started but
			 * before the poor thing even had a chance to
			 * acquire the lock.  Just reschedule it once
			 * more.
			 */
			callout_schedule(&dw->dw_callout, MIN(INT_MAX, ticks));
			dw->dw_state = DELAYED_WORK_RESCHEDULED;
			timer_modified = true;
			break;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return timer_modified;
}

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
			if (wq->wq_current_work == &dw->work) {
				/* Too late, it's already running.  */
				cancelled_p = false;
			} else {
				/* Got in before it started.  Remove it.  */
				TAILQ_REMOVE(&wq->wq_queue, &dw->work,
				    work_entry);
				cancelled_p = true;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
		case DELAYED_WORK_RESCHEDULED:
		case DELAYED_WORK_CANCELLED:
			if (callout_stop(&dw->dw_callout)) {
				/*
				 * Too late to stop, but we got in
				 * before the callout acquired the
				 * lock.  Tell it to give up.
				 */
				dw->dw_state = DELAYED_WORK_CANCELLED;
			} else {
				/* Stopped it.  Kill it.  */
				callout_destroy(&dw->dw_callout);
				TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
				dw->dw_state = DELAYED_WORK_IDLE;
			}
			cancelled_p = true;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return cancelled_p;
}

bool
cancel_delayed_work_sync(struct delayed_work *dw)
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
retry:		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			if (wq->wq_current_work == &dw->work) {
				/* Too late, it's already running.  Wait.  */
				do {
					cv_wait(&wq->wq_cv, &wq->wq_lock);
				} while (wq->wq_current_work == &dw->work);
				cancelled_p = false;
			} else {
				/* Got in before it started.  Remove it.  */
				TAILQ_REMOVE(&wq->wq_queue, &dw->work,
				    work_entry);
				cancelled_p = true;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
		case DELAYED_WORK_RESCHEDULED:
		case DELAYED_WORK_CANCELLED:
			/*
			 * If it has started, tell it to stop, and wait
			 * for it to complete.  We drop the lock, so by
			 * the time the callout has completed, we must
			 * review the state again.
			 */
			dw->dw_state = DELAYED_WORK_CANCELLED;
			callout_halt(&dw->dw_callout, &wq->wq_lock);
			goto retry;
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

void
flush_scheduled_work(void)
{

	flush_workqueue(system_wq);
}

void
flush_workqueue(struct workqueue_struct *wq)
{
	uint64_t gen;

	mutex_enter(&wq->wq_lock);
	gen = wq->wq_gen;
	do {
		cv_wait(&wq->wq_cv, &wq->wq_lock);
	} while (gen == wq->wq_gen);
	mutex_exit(&wq->wq_lock);
}

bool
flush_work(struct work_struct *work)
{
	struct workqueue_struct *wq;

	/* If there's no workqueue, nothing to flush.  */
	if ((wq = work->work_queue) == NULL)
		return false;

	flush_workqueue(wq);
	return true;
}

bool
flush_delayed_work(struct delayed_work *dw)
{
	struct workqueue_struct *wq;
	bool do_flush = false;

	/* If there's no workqueue, nothing to flush.  */
	if ((wq = dw->work.work_queue) == NULL)
		return false;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(dw->work.work_queue != wq)) {
		do_flush = true;
	} else {
retry:		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			if (wq->wq_current_work != &dw->work) {
				TAILQ_REMOVE(&wq->wq_queue, &dw->work,
				    work_entry);
			} else {
				do_flush = true;
			}
			break;
		case DELAYED_WORK_SCHEDULED:
		case DELAYED_WORK_RESCHEDULED:
		case DELAYED_WORK_CANCELLED:
			dw->dw_state = DELAYED_WORK_CANCELLED;
			callout_halt(&dw->dw_callout, &wq->wq_lock);
			goto retry;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	if (do_flush)
		flush_workqueue(wq);

	return true;
}
