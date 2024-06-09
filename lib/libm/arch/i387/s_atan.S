/*	$NetBSD: s_atan.S,v 1.8 2024/06/09 13:35:38 riastradh Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#include "abi.h"

RCSID("$NetBSD: s_atan.S,v 1.8 2024/06/09 13:35:38 riastradh Exp $")

WEAK_ALIAS(atan, _atan)

ENTRY(_atan)
	XMM_ONE_ARG_DOUBLE_PROLOGUE
	fldl	ARG_DOUBLE_ONE
	fld1
	fpatan
	XMM_DOUBLE_EPILOGUE
	ret
END(_atan)
