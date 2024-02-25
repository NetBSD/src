/*	$NetBSD: thread.h,v 1.2.2.2 2024/02/25 15:47:23 martin Exp $	*/

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
#endif

#if defined(HAVE_PTHREAD_NP_H)
#include <pthread_np.h>
#endif /* if defined(HAVE_PTHREAD_NP_H) */

#include <isc/lang.h>
#include <isc/result.h>

extern thread_local size_t isc_tid_v;

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

ISC_LANG_ENDDECLS
