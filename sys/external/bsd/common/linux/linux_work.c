/*	$NetBSD: linux_work.c,v 1.61 2022/04/09 23:43:31 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_work.c,v 1.61 2022/04/09 23:43:31 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/queue.h>
#include <sys/sdt.h>

#include <linux/workqueue.h>

TAILQ_HEAD(work_head, work_struct);
TAILQ_HEAD(dwork_head, delayed_work);

struct workqueue_struct {
	kmutex_t		wq_lock;
	kcondvar_t		wq_cv;
	struct dwork_head	wq_delayed; /* delayed work scheduled */
	struct work_head	wq_rcu;	    /* RCU work scheduled */
	struct work_head	wq_queue;   /* work to run */
	struct work_head	wq_dqueue;  /* delayed work to run now */
	struct work_struct	*wq_current_work;
	int			wq_flags;
	bool			wq_dying;
	uint64_t		wq_gen;
	struct lwp		*wq_lwp;
	const char		*wq_name;
};

static void __dead	linux_workqueue_thread(void *);
static void		linux_workqueue_timeout(void *);
static bool		work_claimed(struct work_struct *,
			    struct workqueue_struct *);
static struct workqueue_struct *
			work_queue(struct work_struct *);
static bool		acquire_work(struct work_struct *,
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

SDT_PROBE_DEFINE2(sdt, linux, work, acquire,
    "struct work_struct *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, release,
    "struct work_struct *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, queue,
    "struct work_struct *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, rcu,
    "struct rcu_work *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, cancel,
    "struct work_struct *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE3(sdt, linux, work, schedule,
    "struct delayed_work *"/*dw*/, "struct workqueue_struct *"/*wq*/,
    "unsigned long"/*ticks*/);
SDT_PROBE_DEFINE2(sdt, linux, work, timer,
    "struct delayed_work *"/*dw*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, wait__start,
    "struct delayed_work *"/*dw*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, wait__done,
    "struct delayed_work *"/*dw*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, run,
    "struct work_struct *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE2(sdt, linux, work, done,
    "struct work_struct *"/*work*/, "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE1(sdt, linux, work, batch__start,
    "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE1(sdt, linux, work, batch__done,
    "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE1(sdt, linux, work, flush__self,
    "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE1(sdt, linux, work, flush__start,
    "struct workqueue_struct *"/*wq*/);
SDT_PROBE_DEFINE1(sdt, linux, work, flush__done,
    "struct workqueue_struct *"/*wq*/);

static specificdata_key_t workqueue_key __read_mostly;

struct workqueue_struct	*system_highpri_wq __read_mostly;
struct workqueue_struct	*system_long_wq __read_mostly;
struct workqueue_struct	*system_power_efficient_wq __read_mostly;
struct workqueue_struct	*system_unbound_wq __read_mostly;
struct workqueue_struct	*system_wq __read_mostly;

static inline uintptr_t
atomic_cas_uintptr(volatile uintptr_t *p, uintptr_t old, uintptr_t new)
{

	return (uintptr_t)atomic_cas_ptr(p, (void *)old, (void *)new);
}

/*
 * linux_workqueue_init()
 *
 *	Initialize the Linux workqueue subsystem.  Return 0 on success,
 *	NetBSD error on failure.
 */
static int
linux_workqueue_init0(void)
{
	int error;

	error = lwp_specific_key_create(&workqueue_key, NULL);
	if (error)
		goto out;

	system_highpri_wq = alloc_ordered_workqueue("lnxhipwq", 0);
	if (system_highpri_wq == NULL) {
		error = ENOMEM;
		goto out;
	}

	system_long_wq = alloc_ordered_workqueue("lnxlngwq", 0);
	if (system_long_wq == NULL) {
		error = ENOMEM;
		goto out;
	}

	system_power_efficient_wq = alloc_ordered_workqueue("lnxpwrwq", 0);
	if (system_power_efficient_wq == NULL) {
		error = ENOMEM;
		goto out;
	}

	system_unbound_wq = alloc_ordered_workqueue("lnxubdwq", 0);
	if (system_unbound_wq == NULL) {
		error = ENOMEM;
		goto out;
	}

	system_wq = alloc_ordered_workqueue("lnxsyswq", 0);
	if (system_wq == NULL) {
		error = ENOMEM;
		goto out;
	}

	/* Success!  */
	error = 0;

out:	if (error) {
		if (system_highpri_wq)
			destroy_workqueue(system_highpri_wq);
		if (system_long_wq)
			destroy_workqueue(system_long_wq);
		if (system_power_efficient_wq)
			destroy_workqueue(system_power_efficient_wq);
		if (system_unbound_wq)
			destroy_workqueue(system_unbound_wq);
		if (system_wq)
			destroy_workqueue(system_wq);
		if (workqueue_key)
			lwp_specific_key_delete(workqueue_key);
	}

	return error;
}

/*
 * linux_workqueue_fini()
 *
 *	Destroy the Linux workqueue subsystem.  Never fails.
 */
static void
linux_workqueue_fini0(void)
{

	destroy_workqueue(system_power_efficient_wq);
	destroy_workqueue(system_long_wq);
	destroy_workqueue(system_wq);
	lwp_specific_key_delete(workqueue_key);
}

#ifndef _MODULE
static ONCE_DECL(linux_workqueue_init_once);
#endif

int
linux_workqueue_init(void)
{
#ifdef _MODULE
	return linux_workqueue_init0();
#else
	return INIT_ONCE(&linux_workqueue_init_once, &linux_workqueue_init0);
#endif
}

void
linux_workqueue_fini(void)
{
#ifdef _MODULE
	return linux_workqueue_fini0();
#else
	return FINI_ONCE(&linux_workqueue_init_once, &linux_workqueue_fini0);
#endif
}

/*
 * Workqueues
 */

/*
 * alloc_workqueue(name, flags, max_active)
 *
 *	Create a workqueue of the given name.  max_active is the
 *	maximum number of work items in flight, or 0 for the default.
 *	Return NULL on failure, pointer to struct workqueue_struct
 *	object on success.
 */
struct workqueue_struct *
alloc_workqueue(const char *name, int flags, unsigned max_active)
{
	struct workqueue_struct *wq;
	int error;

	KASSERT(max_active == 0 || max_active == 1);

	wq = kmem_zalloc(sizeof(*wq), KM_SLEEP);

	mutex_init(&wq->wq_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&wq->wq_cv, name);
	TAILQ_INIT(&wq->wq_delayed);
	TAILQ_INIT(&wq->wq_rcu);
	TAILQ_INIT(&wq->wq_queue);
	TAILQ_INIT(&wq->wq_dqueue);
	wq->wq_current_work = NULL;
	wq->wq_flags = 0;
	wq->wq_dying = false;
	wq->wq_gen = 0;
	wq->wq_lwp = NULL;
	wq->wq_name = name;

	error = kthread_create(PRI_NONE,
	    KTHREAD_MPSAFE|KTHREAD_TS|KTHREAD_MUSTJOIN, NULL,
	    &linux_workqueue_thread, wq, &wq->wq_lwp, "%s", name);
	if (error)
		goto fail0;

	return wq;

fail0:	KASSERT(TAILQ_EMPTY(&wq->wq_dqueue));
	KASSERT(TAILQ_EMPTY(&wq->wq_queue));
	KASSERT(TAILQ_EMPTY(&wq->wq_rcu));
	KASSERT(TAILQ_EMPTY(&wq->wq_delayed));
	cv_destroy(&wq->wq_cv);
	mutex_destroy(&wq->wq_lock);
	kmem_free(wq, sizeof(*wq));
	return NULL;
}

/*
 * alloc_ordered_workqueue(name, flags)
 *
 *	Same as alloc_workqueue(name, flags, 1).
 */
struct workqueue_struct *
alloc_ordered_workqueue(const char *name, int flags)
{

	return alloc_workqueue(name, flags, 1);
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

		KASSERT(work_queue(&dw->work) == wq);
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
		SDT_PROBE2(sdt, linux, work, cancel,  &dw->work, wq);
		dw->dw_state = DELAYED_WORK_CANCELLED;
		if (!callout_halt(&dw->dw_callout, &wq->wq_lock))
			cancel_delayed_work_done(wq, dw);
	}
	mutex_exit(&wq->wq_lock);

	/* Wait for all scheduled RCU work to complete.  */
	mutex_enter(&wq->wq_lock);
	while (!TAILQ_EMPTY(&wq->wq_rcu))
		cv_wait(&wq->wq_cv, &wq->wq_lock);
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
	KASSERT(wq->wq_flags == 0);
	KASSERT(wq->wq_current_work == NULL);
	KASSERT(TAILQ_EMPTY(&wq->wq_dqueue));
	KASSERT(TAILQ_EMPTY(&wq->wq_queue));
	KASSERT(TAILQ_EMPTY(&wq->wq_rcu));
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
	struct work_head *const q[2] = { &wq->wq_queue, &wq->wq_dqueue };
	struct work_struct marker, *work;
	unsigned i;

	lwp_setspecific(workqueue_key, wq);

	mutex_enter(&wq->wq_lock);
	for (;;) {
		/*
		 * Wait until there's activity.  If there's no work and
		 * we're dying, stop here.
		 */
		if (TAILQ_EMPTY(&wq->wq_queue) &&
		    TAILQ_EMPTY(&wq->wq_dqueue)) {
			if (wq->wq_dying)
				break;
			cv_wait(&wq->wq_cv, &wq->wq_lock);
			continue;
		}

		/*
		 * Start a batch of work.  Use a marker to delimit when
		 * the batch ends so we can advance the generation
		 * after the batch.
		 */
		SDT_PROBE1(sdt, linux, work, batch__start,  wq);
		for (i = 0; i < 2; i++) {
			if (TAILQ_EMPTY(q[i]))
				continue;
			TAILQ_INSERT_TAIL(q[i], &marker, work_entry);
			while ((work = TAILQ_FIRST(q[i])) != &marker) {
				void (*func)(struct work_struct *);

				KASSERT(work_queue(work) == wq);
				KASSERT(work_claimed(work, wq));
				KASSERTMSG((q[i] != &wq->wq_dqueue ||
					container_of(work, struct delayed_work,
					    work)->dw_state ==
					DELAYED_WORK_IDLE),
				    "delayed work %p queued and scheduled",
				    work);

				TAILQ_REMOVE(q[i], work, work_entry);
				KASSERT(wq->wq_current_work == NULL);
				wq->wq_current_work = work;
				func = work->func;
				release_work(work, wq);
				/* Can't dereference work after this point.  */

				mutex_exit(&wq->wq_lock);
				SDT_PROBE2(sdt, linux, work, run,  work, wq);
				(*func)(work);
				SDT_PROBE2(sdt, linux, work, done,  work, wq);
				mutex_enter(&wq->wq_lock);

				KASSERT(wq->wq_current_work == work);
				wq->wq_current_work = NULL;
				cv_broadcast(&wq->wq_cv);
			}
			TAILQ_REMOVE(q[i], &marker, work_entry);
		}

		/* Notify cancel that we've completed a batch of work.  */
		wq->wq_gen++;
		cv_broadcast(&wq->wq_cv);
		SDT_PROBE1(sdt, linux, work, batch__done,  wq);
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
	struct workqueue_struct *const wq = work_queue(&dw->work);

	KASSERTMSG(wq != NULL,
	    "delayed work %p state %d resched %d",
	    dw, dw->dw_state, dw->dw_resched);

	SDT_PROBE2(sdt, linux, work, timer,  dw, wq);

	mutex_enter(&wq->wq_lock);
	KASSERT(work_queue(&dw->work) == wq);
	switch (dw->dw_state) {
	case DELAYED_WORK_IDLE:
		panic("delayed work callout uninitialized: %p", dw);
	case DELAYED_WORK_SCHEDULED:
		dw_callout_destroy(wq, dw);
		TAILQ_INSERT_TAIL(&wq->wq_dqueue, &dw->work, work_entry);
		cv_broadcast(&wq->wq_cv);
		SDT_PROBE2(sdt, linux, work, queue,  &dw->work, wq);
		break;
	case DELAYED_WORK_RESCHEDULED:
		KASSERT(dw->dw_resched >= 0);
		callout_schedule(&dw->dw_callout, dw->dw_resched);
		dw->dw_state = DELAYED_WORK_SCHEDULED;
		dw->dw_resched = -1;
		break;
	case DELAYED_WORK_CANCELLED:
		cancel_delayed_work_done(wq, dw);
		/* Can't dereference dw after this point.  */
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

	work->work_owner = 0;
	work->func = fn;
}

/*
 * work_claimed(work, wq)
 *
 *	True if work is currently claimed by a workqueue, meaning it is
 *	either on the queue or scheduled in a callout.  The workqueue
 *	must be wq, and caller must hold wq's lock.
 */
static bool
work_claimed(struct work_struct *work, struct workqueue_struct *wq)
{

	KASSERT(work_queue(work) == wq);
	KASSERT(mutex_owned(&wq->wq_lock));

	return atomic_load_relaxed(&work->work_owner) & 1;
}

/*
 * work_pending(work)
 *
 *	True if work is currently claimed by any workqueue, scheduled
 *	to run on that workqueue.
 */
bool
work_pending(const struct work_struct *work)
{

	return atomic_load_relaxed(&work->work_owner) & 1;
}

/*
 * work_queue(work)
 *
 *	Return the last queue that work was queued on, or NULL if it
 *	was never queued.
 */
static struct workqueue_struct *
work_queue(struct work_struct *work)
{

	return (struct workqueue_struct *)
	    (atomic_load_relaxed(&work->work_owner) & ~(uintptr_t)1);
}

/*
 * acquire_work(work, wq)
 *
 *	Try to claim work for wq.  If work is already claimed, it must
 *	be claimed by wq; return false.  If work is not already
 *	claimed, claim it, issue a memory barrier to match any prior
 *	release_work, and return true.
 *
 *	Caller must hold wq's lock.
 */
static bool
acquire_work(struct work_struct *work, struct workqueue_struct *wq)
{
	uintptr_t owner0, owner;

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(((uintptr_t)wq & 1) == 0);

	owner = (uintptr_t)wq | 1;
	do {
		owner0 = atomic_load_relaxed(&work->work_owner);
		if (owner0 & 1) {
			KASSERT((owner0 & ~(uintptr_t)1) == (uintptr_t)wq);
			return false;
		}
		KASSERT(owner0 == (uintptr_t)NULL || owner0 == (uintptr_t)wq);
	} while (atomic_cas_uintptr(&work->work_owner, owner0, owner) !=
	    owner0);

	KASSERT(work_queue(work) == wq);
	membar_acquire();
	SDT_PROBE2(sdt, linux, work, acquire,  work, wq);
	return true;
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

	KASSERT(work_queue(work) == wq);
	KASSERT(mutex_owned(&wq->wq_lock));

	SDT_PROBE2(sdt, linux, work, release,  work, wq);
	membar_release();

	/*
	 * Non-interlocked r/m/w is safe here because nobody else can
	 * write to this while the claimed bit is set and the workqueue
	 * lock is held.
	 */
	atomic_store_relaxed(&work->work_owner,
	    atomic_load_relaxed(&work->work_owner) & ~(uintptr_t)1);
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
	bool newly_queued;

	KASSERT(wq != NULL);

	mutex_enter(&wq->wq_lock);
	if (__predict_true(acquire_work(work, wq))) {
		/*
		 * It wasn't on any workqueue at all.  Put it on this
		 * one, and signal the worker thread that there is work
		 * to do.
		 */
		TAILQ_INSERT_TAIL(&wq->wq_queue, work, work_entry);
		cv_broadcast(&wq->wq_cv);
		SDT_PROBE2(sdt, linux, work, queue,  work, wq);
		newly_queued = true;
	} else {
		/*
		 * It was already on this workqueue.  Nothing to do
		 * since it is already queued.
		 */
		newly_queued = false;
	}
	mutex_exit(&wq->wq_lock);

	return newly_queued;
}

/*
 * cancel_work(work)
 *
 *	If work was queued, remove it from the queue and return true.
 *	If work was not queued, return false.  Work may still be
 *	running when this returns.
 */
bool
cancel_work(struct work_struct *work)
{
	struct workqueue_struct *wq;
	bool cancelled_p = false;

	/* If there's no workqueue, nothing to cancel.   */
	if ((wq = work_queue(work)) == NULL)
		goto out;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work_queue(work) != wq)) {
		/*
		 * It has finished execution or been cancelled by
		 * another thread, and has been moved off the
		 * workqueue, so it's too to cancel.
		 */
		cancelled_p = false;
	} else {
		/* Check whether it's on the queue.  */
		if (work_claimed(work, wq)) {
			/*
			 * It is still on the queue.  Take it off the
			 * queue and report successful cancellation.
			 */
			TAILQ_REMOVE(&wq->wq_queue, work, work_entry);
			SDT_PROBE2(sdt, linux, work, cancel,  work, wq);
			release_work(work, wq);
			/* Can't dereference work after this point.  */
			cancelled_p = true;
		} else {
			/* Not on the queue.  Couldn't cancel it.  */
			cancelled_p = false;
		}
	}
	mutex_exit(&wq->wq_lock);

out:	return cancelled_p;
}

/*
 * cancel_work_sync(work)
 *
 *	If work was queued, remove it from the queue and return true.
 *	If work was not queued, return false.  Either way, if work is
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
	if ((wq = work_queue(work)) == NULL)
		goto out;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work_queue(work) != wq)) {
		/*
		 * It has finished execution or been cancelled by
		 * another thread, and has been moved off the
		 * workqueue, so it's too late to cancel.
		 */
		cancelled_p = false;
	} else {
		/* Check whether it's on the queue.  */
		if (work_claimed(work, wq)) {
			/*
			 * It is still on the queue.  Take it off the
			 * queue and report successful cancellation.
			 */
			TAILQ_REMOVE(&wq->wq_queue, work, work_entry);
			SDT_PROBE2(sdt, linux, work, cancel,  work, wq);
			release_work(work, wq);
			/* Can't dereference work after this point.  */
			cancelled_p = true;
		} else {
			/* Not on the queue.  Couldn't cancel it.  */
			cancelled_p = false;
		}
		/* If it's still running, wait for it to complete.  */
		if (wq->wq_current_work == work)
			wait_for_current_work(work, wq);
	}
	mutex_exit(&wq->wq_lock);

out:	return cancelled_p;
}

/*
 * wait_for_current_work(work, wq)
 *
 *	wq must be currently executing work.  Wait for it to finish.
 *
 *	Does not dereference work.
 */
static void
wait_for_current_work(struct work_struct *work, struct workqueue_struct *wq)
{
	uint64_t gen;

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(wq->wq_current_work == work);

	/* Wait only one generation in case it gets requeued quickly.  */
	SDT_PROBE2(sdt, linux, work, wait__start,  work, wq);
	gen = wq->wq_gen;
	do {
		cv_wait(&wq->wq_cv, &wq->wq_lock);
	} while (wq->wq_current_work == work && wq->wq_gen == gen);
	SDT_PROBE2(sdt, linux, work, wait__done,  work, wq);
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
	KASSERT(work_queue(&dw->work) == wq);
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
	KASSERT(work_queue(&dw->work) == wq);
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
 *	workqueue.  Caller must not dereference dw after this returns.
 */
static void
cancel_delayed_work_done(struct workqueue_struct *wq, struct delayed_work *dw)
{

	KASSERT(mutex_owned(&wq->wq_lock));
	KASSERT(work_queue(&dw->work) == wq);
	KASSERT(dw->dw_state == DELAYED_WORK_CANCELLED);

	dw_callout_destroy(wq, dw);
	release_work(&dw->work, wq);
	/* Can't dereference dw after this point.  */
}

/*
 * queue_delayed_work(wq, dw, ticks)
 *
 *	If it is not currently scheduled, schedule dw to run after
 *	ticks on wq.  If currently queued, remove it from the queue
 *	first.
 *
 *	If ticks == 0, queue it to run as soon as the worker can,
 *	without waiting for the next callout tick to run.
 */
bool
queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	bool newly_queued;

	mutex_enter(&wq->wq_lock);
	if (__predict_true(acquire_work(&dw->work, wq))) {
		/*
		 * It wasn't on any workqueue at all.  Schedule it to
		 * run on this one.
		 */
		KASSERT(dw->dw_state == DELAYED_WORK_IDLE);
		if (ticks == 0) {
			TAILQ_INSERT_TAIL(&wq->wq_dqueue, &dw->work,
			    work_entry);
			cv_broadcast(&wq->wq_cv);
			SDT_PROBE2(sdt, linux, work, queue,  &dw->work, wq);
		} else {
			/*
			 * Initialize a callout and schedule to run
			 * after a delay.
			 */
			dw_callout_init(wq, dw);
			callout_schedule(&dw->dw_callout, MIN(INT_MAX, ticks));
			SDT_PROBE3(sdt, linux, work, schedule,  dw, wq, ticks);
		}
		newly_queued = true;
	} else {
		/* It was already on this workqueue.  */
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
		case DELAYED_WORK_SCHEDULED:
		case DELAYED_WORK_RESCHEDULED:
			/* On the queue or already scheduled.  Leave it.  */
			newly_queued = false;
			break;
		case DELAYED_WORK_CANCELLED:
			/*
			 * Scheduled and the callout began, but it was
			 * cancelled.  Reschedule it.
			 */
			if (ticks == 0) {
				dw->dw_state = DELAYED_WORK_SCHEDULED;
				SDT_PROBE2(sdt, linux, work, queue,
				    &dw->work, wq);
			} else {
				dw->dw_state = DELAYED_WORK_RESCHEDULED;
				dw->dw_resched = MIN(INT_MAX, ticks);
				SDT_PROBE3(sdt, linux, work, schedule,
				    dw, wq, ticks);
			}
			newly_queued = true;
			break;
		default:
			panic("invalid delayed work state: %d",
			    dw->dw_state);
		}
	}
	mutex_exit(&wq->wq_lock);

	return newly_queued;
}

/*
 * mod_delayed_work(wq, dw, ticks)
 *
 *	Schedule dw to run after ticks.  If scheduled or queued,
 *	reschedule.  If ticks == 0, run without delay.
 *
 *	True if it modified the timer of an already scheduled work,
 *	false if it newly scheduled the work.
 */
bool
mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dw,
    unsigned long ticks)
{
	bool timer_modified;

	mutex_enter(&wq->wq_lock);
	if (acquire_work(&dw->work, wq)) {
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
			TAILQ_INSERT_TAIL(&wq->wq_dqueue, &dw->work,
			    work_entry);
			cv_broadcast(&wq->wq_cv);
			SDT_PROBE2(sdt, linux, work, queue,  &dw->work, wq);
		} else {
			/*
			 * Initialize a callout and schedule to run
			 * after a delay.
			 */
			dw_callout_init(wq, dw);
			callout_schedule(&dw->dw_callout, MIN(INT_MAX, ticks));
			SDT_PROBE3(sdt, linux, work, schedule,  dw, wq, ticks);
		}
		timer_modified = false;
	} else {
		/* It was already on this workqueue.  */
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/* On the queue.  */
			if (ticks == 0) {
				/* Leave it be.  */
				SDT_PROBE2(sdt, linux, work, cancel,
				    &dw->work, wq);
				SDT_PROBE2(sdt, linux, work, queue,
				    &dw->work, wq);
			} else {
				/* Remove from the queue and schedule.  */
				TAILQ_REMOVE(&wq->wq_dqueue, &dw->work,
				    work_entry);
				dw_callout_init(wq, dw);
				callout_schedule(&dw->dw_callout,
				    MIN(INT_MAX, ticks));
				SDT_PROBE2(sdt, linux, work, cancel,
				    &dw->work, wq);
				SDT_PROBE3(sdt, linux, work, schedule,
				    dw, wq, ticks);
			}
			timer_modified = true;
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
					SDT_PROBE2(sdt, linux, work, cancel,
					    &dw->work, wq);
					SDT_PROBE2(sdt, linux, work, queue,
					    &dw->work, wq);
				} else {
					/* Ask the callout to reschedule.  */
					dw->dw_state = DELAYED_WORK_RESCHEDULED;
					dw->dw_resched = MIN(INT_MAX, ticks);
					SDT_PROBE2(sdt, linux, work, cancel,
					    &dw->work, wq);
					SDT_PROBE3(sdt, linux, work, schedule,
					    dw, wq, ticks);
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
					TAILQ_INSERT_TAIL(&wq->wq_dqueue,
					    &dw->work, work_entry);
					cv_broadcast(&wq->wq_cv);
					SDT_PROBE2(sdt, linux, work, cancel,
					    &dw->work, wq);
					SDT_PROBE2(sdt, linux, work, queue,
					    &dw->work, wq);
				} else {
					/*
					 * Reschedule the callout.  No
					 * state change.
					 */
					callout_schedule(&dw->dw_callout,
					    MIN(INT_MAX, ticks));
					SDT_PROBE2(sdt, linux, work, cancel,
					    &dw->work, wq);
					SDT_PROBE3(sdt, linux, work, schedule,
					    dw, wq, ticks);
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
				SDT_PROBE2(sdt, linux, work, cancel,
				    &dw->work, wq);
				SDT_PROBE2(sdt, linux, work, queue,
				    &dw->work, wq);
			} else {
				/* Change the rescheduled time.  */
				dw->dw_resched = ticks;
				SDT_PROBE2(sdt, linux, work, cancel,
				    &dw->work, wq);
				SDT_PROBE3(sdt, linux, work, schedule,
				    dw, wq, ticks);
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
				SDT_PROBE2(sdt, linux, work, queue,
				    &dw->work, wq);
			} else {
				/* Ask it to reschedule.  */
				dw->dw_state = DELAYED_WORK_RESCHEDULED;
				dw->dw_resched = MIN(INT_MAX, ticks);
				SDT_PROBE3(sdt, linux, work, schedule,
				    dw, wq, ticks);
			}
			timer_modified = false;
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
	if ((wq = work_queue(&dw->work)) == NULL)
		return false;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work_queue(&dw->work) != wq)) {
		cancelled_p = false;
	} else {
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It is either on the queue or already running
			 * or both.
			 */
			if (work_claimed(&dw->work, wq)) {
				/* On the queue.  Remove and release.  */
				TAILQ_REMOVE(&wq->wq_dqueue, &dw->work,
				    work_entry);
				SDT_PROBE2(sdt, linux, work, cancel,
				    &dw->work, wq);
				release_work(&dw->work, wq);
				/* Can't dereference dw after this point.  */
				cancelled_p = true;
			} else {
				/* Not on the queue, so didn't cancel.  */
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
			SDT_PROBE2(sdt, linux, work, cancel,  &dw->work, wq);
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
			SDT_PROBE2(sdt, linux, work, cancel,  &dw->work, wq);
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
	if ((wq = work_queue(&dw->work)) == NULL)
		return false;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work_queue(&dw->work) != wq)) {
		cancelled_p = false;
	} else {
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It is either on the queue or already running
			 * or both.
			 */
			if (work_claimed(&dw->work, wq)) {
				/* On the queue.  Remove and release.  */
				TAILQ_REMOVE(&wq->wq_dqueue, &dw->work,
				    work_entry);
				SDT_PROBE2(sdt, linux, work, cancel,
				    &dw->work, wq);
				release_work(&dw->work, wq);
				/* Can't dereference dw after this point.  */
				cancelled_p = true;
			} else {
				/* Not on the queue, so didn't cancel. */
				cancelled_p = false;
			}
			/* If it's still running, wait for it to complete.  */
			if (wq->wq_current_work == &dw->work)
				wait_for_current_work(&dw->work, wq);
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
			SDT_PROBE2(sdt, linux, work, cancel,  &dw->work, wq);
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
			SDT_PROBE2(sdt, linux, work, cancel,  &dw->work, wq);
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

struct flush_work {
	kmutex_t		fw_lock;
	kcondvar_t		fw_cv;
	struct work_struct	fw_work;
	bool			fw_done;
};

static void
flush_work_cb(struct work_struct *work)
{
	struct flush_work *fw = container_of(work, struct flush_work, fw_work);

	mutex_enter(&fw->fw_lock);
	fw->fw_done = true;
	cv_broadcast(&fw->fw_cv);
	mutex_exit(&fw->fw_lock);
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
	struct flush_work fw;

	if (lwp_getspecific(workqueue_key) == wq) {
		SDT_PROBE1(sdt, linux, work, flush__self,  wq);
		return;
	}

	mutex_init(&fw.fw_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&fw.fw_cv, "lxwqflsh");
	INIT_WORK(&fw.fw_work, &flush_work_cb);
	fw.fw_done = false;

	SDT_PROBE1(sdt, linux, work, flush__start,  wq);
	queue_work(wq, &fw.fw_work);

	mutex_enter(&fw.fw_lock);
	while (!fw.fw_done)
		cv_wait(&fw.fw_cv, &fw.fw_lock);
	mutex_exit(&fw.fw_lock);
	SDT_PROBE1(sdt, linux, work, flush__done,  wq);

	KASSERT(fw.fw_done);
	/* no DESTROY_WORK */
	cv_destroy(&fw.fw_cv);
	mutex_destroy(&fw.fw_lock);
}

/*
 * drain_workqueue(wq)
 *
 *	Repeatedly flush wq until there is no more work.
 */
void
drain_workqueue(struct workqueue_struct *wq)
{
	unsigned ntries = 0;
	bool done;

	do {
		if (ntries++ == 10 || (ntries % 100) == 0)
			printf("linux workqueue %s"
			    ": still clogged after %u flushes",
			    wq->wq_name, ntries);
		flush_workqueue(wq);
		mutex_enter(&wq->wq_lock);
		done = wq->wq_current_work == NULL;
		done &= TAILQ_EMPTY(&wq->wq_queue);
		done &= TAILQ_EMPTY(&wq->wq_dqueue);
		mutex_exit(&wq->wq_lock);
	} while (!done);
}

/*
 * flush_work(work)
 *
 *	If work is queued or currently executing, wait for it to
 *	complete.
 *
 *	Return true if we waited to flush it, false if it was already
 *	idle.
 */
bool
flush_work(struct work_struct *work)
{
	struct workqueue_struct *wq;

	/* If there's no workqueue, nothing to flush.  */
	if ((wq = work_queue(work)) == NULL)
		return false;

	flush_workqueue(wq);
	return true;
}

/*
 * flush_delayed_work(dw)
 *
 *	If dw is scheduled to run after a delay, queue it immediately
 *	instead.  Then, if dw is queued or currently executing, wait
 *	for it to complete.
 */
bool
flush_delayed_work(struct delayed_work *dw)
{
	struct workqueue_struct *wq;
	bool waited = false;

	/* If there's no workqueue, nothing to flush.  */
	if ((wq = work_queue(&dw->work)) == NULL)
		return false;

	mutex_enter(&wq->wq_lock);
	if (__predict_false(work_queue(&dw->work) != wq)) {
		/*
		 * Moved off the queue already (and possibly to another
		 * queue, though that would be ill-advised), so it must
		 * have completed, and we have nothing more to do.
		 */
		waited = false;
	} else {
		switch (dw->dw_state) {
		case DELAYED_WORK_IDLE:
			/*
			 * It has a workqueue assigned and the callout
			 * is idle, so it must be in progress or on the
			 * queue.  In that case, we'll wait for it to
			 * complete.
			 */
			break;
		case DELAYED_WORK_SCHEDULED:
		case DELAYED_WORK_RESCHEDULED:
		case DELAYED_WORK_CANCELLED:
			/*
			 * The callout is scheduled, and may have even
			 * started.  Mark it as scheduled so that if
			 * the callout has fired it will queue the work
			 * itself.  Try to stop the callout -- if we
			 * can, queue the work now; if we can't, wait
			 * for the callout to complete, which entails
			 * queueing it.
			 */
			dw->dw_state = DELAYED_WORK_SCHEDULED;
			if (!callout_halt(&dw->dw_callout, &wq->wq_lock)) {
				/*
				 * We stopped it before it ran.  No
				 * state change in the interim is
				 * possible.  Destroy the callout and
				 * queue it ourselves.
				 */
				KASSERT(dw->dw_state ==
				    DELAYED_WORK_SCHEDULED);
				dw_callout_destroy(wq, dw);
				TAILQ_INSERT_TAIL(&wq->wq_dqueue, &dw->work,
				    work_entry);
				cv_broadcast(&wq->wq_cv);
				SDT_PROBE2(sdt, linux, work, queue,
				    &dw->work, wq);
			}
			break;
		default:
			panic("invalid delayed work state: %d", dw->dw_state);
		}
		/*
		 * Waiting for the whole queue to flush is overkill,
		 * but doesn't hurt.
		 */
		mutex_exit(&wq->wq_lock);
		flush_workqueue(wq);
		mutex_enter(&wq->wq_lock);
		waited = true;
	}
	mutex_exit(&wq->wq_lock);

	return waited;
}

/*
 * delayed_work_pending(dw)
 *
 *	True if dw is currently scheduled to execute, false if not.
 */
bool
delayed_work_pending(const struct delayed_work *dw)
{

	return work_pending(&dw->work);
}

/*
 * INIT_RCU_WORK(rw, fn)
 *
 *	Initialize rw for use with a workqueue to call fn in a worker
 *	thread after an RCU grace period.  There is no corresponding
 *	destruction operation.
 */
void
INIT_RCU_WORK(struct rcu_work *rw, void (*fn)(struct work_struct *))
{

	INIT_WORK(&rw->work, fn);
}

static void
queue_rcu_work_cb(struct rcu_head *r)
{
	struct rcu_work *rw = container_of(r, struct rcu_work, rw_rcu);
	struct workqueue_struct *wq = work_queue(&rw->work);

	mutex_enter(&wq->wq_lock);
	KASSERT(work_pending(&rw->work));
	KASSERT(work_queue(&rw->work) == wq);
	destroy_rcu_head(&rw->rw_rcu);
	TAILQ_REMOVE(&wq->wq_rcu, &rw->work, work_entry);
	TAILQ_INSERT_TAIL(&wq->wq_queue, &rw->work, work_entry);
	cv_broadcast(&wq->wq_cv);
	SDT_PROBE2(sdt, linux, work, queue,  &rw->work, wq);
	mutex_exit(&wq->wq_lock);
}

/*
 * queue_rcu_work(wq, rw)
 *
 *	Schedule rw to run on wq after an RCU grace period.
 */
void
queue_rcu_work(struct workqueue_struct *wq, struct rcu_work *rw)
{

	mutex_enter(&wq->wq_lock);
	if (acquire_work(&rw->work, wq)) {
		init_rcu_head(&rw->rw_rcu);
		SDT_PROBE2(sdt, linux, work, rcu,  rw, wq);
		TAILQ_INSERT_TAIL(&wq->wq_rcu, &rw->work, work_entry);
		call_rcu(&rw->rw_rcu, &queue_rcu_work_cb);
	}
	mutex_exit(&wq->wq_lock);
}
