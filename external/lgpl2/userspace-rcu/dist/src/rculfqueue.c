// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Lock-Free RCU Queue
 */

/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#undef _LGPL_SOURCE
#include "urcu/rculfqueue.h"
#define _LGPL_SOURCE
#include "urcu/static/rculfqueue.h"

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void cds_lfq_node_init_rcu(struct cds_lfq_node_rcu *node)
{
	_cds_lfq_node_init_rcu(node);
}

void cds_lfq_init_rcu(struct cds_lfq_queue_rcu *q,
		      void queue_call_rcu(struct rcu_head *head,
				void (*func)(struct rcu_head *head)))
{
	_cds_lfq_init_rcu(q, queue_call_rcu);
}

int cds_lfq_destroy_rcu(struct cds_lfq_queue_rcu *q)
{
	return _cds_lfq_destroy_rcu(q);
}

void cds_lfq_enqueue_rcu(struct cds_lfq_queue_rcu *q, struct cds_lfq_node_rcu *node)
{
	_cds_lfq_enqueue_rcu(q, node);
}

struct cds_lfq_node_rcu *
cds_lfq_dequeue_rcu(struct cds_lfq_queue_rcu *q)
{
	return _cds_lfq_dequeue_rcu(q);
}
