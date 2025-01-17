// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_WAIT_H
#define _URCU_WAIT_H

/*
 * Userspace RCU library wait/wakeup management
 */

#include <urcu/assert.h>
#include <urcu/uatomic.h>
#include <urcu/wfstack.h>
#include <urcu/futex.h>
#include "urcu-die.h"

/*
 * Number of busy-loop attempts before waiting on futex for grace period
 * batching.
 */
#define URCU_WAIT_ATTEMPTS 1000

enum urcu_wait_state {
	/* URCU_WAIT_WAITING is compared directly (futex compares it). */
	URCU_WAIT_WAITING =	0,
	/* non-zero are used as masks. */
	URCU_WAIT_WAKEUP =	(1 << 0),
	URCU_WAIT_RUNNING =	(1 << 1),
	URCU_WAIT_TEARDOWN =	(1 << 2),
};

struct urcu_wait_node {
	struct cds_wfs_node node;
	int32_t state;	/* enum urcu_wait_state */
};

#define URCU_WAIT_NODE_INIT(name, _state)		\
	{ .node = { .next = NULL }, .state = _state }

#define DEFINE_URCU_WAIT_NODE(name, state)		\
	struct urcu_wait_node name = URCU_WAIT_NODE_INIT(name, state)

#define DECLARE_URCU_WAIT_NODE(name)			\
	struct urcu_wait_node name

struct urcu_wait_queue {
	struct cds_wfs_stack stack;
};

#define URCU_WAIT_QUEUE_HEAD_INIT(name)				\
	{							\
		 .stack = {					\
			.head = CDS_WFS_END,			\
			.lock = PTHREAD_MUTEX_INITIALIZER,	\
		},						\
	}

#define DECLARE_URCU_WAIT_QUEUE(name)			\
	struct urcu_wait_queue name

#define DEFINE_URCU_WAIT_QUEUE(name)			\
	struct urcu_wait_queue name = URCU_WAIT_QUEUE_HEAD_INIT(name)

struct urcu_waiters {
	struct cds_wfs_head *head;
};

/*
 * Add ourself atomically to a wait queue. Return 0 if queue was
 * previously empty, else return 1.
 * A full memory barrier is issued before being added to the wait queue.
 */
static inline
bool urcu_wait_add(struct urcu_wait_queue *queue,
		struct urcu_wait_node *node)
{
	return cds_wfs_push(&queue->stack, &node->node);
}

/*
 * Atomically move all waiters from wait queue into our local struct
 * urcu_waiters.
 */
static inline
void urcu_move_waiters(struct urcu_waiters *waiters,
		struct urcu_wait_queue *queue)
{
	waiters->head = __cds_wfs_pop_all(&queue->stack);
}

static inline
void urcu_wait_set_state(struct urcu_wait_node *node,
		enum urcu_wait_state state)
{
	node->state = state;
}

static inline
void urcu_wait_node_init(struct urcu_wait_node *node,
		enum urcu_wait_state state)
{
	urcu_wait_set_state(node, state);
	cds_wfs_node_init(&node->node);
}

/*
 * Note: urcu_adaptative_wake_up needs "value" to stay allocated
 * throughout its execution. In this scheme, the waiter owns the node
 * memory, and we only allow it to free this memory when it receives the
 * URCU_WAIT_TEARDOWN flag.
 */
static inline
void urcu_adaptative_wake_up(struct urcu_wait_node *wait)
{
	urcu_posix_assert(uatomic_read(&wait->state) == URCU_WAIT_WAITING);
	uatomic_store(&wait->state, URCU_WAIT_WAKEUP, CMM_RELEASE);
	if (!(uatomic_read(&wait->state) & URCU_WAIT_RUNNING)) {
		if (futex_noasync(&wait->state, FUTEX_WAKE, 1,
				NULL, NULL, 0) < 0)
			urcu_die(errno);
	}
	/* Allow teardown of struct urcu_wait memory. */
	uatomic_or_mo(&wait->state, URCU_WAIT_TEARDOWN, CMM_RELEASE);
}

/*
 * Caller must initialize "value" to URCU_WAIT_WAITING before passing its
 * memory to waker thread.
 */
static inline
void urcu_adaptative_busy_wait(struct urcu_wait_node *wait)
{
	unsigned int i;

	/* Load and test condition before read state */
	cmm_smp_rmb();
	for (i = 0; i < URCU_WAIT_ATTEMPTS; i++) {
		if (uatomic_load(&wait->state, CMM_ACQUIRE) != URCU_WAIT_WAITING)
			goto skip_futex_wait;
		caa_cpu_relax();
	}
	while (uatomic_load(&wait->state, CMM_ACQUIRE) == URCU_WAIT_WAITING) {
		if (!futex_noasync(&wait->state, FUTEX_WAIT, URCU_WAIT_WAITING, NULL, NULL, 0)) {
			/*
			 * Prior queued wakeups queued by unrelated code
			 * using the same address can cause futex wait to
			 * return 0 even through the futex value is still
			 * URCU_WAIT_WAITING (spurious wakeups). Check
			 * the value again in user-space to validate
			 * whether it really differs from
			 * URCU_WAIT_WAITING.
			 */
			continue;
		}
		switch (errno) {
		case EAGAIN:
			/* Value already changed. */
			goto skip_futex_wait;
		case EINTR:
			/* Retry if interrupted by signal. */
			break;	/* Get out of switch. Check again. */
		default:
			/* Unexpected error. */
			urcu_die(errno);
		}
	}
skip_futex_wait:

	/* Tell waker thread than we are running. */
	uatomic_or(&wait->state, URCU_WAIT_RUNNING);

	/*
	 * Wait until waker thread lets us know it's ok to tear down
	 * memory allocated for struct urcu_wait.
	 */
	for (i = 0; i < URCU_WAIT_ATTEMPTS; i++) {
		if (uatomic_load(&wait->state, CMM_RELAXED) & URCU_WAIT_TEARDOWN)
			break;
		caa_cpu_relax();
	}
	while (!(uatomic_load(&wait->state, CMM_ACQUIRE) & URCU_WAIT_TEARDOWN))
		poll(NULL, 0, 10);
	urcu_posix_assert(uatomic_read(&wait->state) & URCU_WAIT_TEARDOWN);
}

static inline
void urcu_wake_all_waiters(struct urcu_waiters *waiters)
{
	struct cds_wfs_node *iter, *iter_n;

	/* Wake all waiters in our stack head */
	cds_wfs_for_each_blocking_safe(waiters->head, iter, iter_n) {
		struct urcu_wait_node *wait_node =
			caa_container_of(iter, struct urcu_wait_node, node);

		/* Don't wake already running threads */
		if (uatomic_load(&wait_node->state, CMM_RELAXED) & URCU_WAIT_RUNNING)
			continue;
		urcu_adaptative_wake_up(wait_node);
	}
}

#endif /* _URCU_WAIT_H */
