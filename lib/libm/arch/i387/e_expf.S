/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_expf.S,v 1.3 2000/09/26 07:28:24 kleink Exp $")

/* e^x = 2^(x * log2(e)) */
ENTRY(__ieee754_expf)
	flds	4(%esp)
	fldl2e
	fmulp				/* x * log2(e) */
	fldl	%st(0)
	frndint				/* int(x * log2(e)) */
	fsubr	%st(0),%st(1)		/* fract(x * log2(e)) */
	fxch
	f2xm1				/* 2^(fract(x * log2(e))) - 1 */
	fld1
	faddp				/* 2^(fract(x * log2(e))) */
	fscale				/* e^x */
	fstpl	%st(1)
	ret
