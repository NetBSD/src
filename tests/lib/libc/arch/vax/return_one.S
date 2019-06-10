/*	$NetBSD: return_one.S,v 1.1.42.1 2019/06/10 22:10:03 christos Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	.word 0
	subl2 $4,%sp
	movl $1,%r0
	ret
return_one_end:
