/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 * Adapted for NetBSD/x86_64 by Frank van der Linden <fvdl@wasabisystems.com>
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: index.S,v 1.1.22.1 2004/08/03 10:53:50 skrll Exp $")
#endif

#ifdef STRCHR
ENTRY(strchr)
#else
ENTRY(index)
#endif
	movq	%rdi,%rax
	movb	%sil,%cl
L1:
	movb	(%rax),%dl
	cmpb	%dl,%cl			/* found char? */
	je 	L2
	incq	%rax
	testb	%dl,%dl			/* null terminator? */
	jnz	L1
	xorq	%rax,%rax
L2:
	ret
