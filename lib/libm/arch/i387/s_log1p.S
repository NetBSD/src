/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_log1p.S,v 1.9 2001/06/19 00:26:30 fvdl Exp $")

/*
 * Since the fyl2xp1 instruction has such a limited range:
 *	-(1 - (sqrt(2) / 2)) <= x <= sqrt(2) - 1
 * it's not worth trying to use it.
 */

ENTRY(log1p)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldln2
	fldl	ARG_DOUBLE_ONE
	fld1
	faddp
	fyl2x
	XMM_DOUBLE_EPILOGUE
	ret
