/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_scalbnf.S,v 1.5 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(scalbnf)
	XMM_TWO_ARG_FLOAT_PROLOGUE
	fildl	ARG_FLOAT_TWO
	flds	ARG_FLOAT_ONE
	fscale
	fstp	%st(1)		/* bug fix for fp stack overflow */
	XMM_FLOAT_EPILOGUE
	ret
