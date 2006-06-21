/*	$NetBSD: endian_machdep.h,v 1.4.12.1 2006/06/21 14:55:39 yamt Exp $	*/

#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER     _LITTLE_ENDIAN
#else
#define _BYTE_ORDER     _BIG_ENDIAN
#endif
