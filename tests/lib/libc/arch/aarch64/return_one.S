/* $NetBSD: return_one.S,v 1.1.4.2 2014/08/20 00:04:49 tls Exp $ */

#include <machine/asm.h>

ENTRY_NP(return_one)
	mov	x0, #1
	ret
	.globl	return_one_end
return_one_end:
END(return_one)
