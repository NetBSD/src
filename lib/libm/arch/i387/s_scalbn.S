/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_scalbn.S,v 1.6 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(scalbn)
	XMM_TWO_ARG_DOUBLE_PROLOGUE
	fildl	ARG_DOUBLE_TWO
	fldl	ARG_DOUBLE_ONE
	fscale
	fstp	%st(1)		/* bug fix for fp stack overflow */
	XMM_DOUBLE_EPILOGUE
	ret
