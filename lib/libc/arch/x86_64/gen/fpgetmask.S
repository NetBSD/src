/*	$NetBSD: fpgetmask.S,v 1.1 2001/06/19 00:25:02 fvdl Exp $	*/

/*
 * Written by J.T. Conklin, Apr 4, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetmask)
	fnstcw -4(%rsp)
	movl -4(%rsp),%eax
	notl %eax
	andl $63,%eax
	ret
