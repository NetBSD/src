/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_remainderf.S,v 1.3 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(__ieee754_remainderf)
	XMM_TWO_ARG_FLOAT_PROLOGUE
	flds	ARG_FLOAT_TWO
	flds	ARG_FLOAT_ONE
1:	fprem1
	fstsw	%ax
	btw	$2,%ax
	jc	1b
	fstp	%st(1)
	XMM_FLOAT_EPILOGUE
	ret
