/* $NetBSD: gd_qnan.h,v 1.3 2022/12/03 09:38:53 skrll Exp $ */

/*
 * The RISC-V Instruction Set Manual Volume I: User-Level ISA
 * Document Version 2.2
 *
 * 8.3 NaN Generation and Propagation
 *
 * The canonical NaN has a positive sign and all significand bits clear except
 * the MSB, aka the quiet bit.
 */

#define f_QNAN 0x7fc00000
#define d_QNAN0 0x0
#define d_QNAN1 0x7ff80000
#define ld_QNAN0 0x0
#define ld_QNAN1 0x0
#define ld_QNAN2 0x0
#define ld_QNAN3 0x7fff8000
