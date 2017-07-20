/*	$NetBSD: return_one.S,v 1.2 2017/07/20 01:13:47 uwe Exp $ */

#include <machine/asm.h>

NENTRY(return_one)
	rts
	 mov	#1, r0
	SET_ENTRY_SIZE(return_one)
	.globl	return_one_end
return_one_end:
