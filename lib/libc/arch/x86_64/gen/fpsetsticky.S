/*	$NetBSD: fpsetsticky.S,v 1.2 2002/01/13 21:45:52 thorpej Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetsticky, _fpsetsticky)
ENTRY(_fpsetsticky)
#else
ENTRY(fpsetsticky)
#endif
	fnstenv -28(%rsp)
	movl -24(%rsp),%eax
	movl %eax,%edx

	andl $63,%eax

	subl %eax,%edx
	andl $63,%edi
	addl %edi,%edx
	movl %edx,-24(%rsp)
	fldenv -28(%rsp)
	ret
