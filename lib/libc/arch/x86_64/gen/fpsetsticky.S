/*	$NetBSD: fpsetsticky.S,v 1.1 2001/06/19 00:25:03 fvdl Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetsticky)
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
