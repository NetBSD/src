/*	$NetBSD: labs.S,v 1.1 2001/06/19 00:25:04 fvdl Exp $	*/

/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public Domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: labs.S,v 1.1 2001/06/19 00:25:04 fvdl Exp $")
#endif

ENTRY(llabs)
ENTRY(labs)
        movq    %rdi,%rax
        testq   %rax,%rax
        jns     1f
        negq    %rax
1:      ret
