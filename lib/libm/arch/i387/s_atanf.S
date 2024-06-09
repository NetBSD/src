/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_atanf.S,v 1.6 2024/06/09 13:33:36 riastradh Exp $")

WEAK_ALIAS(atanf, _atanf)

ENTRY(_atanf)
	XMM_ONE_ARG_FLOAT_PROLOGUE
	flds	ARG_FLOAT_ONE
	fld1
	fpatan
	XMM_FLOAT_EPILOGUE
	ret
END(_atanf)
