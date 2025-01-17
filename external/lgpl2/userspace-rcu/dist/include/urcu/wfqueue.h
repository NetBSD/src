// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_WFQUEUE_H
#define _URCU_WFQUEUE_H

/*
 * Userspace RCU library - Queue with Wait-Free Enqueue/Blocking Dequeue
 */

#include <pthread.h>
#include <urcu/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CDS_WFQ_DEPRECATED
#define CDS_WFQ_DEPRECATED	\
	CDS_DEPRECATED("urcu/wfqueue.h is deprecated. Please use urcu/wfcqueue.h instead.")
#endif

/*
 * Queue with wait-free enqueue/blocking dequeue.
 * This implementation adds a dummy head node when the queue is empty to ensure
 * we can always update the queue locklessly.
 *
 * Inspired from half-wait-free/half-blocking queue implementation done by
 * Paul E. McKenney.
 */

struct cds_wfq_node {
	struct cds_wfq_node *next;
};

struct cds_wfq_queue {
	struct cds_wfq_node *head, **tail;
	struct cds_wfq_node dummy;	/* Dummy node */
	pthread_mutex_t lock;
};

#ifdef _LGPL_SOURCE

#include <urcu/static/wfqueue.h>

static inline CDS_WFQ_DEPRECATED
void cds_wfq_node_init(struct cds_wfq_node *node)
{
	_cds_wfq_node_init(node);
}

static inline CDS_WFQ_DEPRECATED
void cds_wfq_init(struct cds_wfq_queue *q)
{
	_cds_wfq_init(q);
}

static inline CDS_WFQ_DEPRECATED
void cds_wfq_destroy(struct cds_wfq_queue *q)
{
	_cds_wfq_destroy(q);
}

static inline CDS_WFQ_DEPRECATED
void cds_wfq_enqueue(struct cds_wfq_queue *q, struct cds_wfq_node *node)
{
	_cds_wfq_enqueue(q, node);
}

static inline CDS_WFQ_DEPRECATED
struct cds_wfq_node *__cds_wfq_dequeue_blocking(struct cds_wfq_queue *q)
{
	return ___cds_wfq_dequeue_blocking(q);
}

static inline CDS_WFQ_DEPRECATED
struct cds_wfq_node *cds_wfq_dequeue_blocking(struct cds_wfq_queue *q)
{
	return _cds_wfq_dequeue_blocking(q);
}

#else /* !_LGPL_SOURCE */

extern CDS_WFQ_DEPRECATED
void cds_wfq_node_init(struct cds_wfq_node *node);

extern CDS_WFQ_DEPRECATED
void cds_wfq_init(struct cds_wfq_queue *q);

extern CDS_WFQ_DEPRECATED
void cds_wfq_destroy(struct cds_wfq_queue *q);

extern CDS_WFQ_DEPRECATED
void cds_wfq_enqueue(struct cds_wfq_queue *q, struct cds_wfq_node *node);

/* __cds_wfq_dequeue_blocking: caller ensures mutual exclusion between dequeues */
extern CDS_WFQ_DEPRECATED
struct cds_wfq_node *__cds_wfq_dequeue_blocking(struct cds_wfq_queue *q);

extern CDS_WFQ_DEPRECATED
struct cds_wfq_node *cds_wfq_dequeue_blocking(struct cds_wfq_queue *q);

#endif /* !_LGPL_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _URCU_WFQUEUE_H */
