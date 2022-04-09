/*	$NetBSD: llist.h,v 1.7 2022/04/09 23:43:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_LLIST_H_
#define	_LINUX_LLIST_H_

#include <sys/atomic.h>
#include <sys/null.h>

struct llist_head {
	struct llist_node	*first;
};

struct llist_node {
	struct llist_node	*next;
};

static inline void
init_llist_head(struct llist_head *head)
{

	head->first = NULL;
}

#define	llist_entry(NODE, TYPE, FIELD)	container_of(NODE, TYPE, FIELD)

static inline bool
llist_empty(struct llist_head *head)
{
	bool empty;

	empty = (atomic_load_acquire(&head->first) == NULL);

	return empty;
}

static inline bool
llist_add(struct llist_node *node, struct llist_head *head)
{
	struct llist_node *first;

	do {
		first = head->first;
		node->next = first;
		membar_release();
	} while (atomic_cas_ptr(&head->first, first, node) != first);

	return first == NULL;
}

static inline bool
llist_add_batch(struct llist_node *first, struct llist_node *last,
    struct llist_head *head)
{
	struct llist_node *next;

	do {
		next = atomic_load_consume(&head->first);
		last->next = next;
	} while (atomic_cas_ptr(&head->first, next, first) != next);

	return next == NULL;
}

static inline struct llist_node *
llist_del_all(struct llist_head *head)
{
	struct llist_node *first;

	first = atomic_swap_ptr(&head->first, NULL);
	membar_datadep_consumer();

	return first;
}

static inline struct llist_node *
llist_del_first(struct llist_head *head)
{
	struct llist_node *first;

	do {
		first = atomic_load_consume(&head->first);
		if (first == NULL)
			return NULL;
	} while (atomic_cas_ptr(&head->first, first, first->next)
	    != first);

	return first;
}

#define	_llist_next(ENTRY, FIELD)					      \
({									      \
	__typeof__((ENTRY)->FIELD.next) _NODE =				      \
	    atomic_load_consume(&(ENTRY)->FIELD.next);			      \
	(_NODE == NULL ? NULL :						      \
	    llist_entry(_NODE, __typeof__(*(ENTRY)), FIELD));		      \
})

#define	llist_for_each_safe(NODE, TMP, HEAD)				      \
	for ((NODE) = (HEAD);						      \
		(NODE) && ((TMP) = (NODE)->next, 1);			      \
		(NODE) = (TMP))

#define	llist_for_each_entry(ENTRY, NODE, FIELD)			      \
	for ((ENTRY) = ((NODE) == NULL ? NULL :				      \
		    (membar_datadep_consumer(),				      \
			llist_entry(NODE, typeof(*(ENTRY)), FIELD)));	      \
		(ENTRY) != NULL;					      \
		(ENTRY) = _llist_next(ENTRY, FIELD))

#define	llist_for_each_entry_safe(ENTRY, TMP, NODE, FIELD)		      \
	for ((ENTRY) = ((NODE) == NULL ? NULL :				      \
		    (membar_datadep_consumer(),				      \
			llist_entry(NODE, typeof(*(ENTRY)), FIELD)));	      \
		((ENTRY) == NULL ? 0 :					      \
		    ((TMP) = _llist_next(ENTRY, FIELD), 1));		      \
		(ENTRY) = (TMP))

#endif	/* _LINUX_LLIST_H_ */
