/*	$NetBSD: fpsetround.S,v 1.2 2002/01/13 21:45:52 thorpej Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetround, _fpsetround)
ENTRY(_fpsetround)
#else
ENTRY(fpsetround)
#endif
	fnstcw -4(%rsp)
	movl -4(%rsp),%eax

	rorl $10,%eax
	movl %eax,%edx
	andl $3,%eax

	subl %eax,%edx
	andl $3,%edi
	orl %edi,%edx
	roll $10,%edx
	movl %edx,-4(%rsp)
	fldcw -4(%rsp)
	ret
