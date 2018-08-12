/*	$NetBSD: event.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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


/*!
 * \file
 */

#include <config.h>

#include <isc/event.h>
#include <isc/mem.h>
#include <isc/util.h>

/***
 *** Events.
 ***/

static void
destroy(isc_event_t *event) {
	isc_mem_t *mctx = event->ev_destroy_arg;

	isc_mem_put(mctx, event, event->ev_size);
}

isc_event_t *
isc_event_allocate(isc_mem_t *mctx, void *sender, isc_eventtype_t type,
		   isc_taskaction_t action, void *arg, size_t size)
{
	isc_event_t *event;

	REQUIRE(size >= sizeof(struct isc_event));
	REQUIRE(action != NULL);

	event = isc_mem_get(mctx, size);
	if (event == NULL)
		return (NULL);

	ISC_EVENT_INIT(event, size, 0, NULL, type, action, arg,
		       sender, destroy, mctx);

	return (event);
}

isc_event_t *
isc_event_constallocate(isc_mem_t *mctx, void *sender, isc_eventtype_t type,
			isc_taskaction_t action, const void *arg, size_t size)
{
	isc_event_t *event;
	void *deconst_arg;

	REQUIRE(size >= sizeof(struct isc_event));
	REQUIRE(action != NULL);

	event = isc_mem_get(mctx, size);
	if (event == NULL)
		return (NULL);

	/*
	 * Removing the const attribute from "arg" is the best of two
	 * evils here.  If the event->ev_arg member is made const, then
	 * it affects a great many users of the task/event subsystem
	 * which are not passing in an "arg" which starts its life as
	 * const.  Changing isc_event_allocate() and isc_task_onshutdown()
	 * to not have "arg" prototyped as const (which is quite legitimate,
	 * because neither of those functions modify arg) can cause
	 * compiler whining anytime someone does want to use a const
	 * arg that they themselves never modify, such as with
	 * gcc -Wwrite-strings and using a string "arg".
	 */
	DE_CONST(arg, deconst_arg);

	ISC_EVENT_INIT(event, size, 0, NULL, type, action, deconst_arg,
		       sender, destroy, mctx);

	return (event);
}

void
isc_event_free(isc_event_t **eventp) {
	isc_event_t *event;

	REQUIRE(eventp != NULL);
	event = *eventp;
	REQUIRE(event != NULL);

	REQUIRE(!ISC_LINK_LINKED(event, ev_link));
	REQUIRE(!ISC_LINK_LINKED(event, ev_ratelink));

	if (event->ev_destroy != NULL)
		(event->ev_destroy)(event);

	*eventp = NULL;
}
