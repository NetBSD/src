/*	$NetBSD: byte_swap_8.S,v 1.2.24.2 2020/04/21 19:37:48 martin Exp $	*/

/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public Domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: byte_swap_8.S,v 1.2.24.2 2020/04/21 19:37:48 martin Exp $")
#endif

ENTRY(bswap64)
	bswapq	%rdi
	movq	%rdi,%rax
	ret
END(bswap64)
