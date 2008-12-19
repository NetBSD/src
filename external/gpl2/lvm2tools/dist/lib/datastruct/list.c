/*	$NetBSD: list.c,v 1.2 2008/12/19 15:24:06 haad Exp $	*/

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

#include "lib.h"

/*
 * Initialise a list before use.
 * The list head's next and previous pointers point back to itself.
 */
void dm_list_init(struct dm_list *head)
{
	head->n = head->p = head;
}

/*
 * Insert an element before 'head'.
 * If 'head' is the list head, this adds an element to the end of the list.
 */
void dm_list_add(struct dm_list *head, struct dm_list *elem)
{
	assert(head->n);

	elem->n = head;
	elem->p = head->p;

	head->p->n = elem;
	head->p = elem;
}

/*
 * Insert an element after 'head'.
 * If 'head' is the list head, this adds an element to the front of the list.
 */
void dm_list_add_h(struct dm_list *head, struct dm_list *elem)
{
	assert(head->n);

	elem->n = head->n;
	elem->p = head;

	head->n->p = elem;
	head->n = elem;
}

/*
 * Delete an element from its list.
 * Note that this doesn't change the element itself - it may still be safe
 * to follow its pointers.
 */
void dm_list_del(struct dm_list *elem)
{
	elem->n->p = elem->p;
	elem->p->n = elem->n;
}

/*
 * Remove an element from existing list and insert before 'head'.
 */
void dm_list_move(struct dm_list *head, struct dm_list *elem)
{
        dm_list_del(elem);
        dm_list_add(head, elem);
}

/*
 * Is the list empty?
 */
int dm_list_empty(const struct dm_list *head)
{
	return head->n == head;
}

/*
 * Is this the first element of the list?
 */
int dm_list_start(const struct dm_list *head, const struct dm_list *elem)
{
	return elem->p == head;
}

/*
 * Is this the last element of the list?
 */
int dm_list_end(const struct dm_list *head, const struct dm_list *elem)
{
	return elem->n == head;
}

/*
 * Return first element of the list or NULL if empty
 */
struct dm_list *dm_list_first(const struct dm_list *head)
{
	return (dm_list_empty(head) ? NULL : head->n);
}

/*
 * Return last element of the list or NULL if empty
 */
struct dm_list *dm_list_last(const struct dm_list *head)
{
	return (dm_list_empty(head) ? NULL : head->p);
}

/*
 * Return the previous element of the list, or NULL if we've reached the start.
 */
struct dm_list *dm_list_prev(const struct dm_list *head, const struct dm_list *elem)
{
	return (dm_list_start(head, elem) ? NULL : elem->p);
}

/*
 * Return the next element of the list, or NULL if we've reached the end.
 */
struct dm_list *dm_list_next(const struct dm_list *head, const struct dm_list *elem)
{
	return (dm_list_end(head, elem) ? NULL : elem->n);
}

/*
 * Return the number of elements in a list by walking it.
 */
unsigned int dm_list_size(const struct dm_list *head)
{
	unsigned int s = 0;
	const struct dm_list *v;

	dm_list_iterate(v, head)
	    s++;

	return s;
}
/*	$NetBSD: list.c,v 1.2 2008/12/19 15:24:06 haad Exp $	*/

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

#include "lib.h"

/*
 * Initialise a list before use.
 * The list head's next and previous pointers point back to itself.
 */
void dm_list_init(struct dm_list *head)
{
	head->n = head->p = head;
}

/*
 * Insert an element before 'head'.
 * If 'head' is the list head, this adds an element to the end of the list.
 */
void dm_list_add(struct dm_list *head, struct dm_list *elem)
{
	assert(head->n);

	elem->n = head;
	elem->p = head->p;

	head->p->n = elem;
	head->p = elem;
}

/*
 * Insert an element after 'head'.
 * If 'head' is the list head, this adds an element to the front of the list.
 */
void dm_list_add_h(struct dm_list *head, struct dm_list *elem)
{
	assert(head->n);

	elem->n = head->n;
	elem->p = head;

	head->n->p = elem;
	head->n = elem;
}

/*
 * Delete an element from its list.
 * Note that this doesn't change the element itself - it may still be safe
 * to follow its pointers.
 */
void dm_list_del(struct dm_list *elem)
{
	elem->n->p = elem->p;
	elem->p->n = elem->n;
}

/*
 * Remove an element from existing list and insert before 'head'.
 */
void dm_list_move(struct dm_list *head, struct dm_list *elem)
{
        dm_list_del(elem);
        dm_list_add(head, elem);
}

/*
 * Is the list empty?
 */
int dm_list_empty(const struct dm_list *head)
{
	return head->n == head;
}

/*
 * Is this the first element of the list?
 */
int dm_list_start(const struct dm_list *head, const struct dm_list *elem)
{
	return elem->p == head;
}

/*
 * Is this the last element of the list?
 */
int dm_list_end(const struct dm_list *head, const struct dm_list *elem)
{
	return elem->n == head;
}

/*
 * Return first element of the list or NULL if empty
 */
struct dm_list *dm_list_first(const struct dm_list *head)
{
	return (dm_list_empty(head) ? NULL : head->n);
}

/*
 * Return last element of the list or NULL if empty
 */
struct dm_list *dm_list_last(const struct dm_list *head)
{
	return (dm_list_empty(head) ? NULL : head->p);
}

/*
 * Return the previous element of the list, or NULL if we've reached the start.
 */
struct dm_list *dm_list_prev(const struct dm_list *head, const struct dm_list *elem)
{
	return (dm_list_start(head, elem) ? NULL : elem->p);
}

/*
 * Return the next element of the list, or NULL if we've reached the end.
 */
struct dm_list *dm_list_next(const struct dm_list *head, const struct dm_list *elem)
{
	return (dm_list_end(head, elem) ? NULL : elem->n);
}

/*
 * Return the number of elements in a list by walking it.
 */
unsigned int dm_list_size(const struct dm_list *head)
{
	unsigned int s = 0;
	const struct dm_list *v;

	dm_list_iterate(v, head)
	    s++;

	return s;
}
