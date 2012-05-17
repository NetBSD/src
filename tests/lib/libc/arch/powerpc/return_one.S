/*	$NetBSD: return_one.S,v 1.1.4.1 2012/05/17 18:26:32 riz Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end

_ENTRY(return_one)
	li	%r3, 1
	blr
return_one_end:
END(return_one)
