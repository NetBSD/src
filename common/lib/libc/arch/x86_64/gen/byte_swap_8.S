/*	$NetBSD: byte_swap_8.S,v 1.1.24.1 2014/08/10 06:47:06 tls Exp $	*/

/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public Domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_8.S,v 1.1.24.1 2014/08/10 06:47:06 tls Exp $")
#endif

ENTRY(bswap64)
	bswapq	%rdi
	movq	%rdi,%rax
	ret
END(bswap64)
