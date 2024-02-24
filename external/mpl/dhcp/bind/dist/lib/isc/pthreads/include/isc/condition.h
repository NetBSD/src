/*	$NetBSD: condition.h,v 1.1.2.2 2024/02/24 13:07:30 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

/*! \file */

#include <errno.h>

#include <isc/error.h>
#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/result.h>
#include <isc/strerr.h>
#include <isc/string.h>
#include <isc/types.h>

typedef pthread_cond_t isc_condition_t;

#define isc_condition_init(cond)                                \
	if (pthread_cond_init(cond, NULL) != 0) {               \
		char isc_condition_strbuf[ISC_STRERRORSIZE];    \
		strerror_r(errno, isc_condition_strbuf,         \
			   sizeof(isc_condition_strbuf));       \
		isc_error_fatal(__FILE__, __LINE__,             \
				"pthread_cond_init failed: %s", \
				isc_condition_strbuf);          \
	}

#if ISC_MUTEX_PROFILE
#define isc_condition_wait(cp, mp)                                      \
	((pthread_cond_wait((cp), &((mp)->mutex)) == 0) ? ISC_R_SUCCESS \
							: ISC_R_UNEXPECTED)
#else /* if ISC_MUTEX_PROFILE */
#define isc_condition_wait(cp, mp)                            \
	((pthread_cond_wait((cp), (mp)) == 0) ? ISC_R_SUCCESS \
					      : ISC_R_UNEXPECTED)
#endif /* if ISC_MUTEX_PROFILE */

#define isc_condition_signal(cp) \
	((pthread_cond_signal((cp)) == 0) ? ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#define isc_condition_broadcast(cp) \
	((pthread_cond_broadcast((cp)) == 0) ? ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#define isc_condition_destroy(cp) \
	((pthread_cond_destroy((cp)) == 0) ? ISC_R_SUCCESS : ISC_R_UNEXPECTED)

ISC_LANG_BEGINDECLS

isc_result_t
isc_condition_waituntil(isc_condition_t *, isc_mutex_t *, isc_time_t *);

ISC_LANG_ENDDECLS
