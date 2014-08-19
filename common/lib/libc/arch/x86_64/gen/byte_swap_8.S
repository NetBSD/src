/*	$NetBSD: byte_swap_8.S,v 1.1.12.1 2014/08/19 23:45:14 tls Exp $	*/

/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public Domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_8.S,v 1.1.12.1 2014/08/19 23:45:14 tls Exp $")
#endif

ENTRY(bswap64)
	bswapq	%rdi
	movq	%rdi,%rax
	ret
END(bswap64)
