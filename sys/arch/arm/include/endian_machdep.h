/* $NetBSD: endian_machdep.h,v 1.3 2001/02/17 14:55:44 bjh21 Exp $ */

/* GCC predefines __ARMEB__ when building for big-endian ARM. */
#ifdef __ARMEB__
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
