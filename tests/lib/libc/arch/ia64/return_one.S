/*	$NetBSD: return_one.S,v 1.1.24.1 2016/08/06 00:19:11 pgoyette Exp $ */

#include <machine/asm.h>

.globl	return_one_end	

ENTRY(return_one,0)
	mov	ret0=1
	br.ret.sptk.few  rp
return_one_end:	
END(return_one)
