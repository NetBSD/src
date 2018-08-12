/*	$NetBSD: dlz_list.h,v 1.1.1.1 2018/08/12 12:07:50 christos Exp $	*/

/*
 * Copyright (C) 1997-2002, 2004, 2006, 2007, 2011-2013, 2016  Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLZ_LIST_H
#define DLZ_LIST_H 1

#define DLZ_LIST(type) struct { type *head, *tail; }
#define DLZ_LIST_INIT(list) \
	do { (list).head = NULL; (list).tail = NULL; } while (0)

#define DLZ_LINK(type) struct { type *prev, *next; }
#define DLZ_LINK_INIT(elt, link) \
	do { \
		(elt)->link.prev = (void *)(-1); \
		(elt)->link.next = (void *)(-1); \
	} while (0)

#define DLZ_LIST_HEAD(list) ((list).head)
#define DLZ_LIST_TAIL(list) ((list).tail)

#define DLZ_LIST_APPEND(list, elt, link) \
	do { \
		if ((list).tail != NULL) \
			(list).tail->link.next = (elt); \
		else \
			(list).head = (elt); \
		(elt)->link.prev = (list).tail; \
		(elt)->link.next = NULL; \
		(list).tail = (elt); \
	} while (0)

#define DLZ_LIST_PREV(elt, link) ((elt)->link.prev)
#define DLZ_LIST_NEXT(elt, link) ((elt)->link.next)

#endif /* DLZ_LIST_H */
