/*	$NetBSD: return_one.S,v 1.1.8.1 2014/08/20 00:04:49 tls Exp $ */

#include <machine/asm.h>

ENTRY_NP(return_one)
	mov	r0, #1
	RET
	.align	0
	.globl	return_one_end
return_one_end:
END(return_one)
