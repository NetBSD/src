/*	$NetBSD: atomic.h,v 1.1.1.1 2007/01/27 21:07:51 christos Exp $	*/

/*
 * Copyright (C) 2005  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: atomic.h,v 1.1.6.3 2005/09/01 03:39:13 marka Exp */

#ifndef ISC_ATOMIC_H
#define ISC_ATOMIC_H 1

#include <isc/platform.h>
#include <isc/types.h>

/*!\file
 * static inline isc_int32_t
 * isc_atomic_xadd(isc_int32_t *p, isc_int32_t val);
 *
 * This routine atomically increments the value stored in 'p' by 'val', and
 * returns the previous value.
 *
 * static inline void
 * isc_atomic_store(void *p, isc_int32_t val);
 *
 * This routine atomically stores the value 'val' in 'p'.
 *
 * static inline isc_int32_t
 * isc_atomic_cmpxchg(isc_int32_t *p, isc_int32_t cmpval, isc_int32_t val);
 *
 * This routine atomically replaces the value in 'p' with 'val', if the
 * original value is equal to 'cmpval'.  The original value is returned in any
 * case.
 */

#if defined(_AIX)

#include <sys/atomic_op.h>

#define isc_atomic_xadd(p, v) fetch_and_add(p, v)
#define isc_atomic_store(p, v) _clear_lock(p, v)

#ifdef __GNUC__
static inline int
#else
static int
#endif
isc_atomic_cmpxchg(atomic_p p, int old, int new) {
        int orig = old;

#ifdef __GNUC__
        asm("ics");
#else
         __isync();
#endif
        if (compare_and_swap(p, &orig, new))
		return (old);
        return (orig);
}

#elif ISC_PLATFORM_USEGCCASM
static inline isc_int32_t
isc_atomic_xadd(isc_int32_t *p, isc_int32_t val) {
	isc_int32_t orig;

	__asm__ volatile (
		"1:"
		"lwarx r6, 0, %1\n"
	    	"mr %0, r6\n"
		"add r6, r6, %2\n"
		"stwcx. r6, 0, %1\n"
		"bne- 1b"
		: "=&r"(orig)
		: "r"(p), "r"(val)
		: "r6", "memory"
		);

	return (orig);
}

static inline void
isc_atomic_store(void *p, isc_int32_t val) {
	__asm__ volatile (
		"1:"
		"lwarx r6, 0, %0\n"
		"lwz r6, %1\n"
		"stwcx. r6, 0, %0\n"
		"bne- 1b"
		:
		: "r"(p), "m"(val)
		: "r6", "memory"
		);
}

static inline isc_int32_t
isc_atomic_cmpxchg(isc_int32_t *p, isc_int32_t cmpval, isc_int32_t val) {
	isc_int32_t orig;

	__asm__ volatile (
		"1:"
		"lwarx r6, 0, %1\n"
		"mr %0,r6\n"
		"cmpw r6, %2\n"
		"bne 2f\n"
		"mr r6, %3\n"
		"stwcx. r6, 0, %1\n"
		"bne- 1b\n"
		"2:"
		: "=&r" (orig)
		: "r"(p), "r"(cmpval), "r"(val)
		: "r6", "memory"
		);

	return (orig);
}

#else

#error "unsupported compiler.  disable atomic ops by --disable-atomic"

#endif
#endif /* ISC_ATOMIC_H */
