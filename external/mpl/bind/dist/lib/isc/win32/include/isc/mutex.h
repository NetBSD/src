/*	$NetBSD: mutex.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISC_MUTEX_H
#define ISC_MUTEX_H 1

#include <isc/net.h>
#include <windows.h>

#include <isc/result.h>

typedef CRITICAL_SECTION isc_mutex_t;

/*
 * This definition is here since some versions of WINBASE.H
 * omits it for some reason.
 */
#if (_WIN32_WINNT < 0x0400)
WINBASEAPI BOOL WINAPI
TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
#endif /* _WIN32_WINNT < 0x0400 */

#define isc_mutex_init(mp) \
	(InitializeCriticalSection((mp)), ISC_R_SUCCESS)
#define isc_mutex_lock(mp) \
	(EnterCriticalSection((mp)), ISC_R_SUCCESS)
#define isc_mutex_unlock(mp) \
	(LeaveCriticalSection((mp)), ISC_R_SUCCESS)
#define isc_mutex_trylock(mp) \
	(TryEnterCriticalSection((mp)) ? ISC_R_SUCCESS : ISC_R_LOCKBUSY)
#define isc_mutex_destroy(mp) \
	(DeleteCriticalSection((mp)), ISC_R_SUCCESS)

/*
 * This is a placeholder for now since we are not keeping any mutex stats
 */
#define isc_mutex_stats(fp) do {} while (0)

#endif /* ISC_MUTEX_H */
