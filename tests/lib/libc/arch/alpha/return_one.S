/*	$NetBSD: return_one.S,v 1.2 2017/12/30 16:35:03 martin Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	lda	v0,1
	ret
return_one_end:
	nop
