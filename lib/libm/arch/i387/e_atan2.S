/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_atan2.S,v 1.5 2001/06/19 00:26:29 fvdl Exp $")

ENTRY(__ieee754_atan2)
	XMM_TWO_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE
	fldl	ARG_DOUBLE_TWO
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret
