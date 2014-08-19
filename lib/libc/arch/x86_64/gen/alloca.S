/*	$NetBSD: alloca.S,v 1.1.64.1 2014/08/20 00:02:12 tls Exp $	*/

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: alloca.S,v 1.1.64.1 2014/08/20 00:02:12 tls Exp $")
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
