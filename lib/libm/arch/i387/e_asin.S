/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_asin.S,v 1.5 2000/09/26 07:28:24 kleink Exp $")

/* asin = atan (x / sqrt(1 - x^2)) */
ENTRY(__ieee754_asin)
	fldl	4(%esp)			/* x */
	fld	%st(0)
	fmul	%st(0)			/* x^2 */
	fld1
	fsubp				/* 1 - x^2 */
	fsqrt				/* sqrt (1 - x^2) */
	fpatan
	ret
