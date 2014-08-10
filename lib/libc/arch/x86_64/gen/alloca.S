/*	$NetBSD: alloca.S,v 1.1.76.1 2014/08/10 06:51:49 tls Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: alloca.S,v 1.1.76.1 2014/08/10 06:51:49 tls Exp $")
#endif

ENTRY(alloca)
	popq	%rdx
	movq	%rsp,%rcx
	addq	$15,%rdi	/*  round up to 16 bytes */
	andq	$~15,%rdi
	subq	%rdi,%rsp
	movq	%rsp,%rax
	jmp	*%rdx
END(alloca)
