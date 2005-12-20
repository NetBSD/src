/*	$NetBSD: byte_swap_4.S,v 1.1 2005/12/20 19:28:51 christos Exp $	*/

#include <machine/asm.h>
#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_4.S,v 1.1 2005/12/20 19:28:51 christos Exp $")
#endif

_ENTRY(_C_LABEL(__bswap32))
_ENTRY(_C_LABEL(ntohl))
_ENTRY(_C_LABEL(htonl))
_PROF_PROLOGUE
	movl	%edi,%eax
	bswapl	%eax
	ret
