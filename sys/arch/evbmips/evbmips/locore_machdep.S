/*	$NetBSD: locore_machdep.S,v 1.1.12.2 2002/03/07 14:43:59 simonb Exp $	*/

#include <mips/asm.h>

	.data
	.globl _C_LABEL(intrcnt)
	.globl _C_LABEL(eintrcnt)
	.globl _C_LABEL(intrnames)
	.globl _C_LABEL(eintrnames)

_C_LABEL(intrnames):
_C_LABEL(eintrnames):

	.align	2
_C_LABEL(intrcnt):
_C_LABEL(eintrcnt):
