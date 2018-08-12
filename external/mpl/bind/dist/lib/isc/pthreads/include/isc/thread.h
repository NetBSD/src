/*	$NetBSD: thread.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISC_THREAD_H
#define ISC_THREAD_H 1

/*! \file */

#include <pthread.h>

#if defined(HAVE_PTHREAD_NP_H)
#include <pthread_np.h>
#endif

#include <isc/lang.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

typedef pthread_t isc_thread_t;
typedef void * isc_threadresult_t;
typedef void * isc_threadarg_t;
typedef isc_threadresult_t (*isc_threadfunc_t)(isc_threadarg_t);
typedef pthread_key_t isc_thread_key_t;

isc_result_t
isc_thread_create(isc_threadfunc_t, isc_threadarg_t, isc_thread_t *);

void
isc_thread_setconcurrency(unsigned int level);

void
isc_thread_yield(void);

void
isc_thread_setname(isc_thread_t thread, const char *name);

/* XXX We could do fancier error handling... */

#define isc_thread_join(t, rp) \
	((pthread_join((t), (rp)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#define isc_thread_self \
	(unsigned long)pthread_self

#define isc_thread_key_create pthread_key_create
#define isc_thread_key_getspecific pthread_getspecific
#define isc_thread_key_setspecific pthread_setspecific
#define isc_thread_key_delete pthread_key_delete

ISC_LANG_ENDDECLS

#endif /* ISC_THREAD_H */
