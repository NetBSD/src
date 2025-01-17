// SPDX-FileCopyrightText: Free Software Foundation, Inc.
// SPDX-FileCopyrightText: 2009 Pierre-Marc Fournier
// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * (originally part of the GNU C Library)
 * Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.
 */

#ifndef _URCU_RCULIST_H
#define _URCU_RCULIST_H

#include <urcu/list.h>
#include <urcu/arch.h>
#include <urcu-pointer.h>

/* Add new element at the head of the list. */
static inline
void cds_list_add_rcu(struct cds_list_head *newp, struct cds_list_head *head)
{
	newp->next = head->next;
	newp->prev = head;
	head->next->prev = newp;
	rcu_assign_pointer(head->next, newp);
}

/* Add new element at the tail of the list. */
static inline
void cds_list_add_tail_rcu(struct cds_list_head *newp,
		struct cds_list_head *head)
{
	newp->next = head;
	newp->prev = head->prev;
	rcu_assign_pointer(head->prev->next, newp);
	head->prev = newp;
}

/*
 * Replace an old entry atomically with respect to concurrent RCU
 * traversal. Mutual exclusion against concurrent updates is required
 * though.
 */
static inline
void cds_list_replace_rcu(struct cds_list_head *old, struct cds_list_head *_new)
{
	_new->next = old->next;
	_new->prev = old->prev;
	rcu_assign_pointer(_new->prev->next, _new);
	_new->next->prev = _new;
}

/* Remove element from list. */
static inline
void cds_list_del_rcu(struct cds_list_head *elem)
{
	elem->next->prev = elem->prev;
	CMM_STORE_SHARED(elem->prev->next, elem->next);
}

/*
 * Iteration through all elements of the list must be done while rcu_read_lock()
 * is held.
 */

/* Iterate forward over the elements of the list.  */
#define cds_list_for_each_rcu(pos, head) \
	for (pos = rcu_dereference((head)->next); (pos) != (head); \
		pos = rcu_dereference((pos)->next))

/* Iterate through elements of the list. */
#define cds_list_for_each_entry_rcu(pos, head, member) \
	for (pos = cds_list_entry(rcu_dereference((head)->next), __typeof__(*(pos)), member); \
		&(pos)->member != (head); \
		pos = cds_list_entry(rcu_dereference((pos)->member.next), __typeof__(*(pos)), member))

#endif	/* _URCU_RCULIST_H */
