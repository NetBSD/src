/*	$NetBSD: byte_swap_2.S,v 1.2.70.1 2014/08/10 06:47:06 tls Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_2.S,v 1.2.70.1 2014/08/10 06:47:06 tls Exp $")
#endif

#if defined(_KERNEL) || defined(_STANDALONE)
_ENTRY(_C_LABEL(bswap16))
#else
_ENTRY(_C_LABEL(__bswap16))
#endif
_ENTRY(_C_LABEL(ntohs))
_ENTRY(_C_LABEL(htons))
_PROF_PROLOGUE
	movl	%edi,%eax
	xchgb	%ah,%al
	ret
#if defined(_KERNEL) || defined(_STANDALONE)
END(_C_LABEL(bswap16))
#else
END(_C_LABEL(__bswap16))
#endif
END(_C_LABEL(ntohs))
END(_C_LABEL(htons))
