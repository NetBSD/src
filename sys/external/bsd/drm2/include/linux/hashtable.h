/*	$OpenBSD$	*/
/*
 * Copyright (c) 2013, 2014, 2015 Mark Kettenis
 * Copyright (c) 2017 Martin Pieuchot
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _LINUX_HASHTABLE_H_
#define _LINUX_HASHTABLE_H_

#include <linux/list.h>

#define DECLARE_HASHTABLE(name, bits) struct list_head name[1 << (bits)]

static inline void
__hash_init(struct list_head *table, u_int size)
{
	u_int i;

	for (i = 0; i < size; i++)
		INIT_LIST_HEAD(&table[i]);
}

static inline bool
__hash_empty(struct list_head *table, u_int size)
{
	u_int i;

	for (i = 0; i < size; i++) {
		if (!list_empty(&table[i]))
			return false;
	}

	return true;
}

#define __hash(table, key)	&table[key % (nitems(table) - 1)]

#define hash_init(table)	__hash_init(table, nitems(table))
#define hash_add(table, node, key) \
	hlist_add_head(node, __hash(table, key))
#define hash_del(node)		hlist_del_init(node)
#define hash_empty(table)	__hash_empty(table, nitems(table))
#define hash_for_each_possible(table, obj, member, key) \
	hlist_for_each_entry(obj, __hash(table, key), member)
#define hash_for_each_safe(table, i, tmp, obj, member) 	\
	for (i = 0; i < nitems(table); i++)		\
	       list_for_each_entry_safe(obj, tmp, &table[i], member)

#endif /*_LINUX_HASHTABLE_H_*/
