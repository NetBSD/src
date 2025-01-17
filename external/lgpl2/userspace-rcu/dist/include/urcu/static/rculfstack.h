// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_RCULFSTACK_STATIC_H
#define _URCU_RCULFSTACK_STATIC_H

/*
 * Userspace RCU library - Lock-Free RCU Stack
 *
 * TO BE INCLUDED ONLY IN LGPL-COMPATIBLE CODE. See rculfstack.h for linking
 * dynamically with the userspace rcu library.
 */

#include <urcu/uatomic.h>
#include <urcu-pointer.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline
void _cds_lfs_node_init_rcu(struct cds_lfs_node_rcu *node __attribute__((unused)))
{
}

static inline
void _cds_lfs_init_rcu(struct cds_lfs_stack_rcu *s)
{
	s->head = NULL;
}

/*
 * Lock-free stack push is not subject to ABA problem, so no need to
 * take the RCU read-side lock. Even if "head" changes between two
 * uatomic_cmpxchg() invocations here (being popped, and then pushed
 * again by one or more concurrent threads), the second
 * uatomic_cmpxchg() invocation only cares about pushing a new entry at
 * the head of the stack, ensuring consistency by making sure the new
 * node->next is the same pointer value as the value replaced as head.
 * It does not care about the content of the actual next node, so it can
 * very well be reallocated between the two uatomic_cmpxchg().
 *
 * We take the approach of expecting the stack to be usually empty, so
 * we first try an initial uatomic_cmpxchg() on a NULL old_head, and
 * retry if the old head was non-NULL (the value read by the first
 * uatomic_cmpxchg() is used as old head for the following loop). The
 * upside of this scheme is to minimize the amount of cacheline traffic,
 * always performing an exclusive cacheline access, rather than doing
 * non-exclusive followed by exclusive cacheline access (which would be
 * required if we first read the old head value). This design decision
 * might be revisited after more thorough benchmarking on various
 * platforms.
 *
 * Returns 0 if the stack was empty prior to adding the node.
 * Returns non-zero otherwise.
 */
static inline
int _cds_lfs_push_rcu(struct cds_lfs_stack_rcu *s,
			  struct cds_lfs_node_rcu *node)
{
	struct cds_lfs_node_rcu *head = NULL;

	for (;;) {
		struct cds_lfs_node_rcu *old_head = head;

		node->next = head;
		/*
		 * uatomic_cmpxchg() implicit memory barrier orders earlier
		 * stores to node before publication.
		 */
		cmm_emit_legacy_smp_mb();
		head = uatomic_cmpxchg_mo(&s->head, old_head, node,
					CMM_SEQ_CST, CMM_SEQ_CST);
		if (old_head == head)
			break;
	}
	return (int) !!((unsigned long) head);
}

/*
 * Should be called under rcu read-side lock.
 *
 * The caller must wait for a grace period to pass before freeing the returned
 * node or modifying the cds_lfs_node_rcu structure.
 * Returns NULL if stack is empty.
 */
static inline
struct cds_lfs_node_rcu *
_cds_lfs_pop_rcu(struct cds_lfs_stack_rcu *s)
{
	for (;;) {
		struct cds_lfs_node_rcu *head;

		head = rcu_dereference(s->head);
		if (head) {
			struct cds_lfs_node_rcu *next = rcu_dereference(head->next);

			if (uatomic_cmpxchg_mo(&s->head, head, next,
						CMM_SEQ_CST, CMM_SEQ_CST) == head) {
				cmm_emit_legacy_smp_mb();
				return head;
			} else {
				/* Concurrent modification. Retry. */
				continue;
			}
		} else {
			/* Empty stack */
			return NULL;
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif /* _URCU_RCULFSTACK_STATIC_H */
