/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_expf.S,v 1.2 1999/07/02 15:37:34 simonb Exp $")

/* e^x = 2^(x * log2(e)) */
ENTRY(__ieee754_expf)
	flds	4(%esp)
	fldl2e
	fmulp				/* x * log2(e) */
	fstl	%st(1)
	frndint				/* int(x * log2(e)) */
	fstl	%st(2)
	fsubrp				/* fract(x * log2(e)) */
	f2xm1				/* 2^(fract(x * log2(e))) - 1 */
	fld1
	faddp				/* 2^(fract(x * log2(e))) */
	fscale				/* e^x */
	fstpl	%st(1)
	ret
