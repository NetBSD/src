/* $NetBSD: __aeabi_read_tp.S,v 1.2.4.3 2013/01/16 05:32:23 yamt Exp $ */

#include "SYS.h"

/*
 * Implementations of this function should corrupt only the result register
 * (r0) and the non-parameter integer core registers allowed to be corrupted
 * by the [AAPCS] (ip, lr, and CPSR).
 */
ENTRY(__aeabi_read_tp)
	mrc	p15, 0, r0, c13, c0, 3
#ifndef _ARM_ARCH_6
	cmp	r0, #0			/* was it zero? */
	RETc(ne)			/* return it's not zero */
	mov	ip, r1			/* syscall zeroes r1 */
	SYSTRAP(_lwp_getprivate)	/* can't fail */
	mov	r1, ip			/* restore r1 */
#endif
	RET
_END(__aeabi_read_tp)
