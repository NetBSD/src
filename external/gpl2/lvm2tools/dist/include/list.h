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

struct list {
	struct list *n, *p;
};

/*
 * Initialise a list before use.
 * The list head's next and previous pointers point back to itself.
 */
#define LIST_INIT(name)	struct list name = { &(name), &(name) }
void list_init(struct list *head);

/*
 * Insert an element before 'head'.
 * If 'head' is the list head, this adds an element to the end of the list.
 */
void list_add(struct list *head, struct list *elem);

/*
 * Insert an element after 'head'.
 * If 'head' is the list head, this adds an element to the front of the list.
 */
void list_add_h(struct list *head, struct list *elem);

/*
 * Delete an element from its list.
 * Note that this doesn't change the element itself - it may still be safe
 * to follow its pointers.
 */
void list_del(struct list *elem);

/*
 * Remove an element from existing list and insert before 'head'.
 */
void list_move(struct list *head, struct list *elem);

/*
 * Is the list empty?
 */
int list_empty(const struct list *head);

/*
 * Is this the first element of the list?
 */
int list_start(const struct list *head, const struct list *elem);

/*
 * Is this the last element of the list?
 */
int list_end(const struct list *head, const struct list *elem);

/*
 * Return first element of the list or NULL if empty
 */
struct list *list_first(const struct list *head);

/*
 * Return last element of the list or NULL if empty
 */
struct list *list_last(const struct list *head);

/*
 * Return the previous element of the list, or NULL if we've reached the start.
 */
struct list *list_prev(const struct list *head, const struct list *elem);

/*
 * Return the next element of the list, or NULL if we've reached the end.
 */
struct list *list_next(const struct list *head, const struct list *elem);

/*
 * Given the address v of an instance of 'struct list' called 'head' 
 * contained in a structure of type t, return the containing structure.
 */
#define list_struct_base(v, t, head) \
    ((t *)((uintptr_t)(v) - (uintptr_t)&((t *) 0)->head))

/*
 * Given the address v of an instance of 'struct list list' contained in
 * a structure of type t, return the containing structure.
 */
#define list_item(v, t) list_struct_base((v), t, list)

/*
 * Given the address v of one known element e in a known structure of type t,
 * return another element f.
 */
#define struct_field(v, t, e, f) \
    (((t *)((uintptr_t)(v) - (uintptr_t)&((t *) 0)->e))->f)

/*
 * Given the address v of a known element e in a known structure of type t,
 * return the list head 'list'
 */
#define list_head(v, t, e) struct_field(v, t, e, list)

/*
 * Set v to each element of a list in turn.
 */
#define list_iterate(v, head) \
	for (v = (head)->n; v != head; v = v->n)

/*
 * Set v to each element in a list in turn, starting from the element 
 * in front of 'start'.
 * You can use this to 'unwind' a list_iterate and back out actions on
 * already-processed elements.
 * If 'start' is 'head' it walks the list backwards.
 */
#define list_uniterate(v, head, start) \
	for (v = (start)->p; v != head; v = v->p)

/*
 * A safe way to walk a list and delete and free some elements along
 * the way.
 * t must be defined as a temporary variable of the same type as v.
 */
#define list_iterate_safe(v, t, head) \
	for (v = (head)->n, t = v->n; v != head; v = t, t = v->n)

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct list' variable within the containing structure is 'field'.
 */
#define list_iterate_items_gen(v, head, field) \
	for (v = list_struct_base((head)->n, typeof(*v), field); \
	     &v->field != (head); \
	     v = list_struct_base(v->field.n, typeof(*v), field))

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct list list' within the containing structure.
 */
#define list_iterate_items(v, head) list_iterate_items_gen(v, (head), list)

/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct list' variable within the containing structure is 'field'.
 * t must be defined as a temporary variable of the same type as v.
 */
#define list_iterate_items_gen_safe(v, t, head, field) \
	for (v = list_struct_base((head)->n, typeof(*v), field), \
	     t = list_struct_base(v->field.n, typeof(*v), field); \
	     &v->field != (head); \
	     v = t, t = list_struct_base(v->field.n, typeof(*v), field))
/*
 * Walk a list, setting 'v' in turn to the containing structure of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct list list' within the containing structure.
 * t must be defined as a temporary variable of the same type as v.
 */
#define list_iterate_items_safe(v, t, head) \
	list_iterate_items_gen_safe(v, t, (head), list)

/*
 * Walk a list backwards, setting 'v' in turn to the containing structure 
 * of each item.
 * The containing structure should be the same type as 'v'.
 * The 'struct list' variable within the containing structure is 'field'.
 */
#define list_iterate_back_items_gen(v, head, field) \
	for (v = list_struct_base((head)->p, typeof(*v), field); \
	     &v->field != (head); \
	     v = list_struct_base(v->field.p, typeof(*v), field))

/*
 * Walk a list backwards, setting 'v' in turn to the containing structure 
 * of each item.
 * The containing structure should be the same type as 'v'.
 * The list should be 'struct list list' within the containing structure.
 */
#define list_iterate_back_items(v, head) list_iterate_back_items_gen(v, (head), list)

/*
 * Return the number of elements in a list by walking it.
 */
unsigned int list_size(const struct list *head);

#endif
