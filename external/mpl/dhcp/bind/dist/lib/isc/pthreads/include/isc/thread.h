/*	$NetBSD: thread.h,v 1.1.2.2 2024/02/24 13:07:30 martin Exp $	*/

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

#ifndef ISC_THREAD_H
#define ISC_THREAD_H 1

/*! \file */

#include <pthread.h>

#if defined(HAVE_PTHREAD_NP_H)
#include <pthread_np.h>
#endif /* if defined(HAVE_PTHREAD_NP_H) */

#include <isc/lang.h>
#include <isc/result.h>

#if defined(HAVE_THREAD_LOCAL)
#include <threads.h>
extern thread_local size_t isc_tid_v;
#elif defined(HAVE___THREAD)
extern __thread size_t isc_tid_v;
#endif /* if defined(HAVE_THREAD_LOCAL) */

ISC_LANG_BEGINDECLS

typedef pthread_t isc_thread_t;
typedef void	 *isc_threadresult_t;
typedef void	 *isc_threadarg_t;
typedef isc_threadresult_t (*isc_threadfunc_t)(isc_threadarg_t);

void
isc_thread_create(isc_threadfunc_t, isc_threadarg_t, isc_thread_t *);

void
isc_thread_join(isc_thread_t thread, isc_threadresult_t *result);

void
isc_thread_yield(void);

void
isc_thread_setname(isc_thread_t thread, const char *name);

#define isc_thread_self (uintptr_t) pthread_self

/***
 *** Thread-Local Storage
 ***/

#if defined(HAVE_TLS)
#if defined(HAVE_THREAD_LOCAL)
#define ISC_THREAD_LOCAL static thread_local
#elif defined(HAVE___THREAD)
#define ISC_THREAD_LOCAL static __thread
#else /* if defined(HAVE_THREAD_LOCAL) */
#error "Unknown method for defining a TLS variable!"
#endif /* if defined(HAVE_THREAD_LOCAL) */
#else  /* if defined(HAVE_TLS) */
#error "Thread-local storage support is required!"
#endif /* if defined(HAVE_TLS) */

ISC_LANG_ENDDECLS

#endif /* ISC_THREAD_H */
