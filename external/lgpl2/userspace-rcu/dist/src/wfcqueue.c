// SPDX-FileCopyrightText: 2010-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2011-2012 Lai Jiangshan <laijs@cn.fujitsu.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Concurrent Queue with Wait-Free Enqueue/Blocking Dequeue
 */

/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#include "urcu/wfcqueue.h"
#include "urcu/static/wfcqueue.h"

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void cds_wfcq_node_init(struct cds_wfcq_node *node)
{
	_cds_wfcq_node_init(node);
}

void cds_wfcq_init(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail)
{
	_cds_wfcq_init(head, tail);
}

void cds_wfcq_destroy(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail)
{
	_cds_wfcq_destroy(head, tail);
}

void __cds_wfcq_init(struct __cds_wfcq_head *head,
		struct cds_wfcq_tail *tail)
{
	___cds_wfcq_init(head, tail);
}

bool cds_wfcq_empty(cds_wfcq_head_const_ptr_t head,
		const struct cds_wfcq_tail *tail)

{
	return _cds_wfcq_empty(head, tail);
}

bool cds_wfcq_enqueue(cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node)
{
	return _cds_wfcq_enqueue(head, tail, node);
}

void cds_wfcq_dequeue_lock(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail)
{
	_cds_wfcq_dequeue_lock(head, tail);
}

void cds_wfcq_dequeue_unlock(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail)
{
	_cds_wfcq_dequeue_unlock(head, tail);
}

struct cds_wfcq_node *cds_wfcq_dequeue_blocking(
		struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail)
{
	return _cds_wfcq_dequeue_blocking(head, tail);
}

struct cds_wfcq_node *cds_wfcq_dequeue_with_state_blocking(
		struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail,
		int *state)
{
	return _cds_wfcq_dequeue_with_state_blocking(head, tail, state);
}

enum cds_wfcq_ret cds_wfcq_splice_blocking(
		struct cds_wfcq_head *dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		struct cds_wfcq_head *src_q_head,
		struct cds_wfcq_tail *src_q_tail)
{
	return _cds_wfcq_splice_blocking(dest_q_head, dest_q_tail,
				src_q_head, src_q_tail);
}

struct cds_wfcq_node *__cds_wfcq_dequeue_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail)
{
	return ___cds_wfcq_dequeue_blocking(head, tail);
}

struct cds_wfcq_node *__cds_wfcq_dequeue_with_state_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		int *state)
{
	return ___cds_wfcq_dequeue_with_state_blocking(head, tail, state);
}

struct cds_wfcq_node *__cds_wfcq_dequeue_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail)
{
	return ___cds_wfcq_dequeue_nonblocking(head, tail);
}

struct cds_wfcq_node *__cds_wfcq_dequeue_with_state_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		int *state)
{
	return ___cds_wfcq_dequeue_with_state_nonblocking(head, tail, state);
}

enum cds_wfcq_ret __cds_wfcq_splice_blocking(
		cds_wfcq_head_ptr_t dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		cds_wfcq_head_ptr_t src_q_head,
		struct cds_wfcq_tail *src_q_tail)
{
	return ___cds_wfcq_splice_blocking(dest_q_head, dest_q_tail,
				src_q_head, src_q_tail);
}

enum cds_wfcq_ret __cds_wfcq_splice_nonblocking(
		cds_wfcq_head_ptr_t dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		cds_wfcq_head_ptr_t src_q_head,
		struct cds_wfcq_tail *src_q_tail)
{
	return ___cds_wfcq_splice_nonblocking(dest_q_head, dest_q_tail,
				src_q_head, src_q_tail);
}

struct cds_wfcq_node *__cds_wfcq_first_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail)
{
	return ___cds_wfcq_first_blocking(head, tail);
}

struct cds_wfcq_node *__cds_wfcq_first_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail)
{
	return ___cds_wfcq_first_nonblocking(head, tail);
}

struct cds_wfcq_node *__cds_wfcq_next_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node)
{
	return ___cds_wfcq_next_blocking(head, tail, node);
}

struct cds_wfcq_node *__cds_wfcq_next_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node)
{
	return ___cds_wfcq_next_nonblocking(head, tail, node);
}
