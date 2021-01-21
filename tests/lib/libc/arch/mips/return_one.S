/*	$NetBSD: return_one.S,v 1.3 2021/01/21 00:56:41 simonb Exp $ */

#include <machine/asm.h>

.set noreorder

.globl	return_one, return_one_end;

return_one:
	jr	ra
	 li	v0,1

return_one_end:
