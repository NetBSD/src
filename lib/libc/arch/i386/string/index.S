/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: index.S,v 1.13 2005/02/07 05:22:51 christos Exp $")
#endif

#ifdef STRCHR
ENTRY(strchr)
#else
ENTRY(index)
#endif
	movl	4(%esp),%eax
	movb	8(%esp),%cl
	_ALIGN_TEXT,0x90
L1:
	movb	(%eax),%dl
	cmpb	%dl,%cl			/* found char??? */
	je 	L2
	incl	%eax
	testb	%dl,%dl			/* null terminator??? */
	jnz	L1
	xorl	%eax,%eax
L2:
	ret
