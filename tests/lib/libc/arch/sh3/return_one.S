/*	$NetBSD: return_one.S,v 1.1.34.1 2017/07/25 02:06:10 snj Exp $ */

#include <machine/asm.h>

NENTRY(return_one)
	rts
	 mov	#1, r0
	SET_ENTRY_SIZE(return_one)
	.globl	return_one_end
return_one_end:
