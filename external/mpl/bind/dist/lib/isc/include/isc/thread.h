/*	$NetBSD: thread.h,v 1.4 2025/01/26 16:25:43 christos Exp $	*/

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

#if HAVE_THREADS_H
#include <threads.h>
#else
#define thread_local _Thread_local
#endif

#if defined(HAVE_PTHREAD_NP_H)
#include <pthread_np.h>
#endif /* if defined(HAVE_PTHREAD_NP_H) */

#include <isc/lang.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

typedef pthread_t isc_thread_t;
typedef void *(*isc_threadfunc_t)(void *);

/*%
 * like isc_thread_create(), but run the function on the current
 * thread which must be the main thread.
 */
void
isc_thread_main(isc_threadfunc_t, void *);

void
isc_thread_create(isc_threadfunc_t, void *, isc_thread_t *);

void
isc_thread_join(isc_thread_t thread, void **);

void
isc_thread_yield(void);

void
isc_thread_setname(isc_thread_t thread, const char *name);

#define isc_thread_self (uintptr_t)pthread_self

ISC_LANG_ENDDECLS
