/* $NetBSD: locore_machdep.S,v 1.1.10.2 2002/06/23 17:40:23 jdolecek Exp $ */

#include <mips/asm.h>

	.data
	.globl _C_LABEL(intrcnt)
	.globl _C_LABEL(eintrcnt)
	.globl _C_LABEL(intrnames)
	.globl _C_LABEL(eintrnames)

_C_LABEL(intrnames):
	.asciiz	"softclock"
	.asciiz	"softnet"
_C_LABEL(eintrnames):

	.align	2
_C_LABEL(intrcnt):
	.word	0,0
_C_LABEL(eintrcnt):
	.word	0
