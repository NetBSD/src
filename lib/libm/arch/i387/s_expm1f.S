/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_expm1f.S,v 1.1 1996/07/03 16:50:26 jtc Exp $")

/* e^x = 2^(x * log2(e)) */
ENTRY(expm1f)
	flds	4(%esp)
	fldl2e
	fmulp				/* x * log2(e) */
	fstl	%st(1)
	frndint				/* int(x * log2(e)) */
	fstl	%st(2)
	fsubrp				/* fract(x * log2(e)) */
	f2xm1				/* 2^(fract(x * log2(e))) - 1 */ 
	fscale				/* e^x - 1*/
	fstpl	%st(1)
	ret
