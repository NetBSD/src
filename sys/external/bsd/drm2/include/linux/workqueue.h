/*	$NetBSD: workqueue.h,v 1.1.2.10 2013/09/08 16:40:36 riastradh Exp $	*/

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

#ifndef _LINUX_WORKQUEUE_H_
#define _LINUX_WORKQUEUE_H_

#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/workqueue.h>

#include <asm/bug.h>
#include <linux/kernel.h>

/*
 * XXX This implementation is a load of bollocks -- callouts and
 * workqueues are expedient, but wrong, if for no reason other than
 * that there is no destroy operation.
 *
 * XXX The amount of code in here is absurd; it should be given a
 * proper source file.
 */

struct work_struct {
	void 			(*w_fn)(struct work_struct *);
	struct workqueue	*w_wq;
	struct work		w_wk;
	kmutex_t		w_lock;
	kcondvar_t		w_cv;
	enum {
		WORK_IDLE,
		WORK_QUEUED,
		WORK_CANCELLED,
		WORK_INFLIGHT,
		WORK_REQUEUED,
	}			w_state;
};

static void __unused
linux_work_fn(struct work *wk __unused, void *arg)
{
	struct work_struct *const work = arg;

	mutex_spin_enter(&work->w_lock);
	switch (work->w_state) {
	case WORK_IDLE:
		panic("work ran while idle: %p", work);
		break;

	case WORK_QUEUED:
		work->w_state = WORK_INFLIGHT;
		mutex_spin_exit(&work->w_lock);
		(*work->w_fn)(work);
		mutex_spin_enter(&work->w_lock);
		switch (work->w_state) {
		case WORK_IDLE:
		case WORK_QUEUED:
			panic("work hosed while in flight: %p", work);
			break;

		case WORK_INFLIGHT:
		case WORK_CANCELLED:
			work->w_state = WORK_IDLE;
			cv_broadcast(&work->w_cv);
			break;

		case WORK_REQUEUED:
			workqueue_enqueue(work->w_wq, &work->w_wk, NULL);
			work->w_state = WORK_QUEUED;
			break;

		default:
			panic("work %p in bad state: %d", work,
			    (int)work->w_state);
			break;
		}
		break;

	case WORK_CANCELLED:
		work->w_state = WORK_IDLE;
		cv_broadcast(&work->w_cv);
		break;

	case WORK_INFLIGHT:
		panic("work already in flight: %p", work);
		break;

	case WORK_REQUEUED:
		panic("work requeued while not in flight: %p", work);
		break;

	default:
		panic("work %p in bad state: %d", work, (int)work->w_state);
		break;
	}
	mutex_spin_exit(&work->w_lock);
}

static inline void
INIT_WORK(struct work_struct *work, void (*fn)(struct work_struct *))
{
	int error;

	work->w_fn = fn;
	error = workqueue_create(&work->w_wq, "lnxworkq", &linux_work_fn,
	    work, PRI_NONE, IPL_VM, WQ_MPSAFE);
	if (error)
		panic("workqueue creation failed: %d", error); /* XXX */

	mutex_init(&work->w_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&work->w_cv, "linxwork");
	work->w_state = WORK_IDLE;
}

static inline void
schedule_work(struct work_struct *work)
{

	mutex_spin_enter(&work->w_lock);
	switch (work->w_state) {
	case WORK_IDLE:
		workqueue_enqueue(work->w_wq, &work->w_wk, NULL);
		work->w_state = WORK_QUEUED;
		break;

	case WORK_CANCELLED:
		break;

	case WORK_INFLIGHT:
		work->w_state = WORK_REQUEUED;
		break;

	case WORK_QUEUED:
	case WORK_REQUEUED:
		break;

	default:
		panic("work %p in bad state: %d", work, (int)work->w_state);
		break;
	}
	mutex_spin_exit(&work->w_lock);
}

/*
 * XXX This API can't possibly be right because there is no interlock.
 */
static inline bool
cancel_work_sync(struct work_struct *work)
{
	bool was_pending = false;

	mutex_spin_enter(&work->w_lock);
retry:	switch (work->w_state) {
	case WORK_IDLE:
		break;

	case WORK_QUEUED:
	case WORK_INFLIGHT:
	case WORK_REQUEUED:
		work->w_state = WORK_CANCELLED;
		/* FALLTHROUGH */
	case WORK_CANCELLED:
		cv_wait(&work->w_cv, &work->w_lock);
		was_pending = true;
		goto retry;

	default:
		panic("work %p in bad state: %d", work, (int)work->w_state);
	}
	mutex_spin_exit(&work->w_lock);

	return was_pending;
}

struct delayed_work {
	struct callout		dw_callout;
	struct work_struct	work; /* not dw_work; name must match Linux */
};

static void __unused
linux_delayed_work_fn(void *arg)
{
	struct delayed_work *const dw = arg;

	schedule_work(&dw->work);
}

static inline void
INIT_DELAYED_WORK(struct delayed_work *dw, void (*fn)(struct work_struct *))
{
	callout_init(&dw->dw_callout, CALLOUT_MPSAFE);
	callout_setfunc(&dw->dw_callout, linux_delayed_work_fn, dw);
	INIT_WORK(&dw->work, fn);
}

static inline struct delayed_work *
to_delayed_work(struct work_struct *work)
{
	return container_of(work, struct delayed_work, work);
}

static inline void
schedule_delayed_work(struct delayed_work *dw, unsigned long ticks)
{
	KASSERT(ticks < INT_MAX);
	callout_schedule(&dw->dw_callout, (int)ticks);
}

static inline bool
cancel_delayed_work(struct delayed_work *dw)
{
	return !callout_stop(&dw->dw_callout);
}

static inline bool
cancel_delayed_work_sync(struct delayed_work *dw)
{
	const bool callout_was_pending = !callout_stop(&dw->dw_callout);
	const bool work_was_pending = cancel_work_sync(&dw->work);

	return (callout_was_pending || work_was_pending);
}

/*
 * XXX Bogus stubs for Linux work queues.
 */

struct workqueue_struct;

static inline struct workqueue_struct *
alloc_ordered_workqueue(const char *name __unused, int flags __unused)
{
	return (void *)(uintptr_t)0xdeadbeef;
}

static inline void
destroy_workqueue(struct workqueue_struct *wq __unused)
{
	KASSERT(wq == (void *)(uintptr_t)0xdeadbeef);
}

#define	flush_workqueue(wq)		WARN(true, "Can't flush workqueues!");
#define	flush_scheduled_work(wq)	WARN(true, "Can't flush workqueues!");

static inline void
queue_work(struct workqueue_struct *wq __unused, struct work_struct *work)
{
	KASSERT(wq == (void *)(uintptr_t)0xdeadbeef);
	schedule_work(work);
}

static inline void
queue_delayed_work(struct workqueue_struct *wq __unused,
    struct delayed_work *dw,
    unsigned int ticks)
{
	KASSERT(wq == (void *)(uintptr_t)0xdeadbeef);
	schedule_delayed_work(dw, ticks);
}

#endif  /* _LINUX_WORKQUEUE_H_ */
