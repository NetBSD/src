/*	$NetBSD: return_one.S,v 1.2 2019/02/14 10:36:33 mrg Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	bv %r0(%r2)
	ldi 1,%r28
return_one_end:
