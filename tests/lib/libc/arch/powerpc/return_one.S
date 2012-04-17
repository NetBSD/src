/*	$NetBSD: return_one.S,v 1.1.2.1 2012/04/17 00:09:10 yamt Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end

_ENTRY(return_one)
	li	%r3, 1
	blr
return_one_end:
END(return_one)
