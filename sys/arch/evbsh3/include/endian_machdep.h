/*	$NetBSD: endian_machdep.h,v 1.1.6.2 2000/11/20 20:07:36 bouyer Exp $	*/

#if 1
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
#include <sh3/endian_machdep.h>
