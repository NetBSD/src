// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_RCULFQUEUE_STATIC_H
#define _URCU_RCULFQUEUE_STATIC_H

/*
 * Userspace RCU library - Lock-Free RCU Queue
 *
 * TO BE INCLUDED ONLY IN LGPL-COMPATIBLE CODE. See rculfqueue.h for linking
 * dynamically with the userspace rcu library.
 */

#include <urcu-call-rcu.h>
#include <urcu/assert.h>
#include <urcu/uatomic.h>
#include <urcu-pointer.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cds_lfq_node_rcu_dummy {
	struct cds_lfq_node_rcu parent;
	struct rcu_head head;
	struct cds_lfq_queue_rcu *q;
};

/*
 * Lock-free RCU queue. Enqueue and dequeue operations hold a RCU read
 * lock to deal with cmpxchg ABA problem. This queue is *not* circular:
 * head points to the oldest node, tail points to the newest node.
 * A dummy node is kept to ensure enqueue and dequeue can always proceed
 * concurrently. Keeping a separate head and tail helps with large
 * queues: enqueue and dequeue can proceed concurrently without
 * wrestling for exclusive access to the same variables.
 *
 * Dequeue retry if it detects that it would be dequeueing the last node
 * (it means a dummy node dequeue-requeue is in progress). This ensures
 * that there is always at least one node in the queue.
 *
 * In the dequeue operation, we internally reallocate the dummy node
 * upon dequeue/requeue and use call_rcu to free the old one after a
 * grace period.
 */

static inline
struct cds_lfq_node_rcu *make_dummy(struct cds_lfq_queue_rcu *q,
				    struct cds_lfq_node_rcu *next)
{
	struct cds_lfq_node_rcu_dummy *dummy;

	dummy = (struct cds_lfq_node_rcu_dummy *)
		malloc(sizeof(struct cds_lfq_node_rcu_dummy));
	urcu_posix_assert(dummy);
	dummy->parent.next = next;
	dummy->parent.dummy = 1;
	dummy->q = q;
	return &dummy->parent;
}

static inline
void free_dummy_cb(struct rcu_head *head)
{
	struct cds_lfq_node_rcu_dummy *dummy =
		caa_container_of(head, struct cds_lfq_node_rcu_dummy, head);
	free(dummy);
}

static inline
void rcu_free_dummy(struct cds_lfq_node_rcu *node)
{
	struct cds_lfq_node_rcu_dummy *dummy;

	urcu_posix_assert(node->dummy);
	dummy = caa_container_of(node, struct cds_lfq_node_rcu_dummy, parent);
	dummy->q->queue_call_rcu(&dummy->head, free_dummy_cb);
}

static inline
void free_dummy(struct cds_lfq_node_rcu *node)
{
	struct cds_lfq_node_rcu_dummy *dummy;

	urcu_posix_assert(node->dummy);
	dummy = caa_container_of(node, struct cds_lfq_node_rcu_dummy, parent);
	free(dummy);
}

static inline
void _cds_lfq_node_init_rcu(struct cds_lfq_node_rcu *node)
{
	node->next = NULL;
	node->dummy = 0;
}

static inline
void _cds_lfq_init_rcu(struct cds_lfq_queue_rcu *q,
		       void queue_call_rcu(struct rcu_head *head,
				void (*func)(struct rcu_head *head)))
{
	q->tail = make_dummy(q, NULL);
	q->head = q->tail;
	q->queue_call_rcu = queue_call_rcu;
}

/*
 * The queue should be emptied before calling destroy.
 *
 * Return 0 on success, -EPERM if queue is not empty.
 */
static inline
int _cds_lfq_destroy_rcu(struct cds_lfq_queue_rcu *q)
{
	struct cds_lfq_node_rcu *head;

	head = rcu_dereference(q->head);
	if (!(head->dummy && head->next == NULL))
		return -EPERM;	/* not empty */
	free_dummy(head);
	return 0;
}

/*
 * Should be called under rcu read lock critical section.
 */
static inline
void _cds_lfq_enqueue_rcu(struct cds_lfq_queue_rcu *q,
			  struct cds_lfq_node_rcu *node)
{
	/*
	 * uatomic_cmpxchg() implicit memory barrier orders earlier stores to
	 * node before publication.
	 */
	for (;;) {
		struct cds_lfq_node_rcu *tail, *next;

		tail = rcu_dereference(q->tail);
		cmm_emit_legacy_smp_mb();
		next = uatomic_cmpxchg_mo(&tail->next, NULL, node,
					CMM_SEQ_CST, CMM_SEQ_CST);
		if (next == NULL) {
			/*
			 * Tail was at the end of queue, we successfully
			 * appended to it. Now move tail (another
			 * enqueue might beat us to it, that's fine).
			 */
			(void) uatomic_cmpxchg_mo(&q->tail, tail, node,
						CMM_SEQ_CST, CMM_SEQ_CST);
			return;
		} else {
			/*
			 * Failure to append to current tail.
			 * Help moving tail further and retry.
			 */
			(void) uatomic_cmpxchg_mo(&q->tail, tail, next,
						CMM_SEQ_CST, CMM_SEQ_CST);
			continue;
		}
	}
}

static inline
void enqueue_dummy(struct cds_lfq_queue_rcu *q)
{
	struct cds_lfq_node_rcu *node;

	/* We need to reallocate to protect from ABA. */
	node = make_dummy(q, NULL);
	_cds_lfq_enqueue_rcu(q, node);
}

/*
 * Should be called under rcu read lock critical section.
 *
 * The caller must wait for a grace period to pass before freeing the returned
 * node or modifying the cds_lfq_node_rcu structure.
 * Returns NULL if queue is empty.
 */
static inline
struct cds_lfq_node_rcu *_cds_lfq_dequeue_rcu(struct cds_lfq_queue_rcu *q)
{
	for (;;) {
		struct cds_lfq_node_rcu *head, *next;

		head = rcu_dereference(q->head);
		next = rcu_dereference(head->next);
		if (head->dummy && next == NULL)
			return NULL;	/* empty */
		/*
		 * We never, ever allow dequeue to get to a state where
		 * the queue is empty (we need at least one node in the
		 * queue). This is ensured by checking if the head next
		 * is NULL, which means we need to enqueue a dummy node
		 * before we can hope dequeuing anything.
		 */
		if (!next) {
			enqueue_dummy(q);
			next = rcu_dereference(head->next);
		}
		if (uatomic_cmpxchg_mo(&q->head, head, next,
					CMM_SEQ_CST, CMM_SEQ_CST) != head)
			continue;	/* Concurrently pushed. */
		if (head->dummy) {
			/* Free dummy after grace period. */
			rcu_free_dummy(head);
			continue;	/* try again */
		}
		return head;
	}
}

#ifdef __cplusplus
}
#endif

#endif /* _URCU_RCULFQUEUE_STATIC_H */
