/*	$NetBSD: return_one.S,v 1.2 2018/01/14 19:59:51 martin Exp $ */

#include <machine/asm.h>

.global	return_one_end

ENTRY(return_one)
return_one:
	retl
	 mov 1, %o0
return_one_end:
