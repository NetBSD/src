/* $NetBSD: gd_qnan.h,v 1.1.4.2 2014/08/20 00:02:08 tls Exp $ */

#define f_QNAN 0x7fc00000
#ifdef __AARCH64EB__
#define d_QNAN0 0x7ff80000
#define d_QNAN1 0x0
#define ld_QNAN0 0x7ff80000
#define ld_QNAN1 0x0
#define ld_QNAN2 0x0
#define ld_QNAN3 0x0
#else
#define d_QNAN0 0x0
#define d_QNAN1 0x7ff80000
#define ld_QNAN0 0x0
#define ld_QNAN1 0x0
#define ld_QNAN2 0x0
#define ld_QNAN3 0x7ff80000
#endif
