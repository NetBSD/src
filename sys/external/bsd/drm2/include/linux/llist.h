/*	$NetBSD: llist.h,v 1.2 2021/12/19 00:54:54 riastradh Exp $	*/

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
	struct llist_node	*llh_first;
};

struct llist_node {
	struct llist_node	*llh_next;
};

static inline void
init_llist_head(struct llist_head *head)
{

	head->llh_first = NULL;
}

#define	llist_entry(NODE, TYPE, FIELD)	container_of(NODE, TYPE, FIELD)

static inline bool
llist_empty(struct llist_head *head)
{
	bool empty;

	empty = (head->llh_first == NULL);
	membar_enter();

	return empty;
}

static inline bool
llist_add(struct llist_node *node, struct llist_head *head)
{
	struct llist_node *first;

	do {
		first = head->llh_first;
		node->llh_next = first;
		membar_exit();
	} while (atomic_cas_ptr(&head->llh_first, first, node) != first);

	return first == NULL;
}

static inline struct llist_node *
llist_del_all(struct llist_head *head)
{
	struct llist_node *first;

	first = atomic_swap_ptr(&head->llh_first, NULL);
	membar_enter();

	return first;
}

static inline struct llist_node *
llist_del_first(struct llist_head *head)
{
	struct llist_node *first;

	do {
		first = head->llh_first;
		membar_datadep_consumer();
	} while (atomic_cas_ptr(&head->llh_first, first, first->llh_next)
	    != first);
	membar_enter();

	return first;
}

#define	llist_for_each_entry_safe(ENTRY, TMP, NODE, FIELD)		      \
	for ((ENTRY) = ((NODE) == NULL ? NULL :				      \
			llist_entry(NODE, typeof(*(ENTRY)), FIELD));	      \
		(ENTRY) == NULL ? 0 :					      \
		    (membar_datadep_consumer(),				      \
			(TMP) = list_entry((ENTRY)->FIELD.llh_next,	      \
			    typeof(*(ENTRY)), FIELD),			      \
			1);						      \
		 (ENTRY) = (TMP))

#endif	/* _LINUX_LLIST_H_ */
