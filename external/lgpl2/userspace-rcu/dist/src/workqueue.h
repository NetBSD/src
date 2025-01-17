// SPDX-FileCopyrightText: 2009,2017 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_WORKQUEUE_H
#define _URCU_WORKQUEUE_H

/*
 * Userspace RCU header - Userspace workqueues
 */

#include <stdlib.h>
#include <pthread.h>

#include <urcu/wfcqueue.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note that struct urcu_workqueue is opaque to callers. */

struct urcu_workqueue;
struct urcu_workqueue_completion;

/* Flag values. */

#define URCU_WORKQUEUE_RT	(1U << 0)
#define URCU_WORKQUEUE_STOP	(1U << 1)
#define URCU_WORKQUEUE_PAUSE	(1U << 2)
#define URCU_WORKQUEUE_PAUSED	(1U << 3)

/*
 * The urcu_work data structure is placed in the structure to be acted
 * upon via urcu_workqueue_queue_work().
 */

struct urcu_work {
	struct cds_wfcq_node next;
	void (*func)(struct urcu_work *head);
};

/*
 * Exported functions
 */

struct urcu_workqueue *urcu_workqueue_create(unsigned long flags,
		int cpu_affinity, void *priv,
		void (*grace_period_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*initialize_worker_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*finalize_worker_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_before_wait_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_after_wake_up_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_before_pause_fct)(struct urcu_workqueue *workqueue, void *priv),
		void (*worker_after_resume_fct)(struct urcu_workqueue *workqueue, void *priv));
void urcu_workqueue_destroy(struct urcu_workqueue *workqueue);

/*
 * Never fails. Should not be used to enqueue work from worker threads
 * after the application invokes urcu_workqueue_free.
 */
void urcu_workqueue_queue_work(struct urcu_workqueue *workqueue,
		struct urcu_work *work,
		void (*func)(struct urcu_work *work));

struct urcu_workqueue_completion *urcu_workqueue_create_completion(void);
void urcu_workqueue_destroy_completion(struct urcu_workqueue_completion *completion);

void urcu_workqueue_queue_completion(struct urcu_workqueue *workqueue,
		struct urcu_workqueue_completion *completion);
void urcu_workqueue_wait_completion(struct urcu_workqueue_completion *completion);

void urcu_workqueue_flush_queued_work(struct urcu_workqueue *workqueue);

/*
 * pause/resume/create worker threads. Can be used to pause worker
 * threads across fork/clone while keeping the workqueue in place.
 * Pause is used in parent pre-fork, resume in parent post-fork, create
 * in child after-fork.
 */
void urcu_workqueue_pause_worker(struct urcu_workqueue *workqueue);
void urcu_workqueue_resume_worker(struct urcu_workqueue *workqueue);
void urcu_workqueue_create_worker(struct urcu_workqueue *workqueue);

#ifdef __cplusplus
}
#endif

#endif /* _URCU_WORKQUEUE_H */
