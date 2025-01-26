/*	$NetBSD: mutex.h,v 1.3 2025/01/26 16:25:41 christos Exp $	*/

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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/lang.h>
#include <isc/result.h> /* for ISC_R_ codes */
#include <isc/util.h>

ISC_LANG_BEGINDECLS

/*
 * We use macros instead of static inline functions so that the exact code
 * location can be reported when PTHREADS_RUNTIME_CHECK() fails or when mutrace
 * reports lock contention.
 */

#ifdef ISC_TRACK_PTHREADS_OBJECTS

typedef pthread_mutex_t *isc_mutex_t;

#define isc_mutex_init(mp)                  \
	{                                   \
		*mp = malloc(sizeof(**mp)); \
		isc__mutex_init(*mp);       \
	}
#define isc_mutex_lock(mp)    isc__mutex_lock(*mp)
#define isc_mutex_unlock(mp)  isc__mutex_unlock(*mp)
#define isc_mutex_trylock(mp) isc__mutex_trylock(*mp)
#define isc_mutex_destroy(mp)            \
	{                                \
		isc__mutex_destroy(*mp); \
		free(*mp);               \
	}

#else /* ISC_TRACK_PTHREADS_OBJECTS */

typedef pthread_mutex_t isc_mutex_t;

#define isc_mutex_init(mp)    isc__mutex_init(mp)
#define isc_mutex_lock(mp)    isc__mutex_lock(mp)
#define isc_mutex_unlock(mp)  isc__mutex_unlock(mp)
#define isc_mutex_trylock(mp) isc__mutex_trylock(mp)
#define isc_mutex_destroy(mp) isc__mutex_destroy(mp)

#endif /* ISC_TRACK_PTHREADS_OBJECTS */

extern pthread_mutexattr_t isc__mutex_init_attr;

#define isc__mutex_init(mp)                                               \
	{                                                                 \
		int _ret = pthread_mutex_init(mp, &isc__mutex_init_attr); \
		PTHREADS_RUNTIME_CHECK(pthread_mutex_init, _ret);         \
	}

#define isc__mutex_lock(mp)                                       \
	{                                                         \
		int _ret = pthread_mutex_lock(mp);                \
		PTHREADS_RUNTIME_CHECK(pthread_mutex_lock, _ret); \
	}

#define isc__mutex_unlock(mp)                                       \
	{                                                           \
		int _ret = pthread_mutex_unlock(mp);                \
		PTHREADS_RUNTIME_CHECK(pthread_mutex_unlock, _ret); \
	}

#define isc__mutex_trylock(mp) \
	((pthread_mutex_trylock(mp) == 0) ? ISC_R_SUCCESS : ISC_R_LOCKBUSY)

#define isc__mutex_destroy(mp)                                       \
	{                                                            \
		int _ret = pthread_mutex_destroy(mp);                \
		PTHREADS_RUNTIME_CHECK(pthread_mutex_destroy, _ret); \
	}

ISC_LANG_ENDDECLS
