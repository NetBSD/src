/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 * Adapted for NetBSD/x86_64 by Frank van der Linden <fvdl@wasabisystems.com>
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strlen.S,v 1.1.22.1 2004/08/03 10:53:50 skrll Exp $")
#endif

ENTRY(strlen)
	cld				/* set search forward */
	xorl	%eax,%eax		/* set search for null terminator */
	movq	$-1,%rcx		/* set search for lots of characters */
	repne				/* search! */
	scasb
	notq	%rcx			/* get length by taking	complement */
	leaq	-1(%rcx),%rax		/* and subtracting one */
	ret
