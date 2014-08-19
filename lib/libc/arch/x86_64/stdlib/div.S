/*	$NetBSD: div.S,v 1.1.64.1 2014/08/20 00:02:12 tls Exp $	*/
/*
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
RCSID("$NetBSD: div.S,v 1.1.64.1 2014/08/20 00:02:12 tls Exp $")
#endif

ENTRY(div)
	movl	%edi, %eax
	cltd
	idivl	%esi
	salq	$32, %rdx
	orq	%rdx,%rax
	ret
END(div)
