/*	$NetBSD: byte_swap_4.S,v 1.3.24.2 2020/04/21 19:37:48 martin Exp $	*/

#include <machine/asm.h>
#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_4.S,v 1.3.24.2 2020/04/21 19:37:48 martin Exp $")
#endif

#if defined(_KERNEL) || defined(_STANDALONE)
_ENTRY(_C_LABEL(bswap32))
#else
_ENTRY(_C_LABEL(__bswap32))
#endif
_ENTRY(_C_LABEL(ntohl))
_ENTRY(_C_LABEL(htonl))
_PROF_PROLOGUE
	movl	%edi,%eax
	bswapl	%eax
	ret
#if defined(_KERNEL) || defined(_STANDALONE)
END(_C_LABEL(bswap32))
#else
END(_C_LABEL(__bswap32))
#endif
END(_C_LABEL(ntohl))
END(_C_LABEL(htonl))
