/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_log.S,v 1.5 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(__ieee754_log)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldln2
	fldl	ARG_DOUBLE_ONE
	fyl2x
	XMM_DOUBLE_EPILOGUE
	ret
