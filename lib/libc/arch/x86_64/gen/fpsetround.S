/*	$NetBSD: fpsetround.S,v 1.1 2001/06/19 00:25:03 fvdl Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetround)
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
