/*	$NetBSD: locore_machdep.S,v 1.1.10.2 2002/06/23 17:35:50 jdolecek Exp $	*/

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
