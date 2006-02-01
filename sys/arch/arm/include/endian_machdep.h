/* $NetBSD: endian_machdep.h,v 1.7.2.1 2006/02/01 14:51:26 yamt Exp $ */

/* GCC predefines __ARMEB__ when building for big-endian ARM. */
#ifdef __ARMEB__
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
