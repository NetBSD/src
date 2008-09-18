/*	$NetBSD: ldiv.S,v 1.1.34.1 2008/09/18 04:39:20 wrstuden Exp $	*/

/*
 * Written by gcc 3.0.
 * Copy/pasted by Frank van der Linden (fvdl@wasabisystems.com)
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: ldiv.S,v 1.1.34.1 2008/09/18 04:39:20 wrstuden Exp $")
#endif

#ifdef WEAK_ALIAS
WEAK_ALIAS(ldiv, _ldiv)
#endif

#ifdef WEAK_ALIAS
ENTRY(_ldiv)
#else
ENTRY(ldiv)
#endif
	movq    %rdi, %rax
	cqto
	idivq   %rsi
	ret
