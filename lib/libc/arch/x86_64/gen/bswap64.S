/*	$NetBSD: bswap64.S,v 1.1 2001/06/19 00:25:02 fvdl Exp $	*/

/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public Domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: bswap64.S,v 1.1 2001/06/19 00:25:02 fvdl Exp $")
#endif

ENTRY(bswap64)
	bswapq	%rdi
	movq	%rdi,%rax
	ret
