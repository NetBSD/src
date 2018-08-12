/*	$NetBSD: condition.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


/*
 * This provides a limited subset of the isc_condition_t
 * functionality for use by single-threaded programs that
 * need to block waiting for events.   Only a single
 * call to isc_condition_wait() may be blocked at any given
 * time, and the _waituntil and _broadcast functions are not
 * supported.  This is intended primarily for use by the omapi
 * library, and may go away once omapi goes away.  Use for
 * other purposes is strongly discouraged.
 */

#ifndef ISC_CONDITION_H
#define ISC_CONDITION_H 1

#include <isc/mutex.h>

typedef int isc_condition_t;

isc_result_t isc__nothread_wait_hack(isc_condition_t *cp, isc_mutex_t *mp);
isc_result_t isc__nothread_signal_hack(isc_condition_t *cp);

#define isc_condition_init(cp) \
	(*(cp) = 0, ISC_R_SUCCESS)

#define isc_condition_wait(cp, mp) \
	isc__nothread_wait_hack(cp, mp)

#define isc_condition_waituntil(cp, mp, tp) \
	((void)(cp), (void)(mp), (void)(tp), ISC_R_NOTIMPLEMENTED)

#define isc_condition_signal(cp) \
	isc__nothread_signal_hack(cp)

#define isc_condition_broadcast(cp) \
	((void)(cp), ISC_R_NOTIMPLEMENTED)

#define isc_condition_destroy(cp) \
	(*(cp) == 0 ? (*(cp) = -1, ISC_R_SUCCESS) : ISC_R_UNEXPECTED)

#endif /* ISC_CONDITION_H */
