/*	$NetBSD: return_one.S,v 1.4 2022/05/31 07:12:15 skrll Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	bv	%r0(%r2)
	 ldi	1,%r28
return_one_end:

.globl	return_1, return_1_end;

return_1:
	bv	%r0(%r2)
	 ldi	1,%r28
return_1_end:

.globl	return_2, return_2_end;

return_2:
	bv	%r0(%r2)
	 ldi	2,%r28
return_2_end:

.globl	return_3, return_3_end;

return_3:
	bv	%r0(%r2)
	 ldi	3,%r28
return_3_end:
