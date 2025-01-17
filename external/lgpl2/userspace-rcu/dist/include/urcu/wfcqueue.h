// SPDX-FileCopyrightText: 2010-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2011-2012 Lai Jiangshan <laijs@cn.fujitsu.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_WFCQUEUE_H
#define _URCU_WFCQUEUE_H

/*
 * Userspace RCU library - Concurrent Queue with Wait-Free Enqueue/Blocking Dequeue
 */

#include <pthread.h>
#include <stdbool.h>
#include <urcu/compiler.h>
#include <urcu/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Concurrent queue with wait-free enqueue/blocking dequeue.
 *
 * This queue has been designed and implemented collaboratively by
 * Mathieu Desnoyers and Lai Jiangshan. Inspired from
 * half-wait-free/half-blocking queue implementation done by Paul E.
 * McKenney.
 */

#define CDS_WFCQ_WOULDBLOCK	((struct cds_wfcq_node *) -1UL)

enum cds_wfcq_ret {
	CDS_WFCQ_RET_WOULDBLOCK =	-1,
	CDS_WFCQ_RET_DEST_EMPTY =	0,
	CDS_WFCQ_RET_DEST_NON_EMPTY =	1,
	CDS_WFCQ_RET_SRC_EMPTY = 	2,
};

enum cds_wfcq_state {
	CDS_WFCQ_STATE_LAST =		(1U << 0),
};

struct cds_wfcq_node {
	struct cds_wfcq_node *next;
};

/*
 * Do not put head and tail on the same cache-line if concurrent
 * enqueue/dequeue are expected from many CPUs. This eliminates
 * false-sharing between enqueue and dequeue.
 */
struct __cds_wfcq_head {
	struct cds_wfcq_node node;
};

struct cds_wfcq_head {
	struct cds_wfcq_node node;
	pthread_mutex_t lock;
};

/*
 * In C, the transparent union allows calling functions that work on both
 * struct cds_wfcq_head and struct __cds_wfcq_head on any of those two
 * types.
 *
 * In C++, implement static inline wrappers using function overloading
 * to obtain an API similar to C.
 *
 * Avoid complaints from clang++ not knowing the transparent union
 * attribute.
 */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#endif
typedef union {
	struct __cds_wfcq_head *_h;
	struct cds_wfcq_head *h;
} __attribute__((__transparent_union__)) cds_wfcq_head_ptr_t;

typedef union {
	const struct __cds_wfcq_head *_h;
	const struct cds_wfcq_head *h;
} __attribute__((__transparent_union__)) cds_wfcq_head_const_ptr_t;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifndef __cplusplus
/*
 * This static inline is only present for compatibility with C++. It is
 * effect-less in C.
 */
static inline struct __cds_wfcq_head *__cds_wfcq_head_cast(struct __cds_wfcq_head *head)
{
	return head;
}

/*
 * This static inline is only present for compatibility with C++. It is
 * effect-less in C.
 */
static inline struct cds_wfcq_head *cds_wfcq_head_cast(struct cds_wfcq_head *head)
{
	return head;
}

/*
 * This static inline is only present for compatibility with C++. It is
 * effect-less in C.
 */
static inline const struct __cds_wfcq_head *__cds_wfcq_head_const_cast(const struct __cds_wfcq_head *head)
{
	return head;
}

/*
 * This static inline is only present for compatibility with C++. It is
 * effect-less in C.
 */
static inline const struct cds_wfcq_head *cds_wfcq_head_const_cast(const struct cds_wfcq_head *head)
{
	return head;
}

#else /* #ifndef __cplusplus */

/*
 * This static inline is used by internally in the static inline
 * implementation of the API.
 */
static inline cds_wfcq_head_ptr_t __cds_wfcq_head_cast(struct __cds_wfcq_head *head)
{
	cds_wfcq_head_ptr_t ret = { ._h = head };
	return ret;
}

/*
 * This static inline is used by internally in the static inline
 * implementation of the API.
 */
static inline cds_wfcq_head_ptr_t cds_wfcq_head_cast(struct cds_wfcq_head *head)
{
	cds_wfcq_head_ptr_t ret = { .h = head };
	return ret;
}

/*
 * This static inline is used by internally in the static inline
 * implementation of the API.
 */
static inline cds_wfcq_head_const_ptr_t __cds_wfcq_head_const_cast(const struct __cds_wfcq_head *head)
{
	cds_wfcq_head_const_ptr_t ret = { ._h = head };
	return ret;
}

/*
 * This static inline is used by internally in the static inline
 * implementation of the API.
 */
static inline cds_wfcq_head_const_ptr_t cds_wfcq_head_const_cast(const struct cds_wfcq_head *head)
{
	cds_wfcq_head_const_ptr_t ret = { .h = head };
	return ret;
}

#endif /* #else #ifndef __cplusplus */

struct cds_wfcq_tail {
	struct cds_wfcq_node *p;
};

#ifdef _LGPL_SOURCE

#include <urcu/static/wfcqueue.h>

#define cds_wfcq_node_init		_cds_wfcq_node_init
#define cds_wfcq_init			_cds_wfcq_init
#define __cds_wfcq_init			___cds_wfcq_init
#define cds_wfcq_destroy		_cds_wfcq_destroy
#define cds_wfcq_empty			_cds_wfcq_empty
#define cds_wfcq_enqueue		_cds_wfcq_enqueue

/* Dequeue locking */
#define cds_wfcq_dequeue_lock		_cds_wfcq_dequeue_lock
#define cds_wfcq_dequeue_unlock		_cds_wfcq_dequeue_unlock

/* Locking performed within cds_wfcq calls. */
#define cds_wfcq_dequeue_blocking	_cds_wfcq_dequeue_blocking
#define cds_wfcq_dequeue_with_state_blocking	\
					_cds_wfcq_dequeue_with_state_blocking
#define cds_wfcq_splice_blocking	_cds_wfcq_splice_blocking
#define cds_wfcq_first_blocking		_cds_wfcq_first_blocking
#define cds_wfcq_next_blocking		_cds_wfcq_next_blocking

/* Locking ensured by caller by holding cds_wfcq_dequeue_lock() */
#define __cds_wfcq_dequeue_blocking	___cds_wfcq_dequeue_blocking
#define __cds_wfcq_dequeue_with_state_blocking	\
					___cds_wfcq_dequeue_with_state_blocking
#define __cds_wfcq_splice_blocking	___cds_wfcq_splice_blocking
#define __cds_wfcq_first_blocking	___cds_wfcq_first_blocking
#define __cds_wfcq_next_blocking	___cds_wfcq_next_blocking

/*
 * Locking ensured by caller by holding cds_wfcq_dequeue_lock().
 * Non-blocking: deque, first, next return CDS_WFCQ_WOULDBLOCK if they
 * need to block. splice returns nonzero if it needs to block.
 */
#define __cds_wfcq_dequeue_nonblocking	___cds_wfcq_dequeue_nonblocking
#define __cds_wfcq_dequeue_with_state_nonblocking	\
				___cds_wfcq_dequeue_with_state_nonblocking
#define __cds_wfcq_splice_nonblocking	___cds_wfcq_splice_nonblocking
#define __cds_wfcq_first_nonblocking	___cds_wfcq_first_nonblocking
#define __cds_wfcq_next_nonblocking	___cds_wfcq_next_nonblocking

#else /* !_LGPL_SOURCE */

/*
 * Mutual exclusion of cds_wfcq_* / __cds_wfcq_* API
 *
 * Synchronization table:
 *
 * External synchronization techniques described in the API below is
 * required between pairs marked with "X". No external synchronization
 * required between pairs marked with "-".
 *
 * Legend:
 * [1] cds_wfcq_enqueue
 * [2] __cds_wfcq_splice (destination queue)
 * [3] __cds_wfcq_dequeue
 * [4] __cds_wfcq_splice (source queue)
 * [5] __cds_wfcq_first
 * [6] __cds_wfcq_next
 *
 *     [1] [2] [3] [4] [5] [6]
 * [1]  -   -   -   -   -   -
 * [2]  -   -   -   -   -   -
 * [3]  -   -   X   X   X   X
 * [4]  -   -   X   -   X   X
 * [5]  -   -   X   X   -   -
 * [6]  -   -   X   X   -   -
 *
 * Mutual exclusion can be ensured by holding cds_wfcq_dequeue_lock().
 *
 * For convenience, cds_wfcq_dequeue_blocking() and
 * cds_wfcq_splice_blocking() hold the dequeue lock.
 *
 * Besides locking, mutual exclusion of dequeue, splice and iteration
 * can be ensured by performing all of those operations from a single
 * thread, without requiring any lock.
 */

/*
 * cds_wfcq_node_init: initialize wait-free queue node.
 */
extern void cds_wfcq_node_init(struct cds_wfcq_node *node);

/*
 * cds_wfcq_init: initialize wait-free queue. Pair with
 * cds_wfcq_destroy().
 */
extern void cds_wfcq_init(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail);

/*
 * cds_wfcq_destroy: destroy wait-free queue. Pair with
 * cds_wfcq_init().
 */
extern void cds_wfcq_destroy(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail);

/*
 * __cds_wfcq_init: initialize wait-free queue (without lock). Don't
 * pair with any destroy function.
 */
extern void __cds_wfcq_init(struct __cds_wfcq_head *head,
		struct cds_wfcq_tail *tail);

/*
 * cds_wfcq_empty: return whether wait-free queue is empty.
 *
 * No memory barrier is issued. No mutual exclusion is required.
 */
extern bool cds_wfcq_empty(cds_wfcq_head_const_ptr_t head,
		const struct cds_wfcq_tail *tail);

/*
 * cds_wfcq_dequeue_lock: take the dequeue mutual exclusion lock.
 */
extern void cds_wfcq_dequeue_lock(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail);

/*
 * cds_wfcq_dequeue_unlock: release the dequeue mutual exclusion lock.
 */
extern void cds_wfcq_dequeue_unlock(struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail);

/*
 * cds_wfcq_enqueue: enqueue a node into a wait-free queue.
 *
 * Issues a full memory barrier before enqueue. No mutual exclusion is
 * required.
 *
 * Returns false if the queue was empty prior to adding the node.
 * Returns true otherwise.
 */
extern bool cds_wfcq_enqueue(cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node);

/*
 * cds_wfcq_dequeue_blocking: dequeue a node from a wait-free queue.
 *
 * Content written into the node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * It is valid to reuse and free a dequeued node immediately.
 * Mutual exclusion with cds_wfcq_dequeue_blocking and dequeue lock is
 * ensured.
 */
extern struct cds_wfcq_node *cds_wfcq_dequeue_blocking(
		struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail);

/*
 * cds_wfcq_dequeue_with_state_blocking: dequeue with state.
 *
 * Same as cds_wfcq_dequeue_blocking, but saves whether dequeueing the
 * last node of the queue into state (CDS_WFCQ_STATE_LAST).
 */
extern struct cds_wfcq_node *cds_wfcq_dequeue_with_state_blocking(
		struct cds_wfcq_head *head,
		struct cds_wfcq_tail *tail,
		int *state);

/*
 * cds_wfcq_splice_blocking: enqueue all src_q nodes at the end of dest_q.
 *
 * Dequeue all nodes from src_q.
 * dest_q must be already initialized.
 * Content written into the node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * Mutual exclusion with cds_wfcq_dequeue_blocking and dequeue lock is
 * ensured.
 *
 * Returns enum cds_wfcq_ret which indicates the state of the src or
 * dest queue.
 */
extern enum cds_wfcq_ret cds_wfcq_splice_blocking(
		struct cds_wfcq_head *dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		struct cds_wfcq_head *src_q_head,
		struct cds_wfcq_tail *src_q_tail);

/*
 * __cds_wfcq_dequeue_blocking: dequeue a node from a wait-free queue.
 *
 * Content written into the node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * It is valid to reuse and free a dequeued node immediately.
 * Dequeue/splice/iteration mutual exclusion should be ensured by the
 * caller.
 */
extern struct cds_wfcq_node *__cds_wfcq_dequeue_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail);

/*
 * __cds_wfcq_dequeue_with_state_blocking: dequeue with state.
 *
 * Same as __cds_wfcq_dequeue_blocking, but saves whether dequeueing the
 * last node of the queue into state (CDS_WFCQ_STATE_LAST).
 */
extern struct cds_wfcq_node *__cds_wfcq_dequeue_with_state_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		int *state);

/*
 * __cds_wfcq_dequeue_nonblocking: dequeue a node from a wait-free queue.
 *
 * Same as __cds_wfcq_dequeue_blocking, but returns CDS_WFCQ_WOULDBLOCK
 * if it needs to block.
 */
extern struct cds_wfcq_node *__cds_wfcq_dequeue_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail);

/*
 * __cds_wfcq_dequeue_with_state_blocking: dequeue with state.
 *
 * Same as __cds_wfcq_dequeue_nonblocking, but saves whether dequeueing
 * the last node of the queue into state (CDS_WFCQ_STATE_LAST).
 */
extern struct cds_wfcq_node *__cds_wfcq_dequeue_with_state_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		int *state);

/*
 * __cds_wfcq_splice_blocking: enqueue all src_q nodes at the end of dest_q.
 *
 * Dequeue all nodes from src_q.
 * dest_q must be already initialized.
 * Mutual exclusion for src_q should be ensured by the caller as
 * specified in the "Synchronisation table".
 * Returns enum cds_wfcq_ret which indicates the state of the src or
 * dest queue. Never returns CDS_WFCQ_RET_WOULDBLOCK.
 */
extern enum cds_wfcq_ret __cds_wfcq_splice_blocking(
		cds_wfcq_head_ptr_t dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		cds_wfcq_head_ptr_t src_q_head,
		struct cds_wfcq_tail *src_q_tail);

/*
 * __cds_wfcq_splice_nonblocking: enqueue all src_q nodes at the end of dest_q.
 *
 * Same as __cds_wfcq_splice_blocking, but returns
 * CDS_WFCQ_RET_WOULDBLOCK if it needs to block.
 */
extern enum cds_wfcq_ret __cds_wfcq_splice_nonblocking(
		cds_wfcq_head_ptr_t dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		cds_wfcq_head_ptr_t src_q_head,
		struct cds_wfcq_tail *src_q_tail);

/*
 * __cds_wfcq_first_blocking: get first node of a queue, without dequeuing.
 *
 * Content written into the node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * Dequeue/splice/iteration mutual exclusion should be ensured by the
 * caller.
 *
 * Used by for-like iteration macros:
 * __cds_wfcq_for_each_blocking()
 * __cds_wfcq_for_each_blocking_safe()
 *
 * Returns NULL if queue is empty, first node otherwise.
 */
extern struct cds_wfcq_node *__cds_wfcq_first_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail);

/*
 * __cds_wfcq_first_nonblocking: get first node of a queue, without dequeuing.
 *
 * Same as __cds_wfcq_first_blocking, but returns CDS_WFCQ_WOULDBLOCK if
 * it needs to block.
 */
extern struct cds_wfcq_node *__cds_wfcq_first_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail);

/*
 * __cds_wfcq_next_blocking: get next node of a queue, without dequeuing.
 *
 * Content written into the node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * Dequeue/splice/iteration mutual exclusion should be ensured by the
 * caller.
 *
 * Used by for-like iteration macros:
 * __cds_wfcq_for_each_blocking()
 * __cds_wfcq_for_each_blocking_safe()
 *
 * Returns NULL if reached end of queue, non-NULL next queue node
 * otherwise.
 */
extern struct cds_wfcq_node *__cds_wfcq_next_blocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node);

/*
 * __cds_wfcq_next_blocking: get next node of a queue, without dequeuing.
 *
 * Same as __cds_wfcq_next_blocking, but returns CDS_WFCQ_WOULDBLOCK if
 * it needs to block.
 */
extern struct cds_wfcq_node *__cds_wfcq_next_nonblocking(
		cds_wfcq_head_ptr_t head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node);

#endif /* !_LGPL_SOURCE */

/*
 * __cds_wfcq_for_each_blocking: Iterate over all nodes in a queue,
 * without dequeuing them.
 * @head: head of the queue (struct cds_wfcq_head or __cds_wfcq_head pointer).
 * @tail: tail of the queue (struct cds_wfcq_tail pointer).
 * @node: iterator on the queue (struct cds_wfcq_node pointer).
 *
 * Content written into each node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * Dequeue/splice/iteration mutual exclusion should be ensured by the
 * caller.
 */
#define __cds_wfcq_for_each_blocking(head, tail, node)		\
	for (node = __cds_wfcq_first_blocking(head, tail);	\
		node != NULL;					\
		node = __cds_wfcq_next_blocking(head, tail, node))

/*
 * __cds_wfcq_for_each_blocking_safe: Iterate over all nodes in a queue,
 * without dequeuing them. Safe against deletion.
 * @head: head of the queue (struct cds_wfcq_head or __cds_wfcq_head pointer).
 * @tail: tail of the queue (struct cds_wfcq_tail pointer).
 * @node: iterator on the queue (struct cds_wfcq_node pointer).
 * @n: struct cds_wfcq_node pointer holding the next pointer (used
 *     internally).
 *
 * Content written into each node before enqueue is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 * Dequeue/splice/iteration mutual exclusion should be ensured by the
 * caller.
 */
#define __cds_wfcq_for_each_blocking_safe(head, tail, node, n)		       \
	for (node = __cds_wfcq_first_blocking(head, tail),		       \
			n = (node ? __cds_wfcq_next_blocking(head, tail, node) : NULL); \
		node != NULL;						       \
		node = n, n = (node ? __cds_wfcq_next_blocking(head, tail, node) : NULL))

#ifdef __cplusplus
}

/*
 * In C++, implement static inline wrappers using function overloading
 * to obtain an API similar to C.
 */

static inline cds_wfcq_head_ptr_t cds_wfcq_head_cast_cpp(struct __cds_wfcq_head *head)
{
	cds_wfcq_head_ptr_t ret = { ._h = head };
	return ret;
}

static inline cds_wfcq_head_ptr_t cds_wfcq_head_cast_cpp(struct cds_wfcq_head *head)
{
	cds_wfcq_head_ptr_t ret = { .h = head };
	return ret;
}

static inline cds_wfcq_head_const_ptr_t cds_wfcq_head_const_cast_cpp(const struct __cds_wfcq_head *head)
{
	cds_wfcq_head_const_ptr_t ret = { ._h = head };
	return ret;
}

static inline cds_wfcq_head_const_ptr_t cds_wfcq_head_const_cast_cpp(const struct cds_wfcq_head *head)
{
	cds_wfcq_head_const_ptr_t ret = { .h = head };
	return ret;
}

template<typename T> static inline bool cds_wfcq_empty(T head,
		const struct cds_wfcq_tail *tail)
{
	return cds_wfcq_empty(cds_wfcq_head_const_cast_cpp(head), tail);
}

template<typename T> static inline bool cds_wfcq_enqueue(T head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node)
{
	return cds_wfcq_enqueue(cds_wfcq_head_cast_cpp(head), tail, node);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_dequeue_blocking(
		T head, struct cds_wfcq_tail *tail)
{
	return __cds_wfcq_dequeue_blocking(cds_wfcq_head_cast_cpp(head), tail);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_dequeue_with_state_blocking(
		T head, struct cds_wfcq_tail *tail, int *state)
{
	return __cds_wfcq_dequeue_with_state_blocking(cds_wfcq_head_cast_cpp(head),
			tail, state);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_dequeue_nonblocking(
		T head, struct cds_wfcq_tail *tail)
{
	return __cds_wfcq_dequeue_nonblocking(cds_wfcq_head_cast_cpp(head), tail);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_dequeue_with_state_nonblocking(
		T head, struct cds_wfcq_tail *tail, int *state)
{
	return __cds_wfcq_dequeue_with_state_nonblocking(cds_wfcq_head_cast_cpp(head),
			tail, state);
}

template<typename T, typename U> static inline enum cds_wfcq_ret __cds_wfcq_splice_blocking(
		T dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		U src_q_head,
		struct cds_wfcq_tail *src_q_tail)
{
	return __cds_wfcq_splice_blocking(cds_wfcq_head_cast_cpp(dest_q_head),
			dest_q_tail,
			cds_wfcq_head_cast_cpp(src_q_head),
			src_q_tail);
}

template<typename T, typename U> static inline enum cds_wfcq_ret __cds_wfcq_splice_nonblocking(
		T dest_q_head,
		struct cds_wfcq_tail *dest_q_tail,
		U *src_q_head,
		struct cds_wfcq_tail *src_q_tail)
{
	return __cds_wfcq_splice_nonblocking(cds_wfcq_head_cast_cpp(dest_q_head),
			dest_q_tail,
			cds_wfcq_head_cast_cpp(src_q_head),
			src_q_tail);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_first_blocking(
		T head, struct cds_wfcq_tail *tail)
{
	return __cds_wfcq_first_blocking(cds_wfcq_head_cast_cpp(head), tail);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_first_nonblocking(
		T head, struct cds_wfcq_tail *tail)
{
	return __cds_wfcq_first_nonblocking(cds_wfcq_head_cast_cpp(head), tail);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_next_blocking(
		T head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node)
{
	return __cds_wfcq_next_blocking(cds_wfcq_head_cast_cpp(head), tail, node);
}

template<typename T> static inline struct cds_wfcq_node *__cds_wfcq_next_nonblocking(
		T head,
		struct cds_wfcq_tail *tail,
		struct cds_wfcq_node *node)
{
	return __cds_wfcq_next_nonblocking(cds_wfcq_head_cast_cpp(head), tail, node);
}

#endif

#endif /* _URCU_WFCQUEUE_H */
