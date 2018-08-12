/*	$NetBSD: event.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


#ifndef ISC_EVENT_H
#define ISC_EVENT_H 1

/*! \file isc/event.h */

#include <isc/lang.h>
#include <isc/types.h>

/*****
 ***** Events.
 *****/

typedef void (*isc_eventdestructor_t)(isc_event_t *);

#define ISC_EVENT_COMMON(ltype)		\
	size_t				ev_size; \
	unsigned int			ev_attributes; \
	void *				ev_tag; \
	isc_eventtype_t			ev_type; \
	isc_taskaction_t		ev_action; \
	void *				ev_arg; \
	void *				ev_sender; \
	isc_eventdestructor_t		ev_destroy; \
	void *				ev_destroy_arg; \
	ISC_LINK(ltype)			ev_link; \
	ISC_LINK(ltype)			ev_ratelink

/*%
 * Attributes matching a mask of 0x000000ff are reserved for the task library's
 * definition.  Attributes of 0xffffff00 may be used by the application
 * or non-ISC libraries.
 */
#define ISC_EVENTATTR_NOPURGE		0x00000001

/*%
 * The ISC_EVENTATTR_CANCELED attribute is intended to indicate
 * that an event is delivered as a result of a canceled operation
 * rather than successful completion, by mutual agreement
 * between the sender and receiver.  It is not set or used by
 * the task system.
 */
#define ISC_EVENTATTR_CANCELED		0x00000002

#define ISC_EVENT_INIT(event, sz, at, ta, ty, ac, ar, sn, df, da) \
do { \
	(event)->ev_size = (sz); \
	(event)->ev_attributes = (at); \
	(event)->ev_tag = (ta); \
	(event)->ev_type = (ty); \
	(event)->ev_action = (ac); \
	(event)->ev_arg = (ar); \
	(event)->ev_sender = (sn); \
	(event)->ev_destroy = (df); \
	(event)->ev_destroy_arg = (da); \
	ISC_LINK_INIT((event), ev_link); \
	ISC_LINK_INIT((event), ev_ratelink); \
} while (/*CONSTCOND*/0)

/*%
 * This structure is public because "subclassing" it may be useful when
 * defining new event types.
 */
struct isc_event {
	ISC_EVENT_COMMON(struct isc_event);
};

#define ISC_EVENTTYPE_FIRSTEVENT	0x00000000
#define ISC_EVENTTYPE_LASTEVENT		0xffffffff

#define ISC_EVENT_PTR(p) ((isc_event_t **)(void *)(p))

ISC_LANG_BEGINDECLS

isc_event_t *
isc_event_allocate(isc_mem_t *mctx, void *sender, isc_eventtype_t type,
		   isc_taskaction_t action, void *arg, size_t size);
isc_event_t *
isc_event_constallocate(isc_mem_t *mctx, void *sender, isc_eventtype_t type,
			isc_taskaction_t action, const void *arg, size_t size);
/*%<
 * Allocate an event structure.
 *
 * Allocate and initialize in a structure with initial elements
 * defined by:
 *
 * \code
 *	struct {
 *		ISC_EVENT_COMMON(struct isc_event);
 *		...
 *	};
 * \endcode
 *
 * Requires:
 *\li	'size' >= sizeof(struct isc_event)
 *\li	'action' to be non NULL
 *
 * Returns:
 *\li	a pointer to a initialized structure of the requested size.
 *\li	NULL if unable to allocate memory.
 */

void
isc_event_free(isc_event_t **);

ISC_LANG_ENDDECLS

#endif /* ISC_EVENT_H */
