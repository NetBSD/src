/* $NetBSD: lrint.S,v 1.1 2004/07/01 17:30:48 drochner Exp $ */

#include <machine/asm.h>

ENTRY(lrint)
#ifdef __i386__
	pushl   %ebp
	movl    %esp,%ebp
	subl    $4,%esp
	fldl	8(%ebp)
	fistpl	(%esp)
	movl	(%esp),%eax
	leave
	ret
#else
	cvtsd2siq %xmm0,%rax
	ret
#endif
