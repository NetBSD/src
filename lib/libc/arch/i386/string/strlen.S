/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strlen.S,v 1.7 2003/07/26 19:24:35 salo Exp $")
#endif

ENTRY(strlen)
	pushl	%edi
	movl	8(%esp),%edi		/* string address */
	cld				/* set search forward */
	xorl	%eax,%eax		/* set search for null terminator */
	movl	$-1,%ecx		/* set search for lots of characters */
	repne				/* search! */
	scasb
	notl	%ecx			/* get length by taking	complement */
	leal	-1(%ecx),%eax		/* and subtracting one */
	popl	%edi
	ret
