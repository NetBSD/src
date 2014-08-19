/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_acos.S,v 1.8.60.1 2014/08/20 00:02:18 tls Exp $")

/* acos = atan (sqrt(1 - x^2) / x) */
ENTRY(__ieee754_acos)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE		/* x */
	fld	%st(0)
	fmul	%st(0)			/* x^2 */
	fld1
	fsubp				/* 1 - x^2 */
	fsqrt				/* sqrt (1 - x^2) */
	fabs				/* Change -0.0 to +0.0 */
	fxch	%st(1)
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret
