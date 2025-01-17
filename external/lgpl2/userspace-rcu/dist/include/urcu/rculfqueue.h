// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_RCULFQUEUE_H
#define _URCU_RCULFQUEUE_H

/*
 * Userspace RCU library - Lock-Free RCU Queue
 */


#ifdef __cplusplus
extern "C" {
#endif

struct cds_lfq_queue_rcu;
struct rcu_head;

struct cds_lfq_node_rcu {
	struct cds_lfq_node_rcu *next;
	int dummy;
};

struct cds_lfq_queue_rcu {
	struct cds_lfq_node_rcu *head, *tail;
	void (*queue_call_rcu)(struct rcu_head *head,
		void (*func)(struct rcu_head *head));
};

#ifdef _LGPL_SOURCE

#include <urcu/static/rculfqueue.h>

#define cds_lfq_node_init_rcu		_cds_lfq_node_init_rcu
#define cds_lfq_init_rcu		_cds_lfq_init_rcu
#define cds_lfq_destroy_rcu		_cds_lfq_destroy_rcu
#define cds_lfq_enqueue_rcu		_cds_lfq_enqueue_rcu
#define cds_lfq_dequeue_rcu		_cds_lfq_dequeue_rcu

#else /* !_LGPL_SOURCE */

extern void cds_lfq_node_init_rcu(struct cds_lfq_node_rcu *node);
extern void cds_lfq_init_rcu(struct cds_lfq_queue_rcu *q,
			     void queue_call_rcu(struct rcu_head *head,
					void (*func)(struct rcu_head *head)));
/*
 * The queue should be emptied before calling destroy.
 *
 * Return 0 on success, -EPERM if queue is not empty.
 */
extern int cds_lfq_destroy_rcu(struct cds_lfq_queue_rcu *q);

/*
 * Should be called under rcu read lock critical section.
 */
extern void cds_lfq_enqueue_rcu(struct cds_lfq_queue_rcu *q,
				struct cds_lfq_node_rcu *node);

/*
 * Should be called under rcu read lock critical section.
 *
 * The caller must wait for a grace period to pass before freeing the returned
 * node or modifying the cds_lfq_node_rcu structure.
 * Returns NULL if queue is empty.
 */
extern
struct cds_lfq_node_rcu *cds_lfq_dequeue_rcu(struct cds_lfq_queue_rcu *q);

#endif /* !_LGPL_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _URCU_RCULFQUEUE_H */
