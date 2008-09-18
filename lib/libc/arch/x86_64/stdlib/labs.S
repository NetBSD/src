/*	$NetBSD: labs.S,v 1.1.34.1 2008/09/18 04:39:20 wrstuden Exp $	*/

/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public Domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: labs.S,v 1.1.34.1 2008/09/18 04:39:20 wrstuden Exp $")
#endif

#ifdef WEAK_ALIAS
WEAK_ALIAS(imaxabs, _llabs)
WEAK_ALIAS(llabs, _llabs)
WEAK_ALIAS(labs, _labs)
#endif

#ifdef WEAK_ALIAS
ENTRY(_llabs)
ENTRY(_labs)
#else
ENTRY(llabs)
ENTRY(labs)
#endif
        movq    %rdi,%rax
        testq   %rax,%rax
        jns     1f
        negq    %rax
1:      ret
