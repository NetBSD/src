/*	$NetBSD: fpgetmask.S,v 1.2 2002/01/13 21:45:51 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 4, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpgetmask, _fpgetmask)
ENTRY(_fpgetmask)
#else
ENTRY(fpgetmask)
#endif
	fnstcw -4(%rsp)
	movl -4(%rsp),%eax
	notl %eax
	andl $63,%eax
	ret
