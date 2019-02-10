/*	$NetBSD: return_one.S,v 1.2 2019/02/10 09:54:39 mlelstv Exp $ */

#include <machine/asm.h>

_ENTRY(return_one)
	moveq #1,%d0
	rts
	.globl return_one_end
return_one_end:
