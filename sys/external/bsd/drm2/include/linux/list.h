/*	$NetBSD: list.h,v 1.1.2.3 2013/07/24 01:55:44 riastradh Exp $	*/

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

#ifndef _LINUX_LIST_H_
#define _LINUX_LIST_H_

#include <sys/null.h>
#include <sys/queue.h>

#include <linux/kernel.h>

/*
 * Doubly-linked lists.
 */

struct list_head {
	struct list_head *lh_prev;
	struct list_head *lh_next;
};

static inline struct list_head *
list_first(struct list_head *head)
{
	return head->lh_next;
}

static inline struct list_head *
list_next(struct list_head *node)
{
	return node->lh_next;
}

#define	list_entry(PTR, TYPE, FIELD)	container_of(PTR, TYPE, FIELD)

#define	list_for_each(VAR, HEAD)					\
	for ((VAR) = list_first((HEAD));				\
		(VAR) != NULL;						\
		(VAR) = list_next((VAR))

#define	list_for_each_safe(VAR, NEXT, HEAD)				\
	for ((VAR) = list_first((HEAD));				\
		((VAR) != NULL) && ((NEXT) = list_next((VAR)), 1);	\
		(VAR) = (NEXT))

#define	list_for_each_entry(VAR, HEAD, FIELD)				\
	for ((VAR) = ((list_first((HEAD)) == NULL)? NULL :		\
		    list_entry(list_first((HEAD)), typeof(*(VAR)), FIELD)); \
		(VAR) != NULL;						\
		(VAR) = ((list_next(&(VAR)->FIELD) == NULL)? NULL :	\
		    list_entry(list_next(&(VAR)->FIELD), typeof(*(VAR)), \
			FIELD)))

/*
 * `H'ead-only/`H'ash-table doubly-linked lists.
 */

LIST_HEAD(hlist_head, hlist_node);
struct hlist_node {
	LIST_ENTRY(hlist_node) hln_entry;
};

static inline struct hlist_node *
hlist_first(struct hlist_head *head)
{
	return LIST_FIRST(head);
}

static inline struct hlist_node *
hlist_next(struct hlist_node *node)
{
	return LIST_NEXT(node, hln_entry);
}

#define	hlist_entry(PTR, TYPE, FIELD)	container_of(PTR, TYPE, FIELD)
#define	hlist_for_each(VAR, HEAD)	LIST_FOREACH(VAR, HEAD, hln_entry)
#define	hlist_for_each_safe(VAR, NEXT, HEAD)				\
	LIST_FOREACH_SAFE(VAR, HEAD, hln_entry, NEXT)

#define	hlist_for_each_entry(VAR, HLIST, HEAD, FIELD)			\
	for ((HLIST) = LIST_FIRST((HEAD));				\
		((HLIST) != NULL) &&					\
		    ((VAR) = hlist_entry((HLIST), typeof(*(VAR)), FIELD), 1); \
		(HLIST) = LIST_NEXT((HLIST), hln_entry))

#endif  /* _LINUX_LIST_H_ */
