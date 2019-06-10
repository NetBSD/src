/*	$NetBSD: return_one.S,v 1.1.42.1 2019/06/10 22:10:03 christos Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	jr	$31
	li	$2,1

return_one_end:
