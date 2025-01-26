/*	$NetBSD: condition.h,v 1.3 2025/01/26 16:25:40 christos Exp $	*/

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
#include <stdlib.h>

#include <isc/error.h>
#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

ISC_LANG_BEGINDECLS

/*
 * We use macros instead of static inline functions so that the exact code
 * location can be reported when PTHREADS_RUNTIME_CHECK() fails or when mutrace
 * reports lock contention.
 */

#ifdef ISC_TRACK_PTHREADS_OBJECTS

typedef pthread_cond_t *isc_condition_t;

#define isc_condition_init(cp)              \
	{                                   \
		*cp = malloc(sizeof(**cp)); \
		isc__condition_init(*cp);   \
	}
#define isc_condition_wait(cp, mp)	   isc__condition_wait(*cp, *mp)
#define isc_condition_waituntil(cp, mp, t) isc__condition_waituntil(*cp, *mp, t)
#define isc_condition_signal(cp)	   isc__condition_signal(*cp)
#define isc_condition_broadcast(cp)	   isc__condition_broadcast(*cp)
#define isc_condition_destroy(cp)            \
	{                                    \
		isc__condition_destroy(*cp); \
		free(*cp);                   \
	}

#else /* ISC_TRACK_PTHREADS_OBJECTS */

typedef pthread_cond_t isc_condition_t;

#define isc_condition_init(cond)	   isc__condition_init(cond)
#define isc_condition_wait(cp, mp)	   isc__condition_wait(cp, mp)
#define isc_condition_waituntil(cp, mp, t) isc__condition_waituntil(cp, mp, t)
#define isc_condition_signal(cp)	   isc__condition_signal(cp)
#define isc_condition_broadcast(cp)	   isc__condition_broadcast(cp)
#define isc_condition_destroy(cp)	   isc__condition_destroy(cp)

#endif /* ISC_TRACK_PTHREADS_OBJECTS */

#define isc__condition_init(cond)                                \
	{                                                        \
		int _ret = pthread_cond_init(cond, NULL);        \
		PTHREADS_RUNTIME_CHECK(pthread_cond_init, _ret); \
	}

#define isc__condition_wait(cp, mp)                              \
	{                                                        \
		int _ret = pthread_cond_wait(cp, mp);            \
		PTHREADS_RUNTIME_CHECK(pthread_cond_wait, _ret); \
	}

#define isc__condition_signal(cp)                                  \
	{                                                          \
		int _ret = pthread_cond_signal(cp);                \
		PTHREADS_RUNTIME_CHECK(pthread_cond_signal, _ret); \
	}

#define isc__condition_broadcast(cp)                                  \
	{                                                             \
		int _ret = pthread_cond_broadcast(cp);                \
		PTHREADS_RUNTIME_CHECK(pthread_cond_broadcast, _ret); \
	}

#define isc__condition_destroy(cp)                                  \
	{                                                           \
		int _ret = pthread_cond_destroy(cp);                \
		PTHREADS_RUNTIME_CHECK(pthread_cond_destroy, _ret); \
	}

isc_result_t
isc__condition_waituntil(pthread_cond_t *, pthread_mutex_t *, isc_time_t *);

ISC_LANG_ENDDECLS
