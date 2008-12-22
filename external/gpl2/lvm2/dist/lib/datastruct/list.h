/*	$NetBSD: list.h,v 1.1.1.1 2008/12/22 00:17:54 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_LIST_H
#define _LVM_LIST_H

#include <assert.h>

/*
 * A list consists of a list head plus elements.
 * Each element has 'next' and 'previous' pointers.
 * The list head's pointers point to the first and the last element.
 */

struct dm_list {
	struct dm_list *n, *p;
};

/*
 * Initialise a list before use.
 * The list head's next and previous pointers point back to itself.
 */
#define DM_LIST_INIT(name)	struct dm_list name = { &(name), &(name) }
void dm_list_init(struct dm_list *head);

/*
 * Insert an element before 'head'.
 * If 'head' is the list head, this adds an element to the end of the list.
 */
void dm_list_add(struct dm_list *head, struct dm_list *elem);

/*
 * Insert an element after 'head'.
 * If 'head' is the list head, this adds an element to the front of the list.
 */
void dm_list_add_h(struct dm_list *head, struct dm_list *elem);

/*
 * Delete an element from its list.
 * Note that this doesn't change the element itself - it may still be safe
 * to follow its pointers.
 */
void dm_list_del(struct dm_list *elem);

/*
 * Remove an element from existing list and insert before 'head'.
 */
void dm_list_move(struct dm_list *head, struct dm_list *elem);

/*
 * Is the list empty?
 */
int dm_list_empty(const struct dm_list *head);

/*
 * Is this the first element of the list?
 */
int dm_list_start(const struct dm_list *head, const struct dm_list *elem);

/*
 * Is this the last element of the list?
 */
int dm_list_end(const struct dm_list *head, const struct dm_list *elem);

/*
 * Return first element of the list or NULL if empty
 */
struct dm_list *dm_list_first(const struct dm_list *head);

/*
 * Return last element of the list or NULL if empty
 */
struct dm_list *dm_list_last(const struct dm_list *head);

/*
 * Return the previous element of the list, or NULL if we've reached the start.
 */
struct dm_list *dm_list_prev(const struct dm_list *head, const struct dm_list *elem);

/*
 * Return the next element of the list, or NULL if we've reached the end.
 */
struct dm_list *dm_list_next(const struct dm_list *head, const struct dm_list *elem);

/*
 * Given the address v of an instance of 'struct dm_list' called 'head' 
 * contained in a structure of type t, return the containing structure.
 */
#define dm_list_struct_base(v, t, head) \
    ((t *)((uintptr_t)(v) - (uintptr_t)&((t *) 0)->head))

/*
 * Given the address v of an instance of 'struct dm_list list' contained in
 * a structure of type t, return the containing structure.
 */
#define dm_list_item(v, t) dm_list_struct_base((v), t, list)

/*
 * Given the address v of one known element e in a known structure of type t,
 * return another element f.
 */
#define dm_struct_field(v, t, e, f) \
    (((t *)((uintptr_t)(v) - (uintptr_t)&((t *) 0)->e))->f)

/*
 * Given the address v of a known element e in a known structure of type t,
 * return the list head 'list'
 */
#define dm_list_head(v, t, e) dm_struct_field(v, t, e, list)

/*
 * Set v to each element of a list in turn.
 */
#define dm_list_iterate(v, head) \
	for (v = (head)->n; v != head; v = v->n)

/*
 * Set v to each element in a list in turn, starting from the element 
 * in front of 'start'.
 * You can use this to 'unwind' a list_iterate and back out actions on
 * already-processed elements.
 * If 'start' is 'head' it walks the list backwards.
 */
#define dm_list_uniterate(v, head, start) \
	for (v = (start)->p; v != head; v = v->p)

/*
 * A safe way to walk a list and delete and free some elements along
 * the way.
 * t must be defined as a temporary variable of the same type as v.
 */
#define dm_list_iterate_safe(v, t, head) \
	for (v = (head)->n, t = v->n; v != head; v = t, t = v->n)

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct dm_list' variable within the containing structure is 'field'.
 */
#define dm_list_iterate_items_gen(v, head, field) \
	for (v = dm_list_struct_base((head)->n, typeof(*v), field); \
	     &v->field != (head); \
	     v = dm_list_struct_base(v->field.n, typeof(*v), field))

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct dm_list list' within the containing structure.
 */
#define dm_list_iterate_items(v, head) dm_list_iterate_items_gen(v, (head), list)

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct dm_list' variable within the containing structure is 'field'.
 * t must be defined as a temporary variable of the same type as v.
 */
#define dm_list_iterate_items_gen_safe(v, t, head, field) \
	for (v = dm_list_struct_base((head)->n, typeof(*v), field), \
	     t = dm_list_struct_base(v->field.n, typeof(*v), field); \
	     &v->field != (head); \
	     v = t, t = dm_list_struct_base(v->field.n, typeof(*v), field))
/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct dm_list list' within the containing structure.
 * t must be defined as a temporary variable of the same type as v.
 */
#define dm_list_iterate_items_safe(v, t, head) \
	dm_list_iterate_items_gen_safe(v, t, (head), list)

/*
 * Walk a list backwards, setting 'v' in turn to the containing structure 
 * of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct dm_list' variable within the containing structure is 'field'.
 */
#define dm_list_iterate_back_items_gen(v, head, field) \
	for (v = dm_list_struct_base((head)->p, typeof(*v), field); \
	     &v->field != (head); \
	     v = dm_list_struct_base(v->field.p, typeof(*v), field))

/*
 * Walk a list backwards, setting 'v' in turn to the containing structure 
 * of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct dm_list list' within the containing structure.
 */
#define dm_list_iterate_back_items(v, head) dm_list_iterate_back_items_gen(v, (head), list)

/*
 * Return the number of elements in a list by walking it.
 */
unsigned int dm_list_size(const struct dm_list *head);

#endif
