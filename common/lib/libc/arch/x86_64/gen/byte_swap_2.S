/*	$NetBSD: byte_swap_2.S,v 1.1 2005/12/20 19:28:51 christos Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_2.S,v 1.1 2005/12/20 19:28:51 christos Exp $")
#endif

_ENTRY(_C_LABEL(__bswap16))
_ENTRY(_C_LABEL(ntohs))
_ENTRY(_C_LABEL(htons))
_PROF_PROLOGUE
	movl	%edi,%eax
	xchgb	%ah,%al
	ret
