/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_asin.S,v 1.6 2001/06/19 00:26:29 fvdl Exp $")

/* asin = atan (x / sqrt(1 - x^2)) */
ENTRY(__ieee754_asin)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE		/* x */
	fld	%st(0)
	fmul	%st(0)			/* x^2 */
	fld1
	fsubp				/* 1 - x^2 */
	fsqrt				/* sqrt (1 - x^2) */
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret
