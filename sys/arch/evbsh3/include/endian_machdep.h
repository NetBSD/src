/*	$NetBSD: endian_machdep.h,v 1.2 2001/01/04 00:51:15 itojun Exp $	*/

/* XXX relying upon gcc symbol here... */
#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER _LITTLE_ENDIAN
#else
#define _BYTE_ORDER _BIG_ENDIAN
#endif
#include <sh3/endian_machdep.h>
