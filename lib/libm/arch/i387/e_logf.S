/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_logf.S,v 1.3 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(__ieee754_logf)
	XMM_ONE_ARG_FLOAT_PROLOGUE
	fldln2
	flds	ARG_FLOAT_ONE
	fyl2x
	XMM_FLOAT_EPILOGUE
	ret
