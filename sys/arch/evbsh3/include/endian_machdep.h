/*	$NetBSD: endian_machdep.h,v 1.1.6.3 2001/01/05 17:34:11 bouyer Exp $	*/

/* XXX relying upon gcc symbol here... */
#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER _LITTLE_ENDIAN
#else
#define _BYTE_ORDER _BIG_ENDIAN
#endif
#include <sh3/endian_machdep.h>
