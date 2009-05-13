/*	$NetBSD: atomic.h,v 1.1.2.2 2009/05/13 18:51:49 jym Exp $	*/

#ifndef ISC_ATOMIC_H
#define ISC_ATOMIC_H 1

#include <sys/atomic.h>
#include <isc/types.h>

/*
 * This routine atomically increments the value stored in 'p' by 'val', and
 * returns the previous value.
 */
static __inline isc_int32_t
isc_atomic_xadd(isc_int32_t *p, isc_int32_t val) {
	return (isc_int32_t)atomic_add_32_nv((volatile uint32_t *)p,
	    (uint32_t)val) - val;
}

#ifdef ISC_PLATFORM_HAVEXADDQ
static __inline isc_int64_t
isc_atomic_xaddq(isc_int64_t *p, isc_int64_t val) {
	return (isc_int64_t)atomic_add_64_nv((volatile uint64_t *)p,
	    (uint64_t)val) - val;
}
#endif

/*
 * This routine atomically stores the value 'val' in 'p'.
 */
static __inline void
isc_atomic_store(isc_int32_t *p, isc_int32_t val) {
	(void)atomic_swap_32((volatile uint32_t *)p, (uint32_t)val);
}

/*
 * This routine atomically replaces the value in 'p' with 'val', if the
 * original value is equal to 'cmpval'.  The original value is returned in any
 * case.
 */
static __inline__ isc_int32_t
isc_atomic_cmpxchg(isc_int32_t *p, isc_int32_t cmpval, isc_int32_t val) {
	return (isc_int32_t) atomic_cas_32((volatile uint32_t *)p,
	    (uint32_t)cmpval, (uint32_t)val);
}

#endif /* ISC_ATOMIC_H */
