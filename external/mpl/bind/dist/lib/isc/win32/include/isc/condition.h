/*	$NetBSD: condition.h,v 1.7 2022/09/23 12:15:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_CONDITION_H
#define ISC_CONDITION_H 1

#include <windows.h>

#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/thread.h>
#include <isc/types.h>

typedef struct isc_condition_thread isc_condition_thread_t;

struct isc_condition_thread {
	uintptr_t th;
	HANDLE	  handle[2];
	ISC_LINK(isc_condition_thread_t) link;
};

typedef struct isc_condition {
	HANDLE	     events[2];
	unsigned int waiters;
	ISC_LIST(isc_condition_thread_t) threadlist;
} isc_condition_t;

ISC_LANG_BEGINDECLS

void
isc_condition_init(isc_condition_t *);

isc_result_t
isc_condition_wait(isc_condition_t *, isc_mutex_t *);

isc_result_t
isc_condition_signal(isc_condition_t *);

isc_result_t
isc_condition_broadcast(isc_condition_t *);

isc_result_t
isc_condition_destroy(isc_condition_t *);

isc_result_t
isc_condition_waituntil(isc_condition_t *, isc_mutex_t *, isc_time_t *);

ISC_LANG_ENDDECLS

#endif /* ISC_CONDITION_H */
