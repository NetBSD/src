/*	$NetBSD: byte_swap_2.S,v 1.1 2001/06/19 00:25:02 fvdl Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_2.S,v 1.1 2001/06/19 00:25:02 fvdl Exp $")
#endif

_ENTRY(_C_LABEL(__bswap16))
_ENTRY(_C_LABEL(ntohs))
_ENTRY(_C_LABEL(htons))
_PROF_PROLOGUE
	movl	%edi,%eax
	xchgb	%ah,%al
	ret
