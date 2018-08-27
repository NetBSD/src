/*	$NetBSD: list.h,v 1.15 2018/08/27 07:33:09 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Notes on porting:
 *
 * - LIST_HEAD(x) means a declaration `struct list_head x =
 *   LIST_HEAD_INIT(x)' in Linux, but something else in NetBSD.
 *   Replace by the expansion.
 *
 * - The `_rcu' routines here are not actually pserialize(9)-safe.
 *   They need dependent read memory barriers added.  Please fix this
 *   if you need to use them with pserialize(9).
 */

#ifndef _LINUX_LIST_H_
#define _LINUX_LIST_H_

#include <sys/null.h>
#include <sys/pslist.h>
#include <sys/queue.h>

#include <linux/kernel.h>

/*
 * Doubly-linked lists.
 */

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

#define	LIST_HEAD_INIT(name)	{ .prev = &(name), .next = &(name) }

static inline void
INIT_LIST_HEAD(struct list_head *head)
{
	head->prev = head;
	head->next = head;
}

static inline struct list_head *
list_first(const struct list_head *head)
{
	return head->next;
}

static inline struct list_head *
list_last(const struct list_head *head)
{
	return head->prev;
}

static inline struct list_head *
list_next(const struct list_head *node)
{
	return node->next;
}

static inline struct list_head *
list_prev(const struct list_head *node)
{
	return node->prev;
}

static inline int
list_empty(const struct list_head *head)
{
	return (head->next == head);
}

static inline int
list_is_singular(const struct list_head *head)
{

	if (list_empty(head))
		return false;
	if (head->next != head->prev)
		return false;
	return true;
}

static inline void
__list_add_between(struct list_head *prev, struct list_head *node,
    struct list_head *next)
{
	prev->next = node;
	node->prev = prev;
	node->next = next;
	next->prev = node;
}

static inline void
list_add(struct list_head *node, struct list_head *head)
{
	__list_add_between(head, node, head->next);
}

static inline void
list_add_tail(struct list_head *node, struct list_head *head)
{
	__list_add_between(head->prev, node, head);
}

static inline void
list_del(struct list_head *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
}

static inline void
__list_splice_between(struct list_head *prev, const struct list_head *list,
    struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void
list_splice(const struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice_between(head, list, head->next);
}

static inline void
list_splice_tail(const struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice_between(head->prev, list, head);
}

static inline void
list_splice_tail_init(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice_between(head->prev, list, head);
		INIT_LIST_HEAD(list);
	}
}

static inline void
list_move(struct list_head *node, struct list_head *head)
{
	list_del(node);
	list_add(node, head);
}

static inline void
list_move_tail(struct list_head *node, struct list_head *head)
{
	list_del(node);
	list_add_tail(node, head);
}

static inline void
list_replace(struct list_head *old, struct list_head *new)
{
	new->prev = old->prev;
	old->prev->next = new;
	new->next = old->next;
	old->next->prev = new;
}

static inline void
list_replace_init(struct list_head *old, struct list_head *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}

static inline void
list_del_init(struct list_head *node)
{
	list_del(node);
	INIT_LIST_HEAD(node);
}

#define	list_entry(PTR, TYPE, FIELD)	container_of(PTR, TYPE, FIELD)
#define	list_first_entry(PTR, TYPE, FIELD)				\
	list_entry(list_first((PTR)), TYPE, FIELD)
#define	list_first_entry_or_null(PTR, TYPE, FIELD)			\
	(list_empty((PTR)) ? NULL : list_entry(list_first((PTR)), TYPE, FIELD))
#define	list_last_entry(PTR, TYPE, FIELD)				\
	list_entry(list_last((PTR)), TYPE, FIELD)
#define	list_next_entry(ENTRY, FIELD)					\
	list_entry(list_next(&(ENTRY)->FIELD), typeof(*(ENTRY)), FIELD)
#define	list_prev_entry(ENTRY, FIELD)					\
	list_entry(list_prev(&(ENTRY)->FIELD), typeof(*(ENTRY)), FIELD)

#define	list_for_each(VAR, HEAD)					\
	for ((VAR) = list_first((HEAD));				\
		(VAR) != (HEAD);					\
		(VAR) = list_next((VAR)))

#define	list_for_each_safe(VAR, NEXT, HEAD)				\
	for ((VAR) = list_first((HEAD));				\
		((VAR) != (HEAD)) && ((NEXT) = list_next((VAR)), 1);	\
		(VAR) = (NEXT))

#define	list_for_each_entry(VAR, HEAD, FIELD)				\
	for ((VAR) = list_entry(list_first((HEAD)), typeof(*(VAR)), FIELD); \
		&(VAR)->FIELD != (HEAD);				\
		(VAR) = list_entry(list_next(&(VAR)->FIELD), typeof(*(VAR)), \
		    FIELD))

#define	list_for_each_entry_reverse(VAR, HEAD, FIELD)			\
	for ((VAR) = list_entry(list_last((HEAD)), typeof(*(VAR)), FIELD); \
		&(VAR)->FIELD != (HEAD);				\
		(VAR) = list_entry(list_prev(&(VAR)->FIELD), typeof(*(VAR)), \
		    FIELD))

#define	list_for_each_entry_safe(VAR, NEXT, HEAD, FIELD)		\
	for ((VAR) = list_entry(list_first((HEAD)), typeof(*(VAR)), FIELD); \
		(&(VAR)->FIELD != (HEAD)) &&				\
		    ((NEXT) = list_entry(list_next(&(VAR)->FIELD),	\
			typeof(*(VAR)), FIELD), 1);			\
		(VAR) = (NEXT))

#define	list_for_each_entry_continue(VAR, HEAD, FIELD)			\
	for ((VAR) = list_next_entry((VAR), FIELD);			\
		&(VAR)->FIELD != (HEAD);				\
		(VAR) = list_next_entry((VAR), FIELD))

#define	list_for_each_entry_continue_reverse(VAR, HEAD, FIELD)		\
	for ((VAR) = list_prev_entry((VAR), FIELD);			\
		&(VAR)->FIELD != (HEAD);				\
		(VAR) = list_prev_entry((VAR), FIELD))

#define	list_for_each_entry_safe_from(VAR, NEXT, HEAD, FIELD)		\
	for (;								\
		(&(VAR)->FIELD != (HEAD)) &&				\
		    ((NEXT) = list_next_entry((VAR), FIELD));		\
		(VAR) = (NEXT))

/*
 * `H'ead-only/`H'ash-table doubly-linked lists.
 */

#define	hlist_head	pslist_head
#define	hlist_node	pslist_entry

#define	HLIST_HEAD_INIT	PSLIST_INITIALIZER
#define	INIT_HLIST_HEAD	PSLIST_INIT
#define	hlist_empty(h)	(pslist_writer_first(h) == NULL)
#define	hlist_first	pslist_writer_first
#define	hlist_next	pslist_writer_next
#define	hlist_add_head(n, h)	pslist_writer_insert_head(h, n)

static inline void
hlist_del(struct hlist_node *node)
{

	pslist_writer_remove(node);
	/* XXX abstraction violation */
	node->ple_prevp = _PSLIST_POISON;
}

static inline void
hlist_del_init(struct hlist_node *node)
{

	/* XXX abstraction violation */
	if (node->ple_prevp != NULL)
		pslist_writer_remove(node);
}

#define	hlist_entry(PTR, TYPE, FIELD)	container_of(PTR, TYPE, FIELD)
#define	hlist_for_each(VAR, HEAD)					      \
	for ((VAR) = hlist_first(HEAD); (VAR) != NULL; (VAR) = hlist_next(VAR))
#define	hlist_for_each_safe(VAR, NEXT, HEAD)				      \
	for ((VAR) = hlist_first(HEAD),					      \
		    (NEXT) = ((VAR) == NULL ? NULL : hlist_next(HEAD));	      \
		(VAR) != NULL;						      \
		(VAR) = (NEXT))
#define	hlist_for_each_entry(VAR, HEAD, FIELD)				      \
	for ((VAR) = (hlist_first(HEAD) == NULL ? NULL :		      \
			hlist_entry(hlist_first(HEAD), typeof(*(VAR)),	      \
			    FIELD));					      \
		(VAR) != NULL;						      \
		(VAR) = (hlist_next(&(VAR)->FIELD) == NULL ? NULL :	      \
			hlist_entry(hlist_next(&(VAR)->FIELD), typeof(*(VAR)),\
			    FIELD)))

#define	hlist_for_each_entry_safe(VAR, NEXT, HEAD, FIELD)		      \
	for ((VAR) = (hlist_first(HEAD) == NULL ? NULL :		      \
			hlist_entry(hlist_first(HEAD), typeof(*(VAR)),	      \
			    FIELD)),					      \
		    (NEXT) = ((VAR) == NULL ? NULL :			      \
			hlist_next(&(VAR)->FIELD));			      \
		(VAR) != NULL;						      \
	        (VAR) = ((NEXT) == NULL ? NULL :			      \
			    hlist_entry((NEXT), typeof(*(VAR)), FIELD)))

#define	hlist_add_behind_rcu(n, p)	pslist_writer_insert_after(p, n)
#define	hlist_add_head_rcu(n, h)	pslist_writer_insert_head(h, n)
#define	hlist_del_init_rcu		hlist_del_init /* no special needs */

#define	hlist_first_rcu		pslist_reader_first
#define	hlist_next_rcu		pslist_reader_next

#define	hlist_for_each_entry_rcu(VAR, HEAD, FIELD)			      \
	for ((VAR) = (hlist_first_rcu(HEAD) == NULL ? NULL :		      \
			hlist_entry(hlist_first_rcu(HEAD), typeof(*(VAR)),    \
			    FIELD));					      \
		(VAR) != NULL;						      \
		(VAR) = (hlist_next_rcu(&(VAR)->FIELD) == NULL ? NULL :	      \
			hlist_entry(hlist_next_rcu(&(VAR)->FIELD),	      \
			    typeof(*(VAR)), FIELD)))

#endif  /* _LINUX_LIST_H_ */
