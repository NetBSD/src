/*	$NetBSD: return_one.S,v 1.1.34.1 2018/02/26 01:20:21 snj Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	lda	v0,1
	ret
return_one_end:
	nop
