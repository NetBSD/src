/*	$NetBSD: return_one.S,v 1.1.2.1 2014/05/22 11:42:20 yamt Exp $ */

#include <machine/asm.h>

ENTRY_NP(return_one)
	mov	r0, #1
	RET
	.align	0
	.globl	return_one_end
return_one_end:
END(return_one)
