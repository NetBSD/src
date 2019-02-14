/*	$NetBSD: return_one.S,v 1.2 2019/02/14 10:36:33 mrg Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	.word 0
	subl2 $4,%sp
	movl $1,%r0
	ret
return_one_end:
