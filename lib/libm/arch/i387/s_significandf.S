/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_significandf.S,v 1.4 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(significandf)
	XMM_ONE_ARG_FLOAT_PROLOGUE
	flds	ARG_FLOAT_ONE
	fxtract
	fstp	%st(1)
	XMM_FLOAT_EPILOGUE
	ret
