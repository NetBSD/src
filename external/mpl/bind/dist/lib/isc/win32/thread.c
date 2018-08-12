/*	$NetBSD: thread.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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

#include <config.h>

#include <process.h>

#include <isc/thread.h>
#include <isc/util.h>

isc_result_t
isc_thread_create(isc_threadfunc_t start, isc_threadarg_t arg,
		  isc_thread_t *threadp)
{
	isc_thread_t thread;
	unsigned int id;

	thread = (isc_thread_t)_beginthreadex(NULL, 0, start, arg, 0, &id);
	if (thread == NULL) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}

	*threadp = thread;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_thread_join(isc_thread_t thread, isc_threadresult_t *rp) {
	DWORD result;

	result = WaitForSingleObject(thread, INFINITE);
	if (result != WAIT_OBJECT_0) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}
	if (rp != NULL && !GetExitCodeThread(thread, rp)) {
		/* XXX */
		return (ISC_R_UNEXPECTED);
	}
	(void)CloseHandle(thread);

	return (ISC_R_SUCCESS);
}

void
isc_thread_setconcurrency(unsigned int level) {
	/*
	 * This is unnecessary on Win32 systems, but is here so that the
	 * call exists
	 */
}

void
isc_thread_setname(isc_thread_t thread, const char *name) {
	UNUSED(thread);
	UNUSED(name);
}

void *
isc_thread_key_getspecific(isc_thread_key_t key) {
	return(TlsGetValue(key));
}

int
isc_thread_key_setspecific(isc_thread_key_t key, void *value) {
	return (TlsSetValue(key, value) ? 0 : GetLastError());
}

int
isc_thread_key_create(isc_thread_key_t *key, void (*func)(void *)) {
	*key = TlsAlloc();

	return ((*key != -1) ? 0 : GetLastError());
}

int
isc_thread_key_delete(isc_thread_key_t key) {
	return (TlsFree(key) ? 0 : GetLastError());
}
