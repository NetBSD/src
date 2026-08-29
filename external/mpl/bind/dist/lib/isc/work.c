/*	$NetBSD: work.c,v 1.5 2026/08/29 14:55:18 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <isc/async.h>
#include <isc/job.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/queue.h>
#include <isc/thread.h>
#include <isc/urcu.h>
#include <isc/util.h>
#include <isc/uv.h>
#include <isc/work.h>

#include "loop_p.h"

#define WORK_MAGIC	    ISC_MAGIC('W', 'o', 'r', 'k')
#define VALID_WORK(t)	    ISC_MAGIC_VALID(t, WORK_MAGIC)
#define WORKTHREAD_MAGIC    ISC_MAGIC('W', 'k', 'T', 'h')
#define VALID_WORKTHREAD(t) ISC_MAGIC_VALID(t, WORKTHREAD_MAGIC)

enum waitstate {
	/* The value a sleeping worker blocks on in FUTEX_WAIT. */
	THREAD_WAITING = 0,
	/* Any non-zero bit keeps FUTEX_WAIT from blocking. */
	THREAD_WAKEUP = (1 << 0),
	THREAD_RUNNING = (1 << 1),
	THREAD_SHUTDOWN = (1 << 2),
	THREAD_PAUSE = (1 << 3),  /* request from the owning loop */
	THREAD_PAUSED = (1 << 4), /* ack from the worker */
};

/* Sticky bits a paused worker must not drop. */
#define THREAD_STICKY (THREAD_SHUTDOWN | THREAD_PAUSE | THREAD_PAUSED)

enum workstate {
	WORK_QUEUED = 0,
	WORK_RUNNING,
	WORK_CANCELED,
};

struct isc_work {
	unsigned int magic;
	uint32_t state; /* enum workstate */
	isc_result_t result;
	isc_work_cb cb;		  /* runs on a worker thread */
	isc_work_done_cb done_cb; /* runs on the origin loop */
	void *cbarg;
	isc_loop_t *loop;	   /* origin loop, referenced */
	struct cds_wfcq_node node; /* dispatch queue linkage */
};

typedef struct isc__workthread {
	union {
		struct {
			unsigned int magic;
			isc_worklane_t lane;
			isc_loop_t *loop;
			isc_thread_t thread;
			struct __cds_wfcq_head qhead;
			int32_t state; /* enum waitstate */
		};
		uint8_t __padding0[ISC_OS_CACHELINE_SIZE];
	};
	union {
		struct cds_wfcq_tail qtail;
		uint8_t __padding1[ISC_OS_CACHELINE_SIZE];
	};
} isc__workthread_t;

STATIC_ASSERT(ISC_OS_CACHELINE_SIZE >= sizeof(struct cds_wfcq_tail),
	      "ISC_OS_CACHELINE_SIZE smaller than sizeof(struct "
	      "cds_wfcq_tail)");
STATIC_ASSERT(offsetof(isc__workthread_t, qtail) == ISC_OS_CACHELINE_SIZE,
	      "isc__workthread_t.qtail not on second cacheline");
STATIC_ASSERT(sizeof(isc__workthread_t) == 2 * ISC_OS_CACHELINE_SIZE,
	      "isc__workthread_t is not two cachelines");

static void
workthread_wake(isc__workthread_t *thread) {
	cmm_smp_mb();
	if ((uatomic_load(&thread->state, CMM_RELAXED) & THREAD_RUNNING) != 0) {
		/* Actively running; it will notice the queue on its own. */
		return;
	}

	uatomic_or(&thread->state, THREAD_WAKEUP);
	if (futex_noasync(&thread->state, FUTEX_WAKE, 1, NULL, NULL, 0) < 0) {
		FATAL_ERROR("futex_noasync(FUTEX_WAKE): %s", strerror(errno));
	}
}

static void
workthread_slumber(isc__workthread_t *thread) {
	rcu_thread_offline();
	while (futex_noasync(&thread->state, FUTEX_WAIT, THREAD_WAITING, NULL,
			     NULL, 0) != 0)
	{
		if (errno == EWOULDBLOCK) {
			break;
		} else if (errno != EINTR) {
			FATAL_ERROR("futex_noasync(FUTEX_WAIT): %s",
				    strerror(errno));
		}
		/* Or retry if interrupted by signal. */
	}
	rcu_thread_online();
}

static void
workthread_sleep(isc__workthread_t *thread) {
	/*
	 * Drop to WAITING while keeping a pending SHUTDOWN/PAUSE sticky, so the
	 * FUTEX_WAIT below refuses to block once either is signalled.
	 */
	uatomic_and(&thread->state, THREAD_STICKY);
	cmm_smp_mb();

	/*
	 * The queue is the one wake condition that can't live in 'state', so
	 * recheck it under the fence; SHUTDOWN and WAKEUP are handled by
	 * FUTEX_WAIT's own value check.
	 */
	if (cds_wfcq_empty(&thread->qhead, &thread->qtail)) {
		workthread_slumber(thread);
	}

	/* Tell the waker we are running (keeping any sticky SHUTDOWN/PAUSE). */
	uatomic_or(&thread->state, THREAD_RUNNING);
}

/*
 * Acknowledge a pause request: publish PAUSED (dropping RUNNING/WAKEUP) and
 * wake the waiting pauser.  A new pause clears PAUSED, so the worker re-acks
 * and the pauser only ever observes an ack set for its own request, never a
 * stale one from the previous pause generation.
 */
static void
workthread_ack_pause(isc__workthread_t *thread) {
	int32_t old, next;
	do {
		old = uatomic_load(&thread->state, CMM_RELAXED);
		next = (old & THREAD_STICKY) | THREAD_PAUSED;
	} while (uatomic_cmpxchg(&thread->state, old, next) != old);

	(void)futex_noasync(&thread->state, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
}

/*
 * Honour a pause: pause until the owning loop clears PAUSE (resume).  A fresh
 * pause clears PAUSED (see isc__workthread_pause), so (re-)ack whenever PAUSED
 * is gone — the pauser only proceeds on an ack set for *its* request, never a
 * stale one from the previous generation.  Stays RCU-offline while paused so
 * it can't hold up an exclusive-mode grace period.
 */
static void
workthread_pause(isc__workthread_t *thread) {
	rcu_thread_offline();

	while (true) {
		int32_t old = uatomic_load(&thread->state, CMM_ACQUIRE);
		if ((old & (THREAD_PAUSE | THREAD_SHUTDOWN)) != THREAD_PAUSE) {
			break;
		}
		if ((old & THREAD_PAUSED) == 0) {
			workthread_ack_pause(thread);
			continue;
		}
		(void)futex_noasync(&thread->state, FUTEX_WAIT, old, NULL, NULL,
				    0);
	}

	uatomic_and(&thread->state, ~THREAD_PAUSED);
	rcu_thread_online();
}

static void
work_done(void *arg) {
	isc_work_t *work = arg;
	isc_loop_t *loop = work->loop;

	/* work_run() has settled work->result before scheduling us. */
	INSIST(work->result != ISC_R_UNSET);

	work->done_cb(work->cbarg, work->result);

	work->magic = 0;
	isc_mem_put(work->loop->mctx, work, sizeof(*work));
	isc_loop_unref(loop);
}

static void
work_run(void *arg) {
	isc_work_t *work = arg;
	/*
	 * The CAS *is* the tombstone check: whoever moves the item out
	 * of WORK_QUEUED first — this worker or isc_work_cancel() —
	 * decides whether the callback runs.  uatomic_cmpxchg returns the
	 * prior state, so WORK_QUEUED means we won the race.
	 */
	uint32_t prev = uatomic_cmpxchg(&work->state, WORK_QUEUED,
					WORK_RUNNING);
	switch (prev) {
	case WORK_QUEUED:
		work->result = work->cb(work->cbarg);
		break;
	case WORK_CANCELED:
		work->result = ISC_R_CANCELED;
		break;
	default:
		UNREACHABLE();
	}

	/* Completion always routes back to the origin loop. */
	isc_async_run(work->loop, work_done, work);
}

static void *
workthread_thread(void *arg) {
	isc__workthread_t *thread = arg;

	isc__loopmgr_starting(thread->loop);

	while (true) {
		/*
		 * Honour a pause before touching the queue (gated on !SHUTDOWN
		 * so a shutting-down worker exits instead of pausing).
		 */
		int32_t state = uatomic_load(&thread->state, CMM_ACQUIRE);
		if ((state & (THREAD_PAUSE | THREAD_SHUTDOWN)) == THREAD_PAUSE)
		{
			workthread_pause(thread);
			continue;
		}

		struct cds_wfcq_node *node;
		node = __cds_wfcq_dequeue_blocking(&thread->qhead,
						   &thread->qtail);

		if (node == NULL) {
			/*
			 * Only exit the loop if there's nothing to do.
			 */
			if ((uatomic_load(&thread->state, CMM_ACQUIRE) &
			     THREAD_SHUTDOWN) != 0)
			{
				synchronize_rcu();
				if (!cds_wfcq_empty(&thread->qhead,
						    &thread->qtail))
				{
					continue;
				}
				break;
			}

			workthread_sleep(thread);

			continue;
		}

		isc_work_t *work = caa_container_of(node, isc_work_t, node);
		work_run(work);
	}

	isc__loopmgr_stopping(thread->loop);

	return NULL;
}

isc_work_t *
isc_work_enqueue(isc_loop_t *loop, isc_worklane_t lane, isc_work_cb cb,
		 isc_work_done_cb done_cb, void *cbarg) {
	REQUIRE(loop == isc_loop());

	isc__workthread_t *thread = isc__loopmgr_workthread(loop, lane);

	isc_work_t *work = isc_mem_get(loop->mctx, sizeof(*work));
	*work = (isc_work_t){
		.magic = WORK_MAGIC,
		.result = ISC_R_UNSET,
		.cb = cb,
		.done_cb = done_cb,
		.cbarg = cbarg,
		.loop = isc_loop_ref(loop),
		.state = WORK_QUEUED,
	};

	rcu_read_lock();
	if ((uatomic_load(&thread->state, CMM_ACQUIRE) & THREAD_SHUTDOWN) != 0)
	{
		rcu_read_unlock();

		/*
		 * We are shutting down, so immedaitely run task instead of
		 * adding more in the queue. (The worker is running the
		 * remaining enqueue tasks and shutdown after, see
		 * workthread_thread().)
		 */
		isc_async_run(loop, work_run, work);
	} else {
		(void)cds_wfcq_enqueue(&thread->qhead, &thread->qtail,
				       &work->node);
		rcu_read_unlock();

		if ((uatomic_load(&thread->state, CMM_ACQUIRE) &
		     THREAD_RUNNING) == 0)
		{
			workthread_wake(thread);
		}
	}

	return work;
}

bool
isc_work_cancel(isc_work_t *work) {
	REQUIRE(VALID_WORK(work));

	/*
	 * Tombstone: QUEUED -> CANCELED.  The node stays in the queue
	 * (no interior unlink in a singly-linked lock-free queue) and
	 * is discarded by whichever worker dequeues it; done_cb still
	 * fires with ISC_R_CANCELED.  Nothing is freed here.  False
	 * means the callback is running or done — uv_cancel semantics.
	 */
	return uatomic_cmpxchg(&work->state, WORK_QUEUED, WORK_CANCELED) ==
	       WORK_QUEUED;
}

isc__workthread_t *
isc__workthread_create(isc_loop_t *loop, isc_worklane_t lane) {
	isc__workthread_t *thread = isc_mem_get(loop->mctx, sizeof(*thread));

	*thread = (isc__workthread_t){
		.lane = lane,
		.magic = WORKTHREAD_MAGIC,
		.state = THREAD_WAITING,
		.loop = loop,
	};

	__cds_wfcq_init(&thread->qhead, &thread->qtail);

	isc_thread_create(workthread_thread, thread, &thread->thread);

	return thread;
}

void
isc__workthread_shutdown(isc__workthread_t *thread) {
	REQUIRE(VALID_WORKTHREAD(thread));

	/*
	 * Not called while the worker is paused by isc__workthread_pause():
	 * shutdown callbacks run from uv loops, and loopmgr pause keeps every
	 * loop out of uv_run() until resume, so PAUSE and SHUTDOWN never
	 * coexist on a worker (the SHUTDOWN checks in the pause path are only
	 * a belt-and-braces exit if that ever changed).
	 */

	/* Set the sticky SHUTDOWN bit once; bail if already shutting down. */
	int32_t old;
	do {
		old = uatomic_load(&thread->state, CMM_RELAXED);
		if ((old & THREAD_SHUTDOWN) != 0) {
			return;
		}
	} while (uatomic_cmpxchg(&thread->state, old, old | THREAD_SHUTDOWN) !=
		 old);

	/* Fence in-flight enqueues (which touch the queue) before draining. */
	synchronize_rcu();

	workthread_wake(thread);
}

void
isc__workthread_destroy(isc__workthread_t **threadp) {
	REQUIRE(threadp != NULL && VALID_WORKTHREAD(*threadp));
	isc__workthread_t *thread = MOVE_OWNERSHIP(*threadp);

	isc_thread_join(thread->thread, NULL);

	INSIST(cds_wfcq_empty(&thread->qhead, &thread->qtail));

	thread->magic = 0;
	isc_mem_put(thread->loop->mctx, thread, sizeof(*thread));
}

void
isc__workthread_pause(isc__workthread_t *thread) {
	REQUIRE(VALID_WORKTHREAD(thread));

	/*
	 * Request a pause, but only if not already shutting down — a
	 * shutting-down worker heads for the stopping barrier and must never
	 * be waited on here (that'd be a deadlock).  Clearing PAUSED as we set
	 * PAUSE invalidates any ack left over from the previous generation, so
	 * the wait below can only succeed on an ack for this request.
	 */
	int32_t old;
	do {
		old = uatomic_load(&thread->state, CMM_RELAXED);
		if ((old & THREAD_SHUTDOWN) != 0) {
			return;
		}
	} while (uatomic_cmpxchg(&thread->state, old,
				 (old | THREAD_PAUSE) & ~THREAD_PAUSED) != old);

	workthread_wake(thread);

	/*
	 * Wait for the worker to acknowledge (PAUSED, form workthread_thread()
	 * calling workthread_pause()) or for shutdown.
	 */
	while (true) {
		old = uatomic_load(&thread->state, CMM_ACQUIRE);
		if ((old & (THREAD_PAUSED | THREAD_SHUTDOWN)) != 0) {
			return;
		}
		(void)futex_noasync(&thread->state, FUTEX_WAIT, old, NULL, NULL,
				    0);
	}
}

void
isc__workthread_resume(isc__workthread_t *thread) {
	REQUIRE(VALID_WORKTHREAD(thread));

	/* Clear the request and wake the paused worker. */
	uatomic_and(&thread->state, ~THREAD_PAUSE);
	(void)futex_noasync(&thread->state, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
}
