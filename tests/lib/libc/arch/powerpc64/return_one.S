/*	$NetBSD: return_one.S,v 1.1.42.1 2019/06/10 22:10:03 christos Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	li 3,1
	blr
return_one_end:
