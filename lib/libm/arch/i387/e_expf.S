/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"


RCSID("$NetBSD: e_expf.S,v 1.5 2003/07/26 19:24:58 salo Exp $")

/* e^x = 2^(x * log2(e)) */
ENTRY(__ieee754_expf)
	XMM_ONE_ARG_FLOAT_PROLOGUE
	flds	ARG_FLOAT_ONE
	fldl2e
	fmulp				/* x * log2(e) */
	fld	%st(0)
	frndint				/* int(x * log2(e)) */
	fsubr	%st(0),%st(1)		/* fract(x * log2(e)) */
	fxch
	f2xm1				/* 2^(fract(x * log2(e))) - 1 */
	fld1
	faddp				/* 2^(fract(x * log2(e))) */
	fscale				/* e^x */
	fstp	%st(1)
	XMM_FLOAT_EPILOGUE
	ret
