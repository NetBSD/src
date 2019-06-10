/*	$NetBSD: return_one.S,v 1.1.42.1 2019/06/10 22:10:03 christos Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	bv %r0(%r2)
	ldi 1,%r28
return_one_end:
