/* $NetBSD: gd_qnan.h,v 1.2.2.1 2011/06/23 14:18:36 cherry Exp $ */

#define f_QNAN 0x7fc00000
#define d_QNAN0 0x0
#define d_QNAN1 0x7ff80000
#define ld_QNAN0 0x0
#define ld_QNAN1 0xc0000000
#define ld_QNAN2 0x7fff
#define ld_QNAN3 0x0
#define ldus_QNAN0 0x0
#define ldus_QNAN1 0x0
#define ldus_QNAN2 0x0
#define ldus_QNAN3 0xc000
#define ldus_QNAN4 0x7fff
/* 6 bytes of tail padding follow, per AMD64 ABI */
