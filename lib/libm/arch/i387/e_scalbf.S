/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: e_scalbf.S,v 1.2 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(__ieee754_scalbf)
	XMM_TWO_ARG_FLOAT_PROLOGUE
	flds	ARG_FLOAT_TWO
	flds	ARG_FLOAT_ONE
	fscale
	XMM_FLOAT_EPILOGUE
	ret
