/*      $NetBSD: bswap.h,v 1.1 1999/09/13 10:31:14 itojun Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _SH3_BSWAP_H_
#define _SH3_BSWAP_H_

#include <sys/cdefs.h>

#ifndef _KERNEL

__BEGIN_DECLS
u_int16_t bswap16 __P((u_int16_t));
u_int32_t bswap32 __P((u_int32_t));
u_int64_t bswap64 __P((u_int64_t));
__END_DECLS

#else /* _KERNEL */

__BEGIN_DECLS
static __inline u_int16_t bswap16 __P((u_int16_t));
static __inline u_int32_t bswap32 __P((u_int32_t));
u_int64_t bswap64 __P((u_int64_t));
__END_DECLS

static __inline u_int16_t
bswap16(x)
	u_int16_t x;
{
	u_int16_t rval;

	__asm __volatile ("swap.b %1,%0" : "=r"(rval) : "r"(x));

	return rval;
}

static __inline u_int32_t
bswap32(x)
	u_int32_t x;
{
	u_int32_t rval;

	__asm __volatile ("swap.b %1,%0; swap.w %0,%0; swap.b %0,%0"
			  : "=r"(rval) : "r"(x));

	return rval;
}
#endif /* _KERNEL */

#endif /* _SH3_BSWAP_H_ */
