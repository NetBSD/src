/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_rint.S,v 1.5 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(rint)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE
	frndint
	XMM_DOUBLE_EPILOGUE
	ret
