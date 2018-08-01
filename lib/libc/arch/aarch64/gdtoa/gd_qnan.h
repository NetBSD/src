/* $NetBSD: gd_qnan.h,v 1.2 2018/08/01 19:59:49 ryo Exp $ */

#define f_QNAN 0x7fc00000
#ifdef __AARCH64EB__
#define d_QNAN0 0x7ff80000
#define d_QNAN1 0x0
#define ld_QNAN0 0x7fff8000
#define ld_QNAN1 0x0
#define ld_QNAN2 0x0
#define ld_QNAN3 0x0
#else
#define d_QNAN0 0x0
#define d_QNAN1 0x7ff80000
#define ld_QNAN0 0x0
#define ld_QNAN1 0x0
#define ld_QNAN2 0x0
#define ld_QNAN3 0x7fff8000
#endif
