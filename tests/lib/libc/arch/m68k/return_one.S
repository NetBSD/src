/*	$NetBSD: return_one.S,v 1.1.42.1 2019/06/10 22:10:03 christos Exp $ */

#include <machine/asm.h>

_ENTRY(return_one)
	moveq #1,%d0
	rts
	.globl return_one_end
return_one_end:
