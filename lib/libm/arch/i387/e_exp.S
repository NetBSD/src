/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_exp.S,v 1.10 2001/06/19 00:26:30 fvdl Exp $")

/* e^x = 2^(x * log2(e)) */
ENTRY(__ieee754_exp)
#ifdef __i386__
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp

	fstcw	-12(%ebp)		/* store fpu control word */
	movw	-12(%ebp),%dx
	orw	$0x0180,%dx
	movw	%dx,-16(%ebp)
	fldcw	-16(%ebp)		/* load modfied control word */
	fldl	8(%ebp)
#else
	fstcw	-12(%rsp)
	movw	-12(%rsp),%dx
	orw	$0x0180,%dx
	movw	%dx,-16(%rsp)
	fldcw	-16(%rsp)
	movsd	%xmm0,-8(%rsp)
	fldl	-8(%rsp)
#endif

	fldl2e
	fmulp				/* x * log2(e) */
	fld	%st(0)
	frndint				/* int(x * log2(e)) */
	fxch	%st(1)
	fsub	%st(1),%st		/* fract(x * log2(e)) */
	f2xm1				/* 2^(fract(x * log2(e))) - 1 */
	fld1
	faddp				/* 2^(fract(x * log2(e))) */
	fscale				/* e^x */
	fstp	%st(1)

#ifdef __i386__
	fldcw	-12(%ebp)		/* restore original control word */
	leave
#else
	fstpl	-8(%rsp)
	movsd	-8(%rsp),%xmm0
	fldcw	-12(%rbp)
#endif
	ret
