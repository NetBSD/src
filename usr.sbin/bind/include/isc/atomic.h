/*	$NetBSD: atomic.h,v 1.5 2008/06/21 22:53:06 christos Exp $	*/

#include <sys/atomic.h>

/*
 * This routine atomically increments the value stored in 'p' by 'val', and
 * returns the previous value.
 */
static __inline isc_int32_t
isc_atomic_xadd(isc_int32_t *p, isc_int32_t val) {
        isc_int32_t prev = val;
	atomic_add_32((volatile uint32_t *)p, (uint32_t)val);
	return prev;
}

static __inline isc_int64_t
isc_atomic_xaddq(isc_int64_t *p, isc_int64_t val) {
        isc_int64_t prev = val;
	atomic_add_64((volatile uint64_t *)p, (uint64_t)val);
        return prev;
}

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

