/*	$NetBSD: fpsetmask.S,v 1.2 2002/01/13 21:45:51 thorpej Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetmask, _fpsetmask)
ENTRY(_fpsetmask)
#else
ENTRY(fpsetmask)
#endif
	fnstcw -4(%rsp)
	movl -4(%rsp),%eax
	movl %eax,%edx

	notl %eax
	andl $63,%eax

	addl %eax,%edx
	andl $63,%edi
	subl %edi,%edx
	movl %edx,-4(%rsp)
	fldcw -4(%rsp)
	ret
