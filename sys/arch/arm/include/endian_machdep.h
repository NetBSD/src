/* $NetBSD: endian_machdep.h,v 1.3.4.2 2001/03/12 13:27:22 bouyer Exp $ */

/* GCC predefines __ARMEB__ when building for big-endian ARM. */
#ifdef __ARMEB__
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
