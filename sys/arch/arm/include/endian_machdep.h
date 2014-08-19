/* $NetBSD: endian_machdep.h,v 1.8.122.1 2014/08/20 00:02:46 tls Exp $ */

/* __ARMEB__ or __AARCH64EB__ is predefined when building big-endian ARM. */
#if defined(__ARMEB__) || defined(__AARCH64EB__)
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
