/*	$NetBSD: barrier.h,v 1.5 2025/01/26 16:25:40 christos Exp $	*/

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

#include <isc/util.h>

#if HAVE_PTHREAD_BARRIER_INIT

#include <pthread.h>

#if ISC_TRACK_PTHREADS_OBJECTS
typedef pthread_barrier_t *isc_barrier_t;
#else
typedef pthread_barrier_t isc_barrier_t;
#endif

#define isc__barrier_init(bp, count)                                \
	{                                                           \
		int _ret = pthread_barrier_init(bp, NULL, count);   \
		PTHREADS_RUNTIME_CHECK(pthread_barrier_init, _ret); \
	}

#define isc__barrier_wait(bp) pthread_barrier_wait(bp)

#define isc__barrier_destroy(bp)                                       \
	{                                                              \
		int _ret = pthread_barrier_destroy(bp);                \
		PTHREADS_RUNTIME_CHECK(pthread_barrier_destroy, _ret); \
	}

#else

#include <uv.h>

#if ISC_TRACK_PTHREADS_OBJECTS
typedef uv_barrier_t *isc_barrier_t;
#else
typedef uv_barrier_t isc_barrier_t;
#endif

#define isc__barrier_init(bp, count)                     \
	{                                                \
		int _ret = uv_barrier_init(bp, count);   \
		UV_RUNTIME_CHECK(uv_barrier_init, _ret); \
	}

#define isc__barrier_wait(bp) uv_barrier_wait(bp)

#define isc__barrier_destroy(bp) uv_barrier_destroy(bp)

#endif

#if ISC_TRACK_PTHREADS_OBJECTS

#define isc_barrier_init(bp, count)            \
	{                                      \
		*bp = malloc(sizeof(**bp));    \
		isc__barrier_init(*bp, count); \
	}
#define isc_barrier_wait(bp) isc__barrier_wait(*bp)
#define isc_barrier_destroy(bp)            \
	{                                  \
		isc__barrier_destroy(*bp); \
		free(*bp);                 \
	}

#else /* ISC_TRACK_PTHREADS_OBJECTS */

#define isc_barrier_init(bp, count) isc__barrier_init(bp, count)
#define isc_barrier_wait(bp)	    isc__barrier_wait(bp)
#define isc_barrier_destroy(bp)	    isc__barrier_destroy(bp)

#endif /* ISC_TRACK_PTHREADS_OBJECTS */
