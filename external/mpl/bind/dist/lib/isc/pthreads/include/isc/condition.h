/*	$NetBSD: condition.h,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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


#ifndef ISC_CONDITION_H
#define ISC_CONDITION_H 1

/*! \file */

#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/result.h>
#include <isc/types.h>

typedef pthread_cond_t isc_condition_t;

#define isc_condition_init(cp) \
	((pthread_cond_init((cp), NULL) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#if ISC_MUTEX_PROFILE
#define isc_condition_wait(cp, mp) \
	((pthread_cond_wait((cp), &((mp)->mutex)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)
#else
#define isc_condition_wait(cp, mp) \
	((pthread_cond_wait((cp), (mp)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)
#endif

#define isc_condition_signal(cp) \
	((pthread_cond_signal((cp)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#define isc_condition_broadcast(cp) \
	((pthread_cond_broadcast((cp)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#define isc_condition_destroy(cp) \
	((pthread_cond_destroy((cp)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)

ISC_LANG_BEGINDECLS

isc_result_t
isc_condition_waituntil(isc_condition_t *, isc_mutex_t *, isc_time_t *);

ISC_LANG_ENDDECLS

#endif /* ISC_CONDITION_H */
