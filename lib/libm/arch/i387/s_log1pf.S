/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_log1pf.S,v 1.7 2003/07/26 19:25:02 salo Exp $")

/*
 * Since the fyl2xp1 instruction has such a limited range:
 *	-(1 - (sqrt(2) / 2)) <= x <= sqrt(2) - 1
 * it's not worth trying to use it.
 */

ENTRY(log1pf)
	XMM_ONE_ARG_FLOAT_PROLOGUE
	fldln2
	flds	ARG_FLOAT_ONE
	fld1
	faddp
	fyl2x
	XMM_FLOAT_EPILOGUE
	ret
