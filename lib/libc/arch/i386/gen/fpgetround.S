/*	$NetBSD: fpgetround.S,v 1.4 1998/01/09 03:45:04 perry Exp $	*/

/*
 * Written by J.T. Conklin, Apr 4, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetround)
	subl $4,%esp
	fnstcw (%esp)
	movl (%esp),%eax
	rorl $10,%eax
	andl $3,%eax
	addl $4,%esp
	ret
