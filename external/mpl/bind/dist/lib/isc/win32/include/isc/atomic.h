/*	$NetBSD: atomic.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISC_ATOMIC_H
#define ISC_ATOMIC_H 1

#include <config.h>
#include <isc/platform.h>
#include <isc/types.h>

/*
 * This routine atomically increments the value stored in 'p' by 'val', and
 * returns the previous value.
 */
#ifdef ISC_PLATFORM_HAVEXADD
static __inline isc_int32_t
isc_atomic_xadd(isc_int32_t *p, isc_int32_t val) {
	return (isc_int32_t) _InterlockedExchangeAdd((long *)p, (long)val);
}
#endif

#ifdef ISC_PLATFORM_HAVEXADDQ
static __inline isc_int64_t
isc_atomic_xaddq(isc_int64_t *p, isc_int64_t val) {
	return (isc_int64_t) _InterlockedExchangeAdd64((__int64 *)p,
						       (__int64) val);
}
#endif

/*
 * This routine atomically stores the value 'val' in 'p' (32-bit version).
 */
#ifdef ISC_PLATFORM_HAVEATOMICSTORE
static __inline void
isc_atomic_store(isc_int32_t *p, isc_int32_t val) {
	(void) _InterlockedExchange((long *)p, (long)val);
}
#endif

/*
 * This routine atomically stores the value 'val' in 'p' (64-bit version).
 */
#ifdef ISC_PLATFORM_HAVEATOMICSTOREQ
static __inline void
isc_atomic_storeq(isc_int64_t *p, isc_int64_t val) {
	(void) _InterlockedExchange64((__int64 *)p, (__int64)val);
}
#endif

/*
 * This routine atomically replaces the value in 'p' with 'val', if the
 * original value is equal to 'cmpval'.  The original value is returned in any
 * case.
 */
#ifdef ISC_PLATFORM_HAVECMPXCHG
static __inline isc_int32_t
isc_atomic_cmpxchg(isc_int32_t *p, isc_int32_t cmpval, isc_int32_t val) {
	/* beware: swap arguments */
	return (isc_int32_t) _InterlockedCompareExchange((long *)p,
							 (long)val,
							 (long)cmpval);
}
#endif

#endif /* ISC_ATOMIC_H */
