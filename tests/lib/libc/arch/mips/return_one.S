/*	$NetBSD: return_one.S,v 1.2 2019/02/14 10:36:33 mrg Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	jr	$31
	li	$2,1

return_one_end:
