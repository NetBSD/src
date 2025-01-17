// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Queue with Wait-Free Enqueue/Blocking Dequeue
 */

/* Remove deprecation warnings from LGPL wrapper build. */
#define CDS_WFQ_DEPRECATED

/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#include "urcu/wfqueue.h"
#include "urcu/static/wfqueue.h"

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void cds_wfq_node_init(struct cds_wfq_node *node)
{
	_cds_wfq_node_init(node);
}

void cds_wfq_init(struct cds_wfq_queue *q)
{
	_cds_wfq_init(q);
}

void cds_wfq_destroy(struct cds_wfq_queue *q)
{
	_cds_wfq_destroy(q);
}

void cds_wfq_enqueue(struct cds_wfq_queue *q, struct cds_wfq_node *node)
{
	_cds_wfq_enqueue(q, node);
}

struct cds_wfq_node *__cds_wfq_dequeue_blocking(struct cds_wfq_queue *q)
{
	return ___cds_wfq_dequeue_blocking(q);
}

struct cds_wfq_node *cds_wfq_dequeue_blocking(struct cds_wfq_queue *q)
{
	return _cds_wfq_dequeue_blocking(q);
}
