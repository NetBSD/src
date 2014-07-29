/*	$NetBSD: linux_work.c,v 1.7 2014/07/29 17:36:06 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_work.c,v 1.7 2014/07/29 17:36:06 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <machine/lock.h>

#include <linux/workqueue.h>

/* XXX Kludge until we sync with HEAD.  */
#if DIAGNOSTIC
#define	__diagused
#else
#define	__diagused	__unused
#endif

struct workqueue_struct {
	struct workqueue		*wq_workqueue;

	/* XXX The following should all be per-CPU.  */
	kmutex_t			wq_lock;

	/*
	 * Condvar for when any state related to this workqueue
	 * changes.  XXX Could split this into multiple condvars for
	 * different purposes, but whatever...
	 */
	kcondvar_t			wq_cv;

	TAILQ_HEAD(, delayed_work)	wq_delayed;
	struct work_struct		*wq_current_work;
};

static void	linux_work_lock_init(struct work_struct *);
static void	linux_work_lock(struct work_struct *);
static void	linux_work_unlock(struct work_struct *);
static bool	linux_work_locked(struct work_struct *) __diagused;

static void	linux_wq_barrier(struct work_struct *);

static void	linux_wait_for_cancelled_work(struct work_struct *);
static void	linux_wait_for_invoked_work(struct work_struct *);
static void	linux_worker(struct work *, void *);

static void	linux_cancel_delayed_work_callout(struct delayed_work *, bool);
static void	linux_wait_for_delayed_cancelled_work(struct delayed_work *);
static void	linux_worker_intr(void *);

struct workqueue_struct		*system_wq;

int
linux_workqueue_init(void)
{

	system_wq = alloc_ordered_workqueue("lnxsyswq", 0);
	if (system_wq == NULL)
		return ENOMEM;

	return 0;
}

void
linux_workqueue_fini(void)
{
	destroy_workqueue(system_wq);
	system_wq = NULL;
}

/*
 * Workqueues
 */

struct workqueue_struct *
alloc_ordered_workqueue(const char *name, int linux_flags)
{
	struct workqueue_struct *wq;
	int flags = WQ_MPSAFE;
	int error;

	KASSERT(linux_flags == 0);

	wq = kmem_alloc(sizeof(*wq), KM_SLEEP);
	error = workqueue_create(&wq->wq_workqueue, name, &linux_worker,
	    wq, PRI_NONE, IPL_VM, flags);
	if (error) {
		kmem_free(wq, sizeof(*wq));
		return NULL;
	}

	mutex_init(&wq->wq_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&wq->wq_cv, name);
	TAILQ_INIT(&wq->wq_delayed);
	wq->wq_current_work = NULL;

	return wq;
}

void
destroy_workqueue(struct workqueue_struct *wq)
{

	/*
	 * Cancel all delayed work.
	 */
	for (;;) {
		struct delayed_work *dw;

		mutex_enter(&wq->wq_lock);
		if (TAILQ_EMPTY(&wq->wq_delayed)) {
			dw = NULL;
		} else {
			dw = TAILQ_FIRST(&wq->wq_delayed);
			TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
		}
		mutex_exit(&wq->wq_lock);

		if (dw == NULL)
			break;

		cancel_delayed_work_sync(dw);
	}

	/*
	 * workqueue_destroy empties the queue; we need not wait for
	 * completion explicitly.  However, we can't destroy the
	 * condvar or mutex until this is done.
	 */
	workqueue_destroy(wq->wq_workqueue);
	KASSERT(wq->wq_current_work == NULL);
	wq->wq_workqueue = NULL;

	cv_destroy(&wq->wq_cv);
	mutex_destroy(&wq->wq_lock);

	kmem_free(wq, sizeof(*wq));
}

/*
 * Flush
 *
 * Note:  This doesn't cancel or wait for delayed work.  This seems to
 * match what Linux does (or, doesn't do).
 */

void
flush_scheduled_work(void)
{
	flush_workqueue(system_wq);
}

struct wq_flush_work {
	struct work_struct	wqfw_work;
	struct wq_flush		*wqfw_flush;
};

struct wq_flush {
	kmutex_t	wqf_lock;
	kcondvar_t	wqf_cv;
	unsigned int	wqf_n;
};

void
flush_work(struct work_struct *work)
{
	struct workqueue_struct *const wq = work->w_wq;

	if (wq != NULL)
		flush_workqueue(wq);
}

void
flush_workqueue(struct workqueue_struct *wq)
{
	static const struct wq_flush zero_wqf;
	struct wq_flush wqf = zero_wqf;

	mutex_init(&wqf.wqf_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&wqf.wqf_cv, "lnxwflsh");

	if (1) {
		struct wq_flush_work *const wqfw = kmem_zalloc(sizeof(*wqfw),
		    KM_SLEEP);

		wqf.wqf_n = 1;
		wqfw->wqfw_flush = &wqf;
		INIT_WORK(&wqfw->wqfw_work, &linux_wq_barrier);
		wqfw->wqfw_work.w_wq = wq;
		wqfw->wqfw_work.w_state = WORK_PENDING;
		workqueue_enqueue(wq->wq_workqueue, &wqfw->wqfw_work.w_wk,
		    NULL);
	} else {
		struct cpu_info *ci;
		CPU_INFO_ITERATOR cii;
		struct wq_flush_work *wqfw;

		panic("per-CPU Linux workqueues don't work yet!");

		wqf.wqf_n = 0;
		for (CPU_INFO_FOREACH(cii, ci)) {
			wqfw = kmem_zalloc(sizeof(*wqfw), KM_SLEEP);
			mutex_enter(&wqf.wqf_lock);
			wqf.wqf_n++;
			mutex_exit(&wqf.wqf_lock);
			wqfw->wqfw_flush = &wqf;
			INIT_WORK(&wqfw->wqfw_work, &linux_wq_barrier);
			wqfw->wqfw_work.w_state = WORK_PENDING;
			wqfw->wqfw_work.w_wq = wq;
			workqueue_enqueue(wq->wq_workqueue,
			    &wqfw->wqfw_work.w_wk, ci);
		}
	}

	mutex_enter(&wqf.wqf_lock);
	while (0 < wqf.wqf_n)
		cv_wait(&wqf.wqf_cv, &wqf.wqf_lock);
	mutex_exit(&wqf.wqf_lock);

	cv_destroy(&wqf.wqf_cv);
	mutex_destroy(&wqf.wqf_lock);
}

static void
linux_wq_barrier(struct work_struct *work)
{
	struct wq_flush_work *const wqfw = container_of(work,
	    struct wq_flush_work, wqfw_work);
	struct wq_flush *const wqf = wqfw->wqfw_flush;

	mutex_enter(&wqf->wqf_lock);
	if (--wqf->wqf_n == 0)
		cv_broadcast(&wqf->wqf_cv);
	mutex_exit(&wqf->wqf_lock);

	kmem_free(wqfw, sizeof(*wqfw));
}

/*
 * Work locking
 *
 * We use __cpu_simple_lock(9) rather than mutex(9) because Linux code
 * does not destroy work, so there is nowhere to call mutex_destroy.
 *
 * XXX This is getting out of hand...  Really, work items shouldn't
 * have locks in them at all; instead the workqueues should.
 */

static void
linux_work_lock_init(struct work_struct *work)
{

	__cpu_simple_lock_init(&work->w_lock);
}

static void
linux_work_lock(struct work_struct *work)
{
	struct cpu_info *ci;
	int cnt, s;

	/* XXX Copypasta of MUTEX_SPIN_SPLRAISE.  */
	s = splvm();
	ci = curcpu();
	cnt = ci->ci_mtx_count--;
	__insn_barrier();
	if (cnt == 0)
		ci->ci_mtx_oldspl = s;

	__cpu_simple_lock(&work->w_lock);
}

static void
linux_work_unlock(struct work_struct *work)
{
	struct cpu_info *ci;
	int s;

	__cpu_simple_unlock(&work->w_lock);

	/* XXX Copypasta of MUTEX_SPIN_SPLRESTORE.  */
	ci = curcpu();
	s = ci->ci_mtx_oldspl;
	__insn_barrier();
	if (++ci->ci_mtx_count == 0)
		splx(s);
}

static bool __diagused
linux_work_locked(struct work_struct *work)
{
	return __SIMPLELOCK_LOCKED_P(&work->w_lock);
}

/*
 * Work
 */

void
INIT_WORK(struct work_struct *work, void (*fn)(struct work_struct *))
{

	linux_work_lock_init(work);
	work->w_state = WORK_IDLE;
	work->w_wq = NULL;
	work->w_fn = fn;
}

bool
schedule_work(struct work_struct *work)
{
	return queue_work(system_wq, work);
}

bool
queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	/* True if we put it on the queue, false if it was already there.  */
	bool newly_queued;

	KASSERT(wq != NULL);

	linux_work_lock(work);
	switch (work->w_state) {
	case WORK_IDLE:
	case WORK_INVOKED:
		work->w_state = WORK_PENDING;
		work->w_wq = wq;
		workqueue_enqueue(wq->wq_workqueue, &work->w_wk, NULL);
		newly_queued = true;
		break;

	case WORK_DELAYED:
		panic("queue_work(delayed work %p)", work);
		break;

	case WORK_PENDING:
		KASSERT(work->w_wq == wq);
		newly_queued = false;
		break;

	case WORK_CANCELLED:
		newly_queued = false;
		break;

	case WORK_DELAYED_CANCELLED:
		panic("queue_work(delayed work %p)", work);
		break;

	default:
		panic("work %p in bad state: %d", work, (int)work->w_state);
		break;
	}
	linux_work_unlock(work);

	return newly_queued;
}

bool
cancel_work_sync(struct work_struct *work)
{
	bool cancelled_p = false;

	linux_work_lock(work);
	switch (work->w_state) {
	case WORK_IDLE:		/* Nothing to do.  */
		break;

	case WORK_DELAYED:
		panic("cancel_work_sync(delayed work %p)", work);
		break;

	case WORK_PENDING:
		work->w_state = WORK_CANCELLED;
		linux_wait_for_cancelled_work(work);
		cancelled_p = true;
		break;

	case WORK_INVOKED:
		linux_wait_for_invoked_work(work);
		break;

	case WORK_CANCELLED:	/* Already done.  */
		break;

	case WORK_DELAYED_CANCELLED:
		panic("cancel_work_sync(delayed work %p)", work);
		break;

	default:
		panic("work %p in bad state: %d", work, (int)work->w_state);
		break;
	}
	linux_work_unlock(work);

	return cancelled_p;
}

static void
linux_wait_for_cancelled_work(struct work_struct *work)
{
	struct workqueue_struct *wq;

	KASSERT(linux_work_locked(work));
	KASSERT(work->w_state == WORK_CANCELLED);

	wq = work->w_wq;
	do {
		mutex_enter(&wq->wq_lock);
		linux_work_unlock(work);
		cv_wait(&wq->wq_cv, &wq->wq_lock);
		mutex_exit(&wq->wq_lock);
		linux_work_lock(work);
	} while ((work->w_state == WORK_CANCELLED) && (work->w_wq == wq));
}

static void
linux_wait_for_invoked_work(struct work_struct *work)
{
	struct workqueue_struct *wq;

	KASSERT(linux_work_locked(work));
	KASSERT(work->w_state == WORK_INVOKED);

	wq = work->w_wq;
	mutex_enter(&wq->wq_lock);
	linux_work_unlock(work);
	while (wq->wq_current_work == work)
		cv_wait(&wq->wq_cv, &wq->wq_lock);
	mutex_exit(&wq->wq_lock);

	linux_work_lock(work);	/* XXX needless relock */
}

static void
linux_worker(struct work *wk, void *arg)
{
	struct work_struct *const work = container_of(wk, struct work_struct,
	    w_wk);
	struct workqueue_struct *const wq = arg;

	linux_work_lock(work);
	switch (work->w_state) {
	case WORK_IDLE:
		panic("idle work %p got queued: %p", work, wq);
		break;

	case WORK_DELAYED:
		panic("delayed work %p got queued: %p", work, wq);
		break;

	case WORK_PENDING:
		KASSERT(work->w_wq == wq);

		/* Get ready to invoke this one.  */
		mutex_enter(&wq->wq_lock);
		work->w_state = WORK_INVOKED;
		KASSERT(wq->wq_current_work == NULL);
		wq->wq_current_work = work;
		mutex_exit(&wq->wq_lock);

		/* Unlock it and do it.  Can't use work after this.  */
		linux_work_unlock(work);
		(*work->w_fn)(work);

		/* All done.  Notify anyone waiting for completion.  */
		mutex_enter(&wq->wq_lock);
		KASSERT(wq->wq_current_work == work);
		wq->wq_current_work = NULL;
		cv_broadcast(&wq->wq_cv);
		mutex_exit(&wq->wq_lock);
		return;

	case WORK_INVOKED:
		panic("invoked work %p got requeued: %p", work, wq);
		break;

	case WORK_CANCELLED:
		KASSERT(work->w_wq == wq);

		/* Return to idle; notify anyone waiting for cancellation.  */
		mutex_enter(&wq->wq_lock);
		work->w_state = WORK_IDLE;
		work->w_wq = NULL;
		cv_broadcast(&wq->wq_cv);
		mutex_exit(&wq->wq_lock);
		break;

	case WORK_DELAYED_CANCELLED:
		panic("cancelled delayed work %p got uqeued: %p", work, wq);
		break;

	default:
		panic("work %p in bad state: %d", work, (int)work->w_state);
		break;
	}
	linux_work_unlock(work);
}

/*
 * Delayed work
 */

void
INIT_DELAYED_WORK(struct delayed_work *dw, void (*fn)(struct work_struct *))
{
	INIT_WORK(&dw->work, fn);
}

bool
schedule_delayed_work(struct delayed_work *dw, unsigned long ticks)
{
	return queue_delayed_work(system_wq, dw, ticks);
}

bool
queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	bool newly_queued;

	KASSERT(wq != NULL);

	linux_work_lock(&dw->work);
	switch (dw->work.w_state) {
	case WORK_IDLE:
	case WORK_INVOKED:
		if (ticks == 0) {
			/* Skip the delay and queue it now.  */
			dw->work.w_state = WORK_PENDING;
			dw->work.w_wq = wq;
			workqueue_enqueue(wq->wq_workqueue, &dw->work.w_wk,
			    NULL);
		} else {
			callout_init(&dw->dw_callout, CALLOUT_MPSAFE);
			callout_reset(&dw->dw_callout, ticks,
			    &linux_worker_intr, dw);
			dw->work.w_state = WORK_DELAYED;
			dw->work.w_wq = wq;
			TAILQ_INSERT_HEAD(&wq->wq_delayed, dw, dw_entry);
		}
		newly_queued = true;
		break;

	case WORK_DELAYED:
		/*
		 * Timer is already ticking.  Leave it to time out
		 * whenever it was going to time out, as Linux does --
		 * neither speed it up nor postpone it.
		 */
		newly_queued = false;
		break;

	case WORK_PENDING:
		KASSERT(dw->work.w_wq == wq);
		newly_queued = false;
		break;

	case WORK_CANCELLED:
	case WORK_DELAYED_CANCELLED:
		/* XXX Wait for cancellation and then queue?  */
		newly_queued = false;
		break;

	default:
		panic("delayed work %p in bad state: %d", dw,
		    (int)dw->work.w_state);
		break;
	}
	linux_work_unlock(&dw->work);

	return newly_queued;
}

bool
mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	bool timer_modified;

	KASSERT(wq != NULL);

	linux_work_lock(&dw->work);
	switch (dw->work.w_state) {
	case WORK_IDLE:
	case WORK_INVOKED:
		if (ticks == 0) {
			/* Skip the delay and queue it now.  */
			dw->work.w_state = WORK_PENDING;
			dw->work.w_wq = wq;
			workqueue_enqueue(wq->wq_workqueue, &dw->work.w_wk,
			    NULL);
		} else {
			callout_init(&dw->dw_callout, CALLOUT_MPSAFE);
			callout_reset(&dw->dw_callout, ticks,
			    &linux_worker_intr, dw);
			dw->work.w_state = WORK_DELAYED;
			dw->work.w_wq = wq;
			TAILQ_INSERT_HEAD(&wq->wq_delayed, dw, dw_entry);
		}
		timer_modified = false;
		break;

	case WORK_DELAYED:
		/*
		 * Timer is already ticking.  Reschedule it.
		 */
		callout_schedule(&dw->dw_callout, ticks);
		timer_modified = true;
		break;

	case WORK_PENDING:
		KASSERT(dw->work.w_wq == wq);
		timer_modified = false;
		break;

	case WORK_CANCELLED:
	case WORK_DELAYED_CANCELLED:
		/* XXX Wait for cancellation and then queue?  */
		timer_modified = false;
		break;

	default:
		panic("delayed work %p in bad state: %d", dw,
		    (int)dw->work.w_state);
		break;
	}
	linux_work_unlock(&dw->work);

	return timer_modified;
}

bool
cancel_delayed_work(struct delayed_work *dw)
{
	bool cancelled_p = false;

	linux_work_lock(&dw->work);
	switch (dw->work.w_state) {
	case WORK_IDLE:		/* Nothing to do.  */
		break;

	case WORK_DELAYED:
		dw->work.w_state = WORK_DELAYED_CANCELLED;
		linux_cancel_delayed_work_callout(dw, false);
		cancelled_p = true;
		break;

	case WORK_PENDING:
		dw->work.w_state = WORK_CANCELLED;
		cancelled_p = true;
		break;

	case WORK_INVOKED:	/* Don't wait!  */
		break;

	case WORK_CANCELLED:	/* Already done.  */
	case WORK_DELAYED_CANCELLED:
		break;

	default:
		panic("delayed work %p in bad state: %d", dw,
		    (int)dw->work.w_state);
		break;
	}
	linux_work_unlock(&dw->work);

	return cancelled_p;
}

bool
cancel_delayed_work_sync(struct delayed_work *dw)
{
	bool cancelled_p = false;

	linux_work_lock(&dw->work);
	switch (dw->work.w_state) {
	case WORK_IDLE:		/* Nothing to do.  */
		break;

	case WORK_DELAYED:
		dw->work.w_state = WORK_DELAYED_CANCELLED;
		linux_cancel_delayed_work_callout(dw, true);
		cancelled_p = true;
		break;

	case WORK_PENDING:
		dw->work.w_state = WORK_CANCELLED;
		linux_wait_for_cancelled_work(&dw->work);
		cancelled_p = true;
		break;

	case WORK_INVOKED:
		linux_wait_for_invoked_work(&dw->work);
		break;

	case WORK_CANCELLED:	/* Already done.  */
		break;

	case WORK_DELAYED_CANCELLED:
		linux_wait_for_delayed_cancelled_work(dw);
		break;

	default:
		panic("delayed work %p in bad state: %d", dw,
		    (int)dw->work.w_state);
		break;
	}
	linux_work_unlock(&dw->work);

	return cancelled_p;
}

static void
linux_cancel_delayed_work_callout(struct delayed_work *dw, bool wait)
{
	bool fired_p;

	KASSERT(linux_work_locked(&dw->work));
	KASSERT(dw->work.w_state == WORK_DELAYED_CANCELLED);

	if (wait) {
		/*
		 * We unlock, halt, and then relock, rather than
		 * passing an interlock to callout_halt, for two
		 * reasons:
		 *
		 * (1) The work lock is not a mutex(9), so we can't use it.
		 * (2) The WORK_DELAYED_CANCELLED state serves as an interlock.
		 */
		linux_work_unlock(&dw->work);
		fired_p = callout_halt(&dw->dw_callout, NULL);
		linux_work_lock(&dw->work);
	} else {
		fired_p = callout_stop(&dw->dw_callout);
	}

	/*
	 * fired_p means we didn't cancel the callout, so it must have
	 * already begun and will clean up after itself.
	 *
	 * !fired_p means we cancelled it so we have to clean up after
	 * it.  Nobody else should have changed the state in that case.
	 */
	if (!fired_p) {
		struct workqueue_struct *wq;

		KASSERT(linux_work_locked(&dw->work));
		KASSERT(dw->work.w_state == WORK_DELAYED_CANCELLED);

		wq = dw->work.w_wq;
		mutex_enter(&wq->wq_lock);
		TAILQ_REMOVE(&wq->wq_delayed, dw, dw_entry);
		callout_destroy(&dw->dw_callout);
		dw->work.w_state = WORK_IDLE;
		dw->work.w_wq = NULL;
		cv_broadcast(&wq->wq_cv);
		mutex_exit(&wq->wq_lock);
	}
}

static void
linux_wait_for_delayed_cancelled_work(struct delayed_work *dw)
{
	struct workqueue_struct *wq;

	KASSERT(linux_work_locked(&dw->work));
	KASSERT(dw->work.w_state == WORK_DELAYED_CANCELLED);

	wq = dw->work.w_wq;
	do {
		mutex_enter(&wq->wq_lock);
		linux_work_unlock(&dw->work);
		cv_wait(&wq->wq_cv, &wq->wq_lock);
		mutex_exit(&wq->wq_lock);
		linux_work_lock(&dw->work);
	} while ((dw->work.w_state == WORK_DELAYED_CANCELLED) &&
	    (dw->work.w_wq == wq));
}

static void
linux_worker_intr(void *arg)
{
	struct delayed_work *dw = arg;
	struct workqueue_struct *wq;

	linux_work_lock(&dw->work);

	KASSERT((dw->work.w_state == WORK_DELAYED) ||
	    (dw->work.w_state == WORK_DELAYED_CANCELLED));

	wq = dw->work.w_wq;
	mutex_enter(&wq->wq_lock);

	/* Queue the work, or return it to idle and alert any cancellers.  */
	if (__predict_true(dw->work.w_state == WORK_DELAYED)) {
		dw->work.w_state = WORK_PENDING;
		workqueue_enqueue(dw->work.w_wq->wq_workqueue, &dw->work.w_wk,
		    NULL);
	} else {
		KASSERT(dw->work.w_state == WORK_DELAYED_CANCELLED);
		dw->work.w_state = WORK_IDLE;
		dw->work.w_wq = NULL;
		cv_broadcast(&wq->wq_cv);
	}

	/* Either way, the callout is done.  */
	TAILQ_REMOVE(&dw->work.w_wq->wq_delayed, dw, dw_entry);
	callout_destroy(&dw->dw_callout);

	mutex_exit(&wq->wq_lock);
	linux_work_unlock(&dw->work);
}
