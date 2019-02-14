/*	$NetBSD: return_one.S,v 1.2 2019/02/14 10:36:33 mrg Exp $ */

#include <machine/asm.h>

.globl	return_one, return_one_end;

return_one:
	li 3,1
	blr
return_one_end:
