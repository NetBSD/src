/*	$NetBSD: endian_machdep.h,v 1.6 2006/01/30 21:52:38 dsl Exp $	*/

#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER     _LITTLE_ENDIAN
#else
#define _BYTE_ORDER     _BIG_ENDIAN
#endif
