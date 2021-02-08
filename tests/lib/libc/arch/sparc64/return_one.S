/*	$NetBSD: return_one.S,v 1.3 2021/02/08 23:50:25 joerg Exp $ */

#include <machine/asm.h>

.global	return_one_end

ENTRY(return_one)
	retl
	 mov 1, %o0
return_one_end:
