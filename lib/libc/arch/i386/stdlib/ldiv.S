/*	$NetBSD: ldiv.S,v 1.7 2001/01/08 15:21:37 lukem Exp $	*/
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
RCSID("$NetBSD: ldiv.S,v 1.7 2001/01/08 15:21:37 lukem Exp $")
#endif

ENTRY(ldiv)
	pushl	%ebx
	movl	8(%esp),%ebx
	movl	12(%esp),%eax
	movl	16(%esp),%ecx
	cdq
	idiv	%ecx
	movl	%eax,(%ebx)
	movl	%edx,4(%ebx)
	popl	%ebx
	ret	$4
