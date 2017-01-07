/*	$NetBSD: return_one.S,v 1.1.24.1 2017/01/07 08:56:54 pgoyette Exp $ */

#include <machine/asm.h>

.global	return_one_end

ENTRY(return_one)
return_one:
	retl
	 mov 1, %o0
return_one_end:
