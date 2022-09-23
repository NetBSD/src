/*	$NetBSD: thread.h,v 1.7 2022/09/23 12:15:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_THREAD_H
#define ISC_THREAD_H 1

#include <inttypes.h>
#include <windows.h>

#include <isc/lang.h>
#include <isc/result.h>

extern __declspec(thread) size_t isc_tid_v;

/*
 * Inlines to help with wait return checking
 */

/* check handle for NULL and INVALID_HANDLE */
inline BOOL
IsValidHandle(HANDLE hHandle) {
	return ((hHandle != NULL) && (hHandle != INVALID_HANDLE_VALUE));
}

/* validate wait return codes... */
inline BOOL
WaitSucceeded(DWORD dwWaitResult, DWORD dwHandleCount) {
	return ((dwWaitResult >= WAIT_OBJECT_0) &&
		(dwWaitResult < WAIT_OBJECT_0 + dwHandleCount));
}

inline BOOL
WaitAbandoned(DWORD dwWaitResult, DWORD dwHandleCount) {
	return ((dwWaitResult >= WAIT_ABANDONED_0) &&
		(dwWaitResult < WAIT_ABANDONED_0 + dwHandleCount));
}

inline BOOL
WaitTimeout(DWORD dwWaitResult) {
	return (dwWaitResult == WAIT_TIMEOUT);
}

inline BOOL
WaitFailed(DWORD dwWaitResult) {
	return (dwWaitResult == WAIT_FAILED);
}

/* compute object indices for waits... */
inline DWORD
WaitSucceededIndex(DWORD dwWaitResult) {
	return (dwWaitResult - WAIT_OBJECT_0);
}

inline DWORD
WaitAbandonedIndex(DWORD dwWaitResult) {
	return (dwWaitResult - WAIT_ABANDONED_0);
}

typedef HANDLE isc_thread_t;
typedef DWORD  isc_threadresult_t;
typedef void  *isc_threadarg_t;
typedef isc_threadresult_t(WINAPI *isc_threadfunc_t)(isc_threadarg_t);

#define isc_thread_self (uintptr_t) GetCurrentThreadId

ISC_LANG_BEGINDECLS

void
isc_thread_create(isc_threadfunc_t, isc_threadarg_t, isc_thread_t *);

void
isc_thread_join(isc_thread_t, isc_threadresult_t *);

void
isc_thread_setconcurrency(unsigned int level);

void
isc_thread_setname(isc_thread_t, const char *);

isc_result_t
isc_thread_setaffinity(int cpu);

#define isc_thread_yield() Sleep(0)

#if HAVE___DECLSPEC_THREAD
#define ISC_THREAD_LOCAL static __declspec(thread)
#else /* if HAVE___DECLSPEC_THREAD */
#error "Thread-local storage support is required!"
#endif /* if HAVE___DECLSPEC_THREAD */

ISC_LANG_ENDDECLS

#endif /* ISC_THREAD_H */
