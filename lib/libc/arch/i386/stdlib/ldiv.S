/*	$NetBSD: ldiv.S,v 1.9.8.1 2014/08/20 00:02:10 tls Exp $	*/
/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
RCSID("$NetBSD: ldiv.S,v 1.9.8.1 2014/08/20 00:02:10 tls Exp $")
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
	movl	%ebx,%eax
	popl	%ebx
	ret	$4
END(ldiv)
