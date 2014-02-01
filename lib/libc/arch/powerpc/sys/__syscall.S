/*	$NetBSD: __syscall.S,v 1.2 2014/02/01 20:26:21 matt Exp $	*/

#include <powerpc/asm.h>

	.text
	.align	2
ENTRY(__syscall)
	mr	%r0,%r3		/* syscall number */
	mr	%r3,%r4
	mr	%r4,%r5
	mr	%r5,%r6
	mr	%r6,%r7
	mr	%r7,%r8
	mr	%r8,%r9
	mr	%r9,%r10
	lwz	%r10,8(%r1)	/* final argument is on the stack */
	sc
	bnslr
	b	_C_LABEL(__cerror)
END(__syscall)

STRONG_ALIAS(syscall, __syscall)
