/*	$NetBSD: endian_machdep.h,v 1.5.2.1 2006/02/01 14:51:32 yamt Exp $	*/

#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER     _LITTLE_ENDIAN
#else
#define _BYTE_ORDER     _BIG_ENDIAN
#endif
