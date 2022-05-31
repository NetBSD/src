/*	$NetBSD: return_one.S,v 1.3 2022/05/31 07:10:50 skrll Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	bv	%r0(%r2)
	 ldi	1,%r28
return_one_end:
