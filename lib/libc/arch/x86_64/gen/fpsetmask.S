/*	$NetBSD: fpsetmask.S,v 1.1 2001/06/19 00:25:03 fvdl Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetmask)
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
