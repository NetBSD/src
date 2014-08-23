/*	$NetBSD: cerror.S,v 1.6 2014/08/23 02:24:22 matt Exp $	*/

#include <machine/asm.h>
#include "SYS.h"

#ifndef _REENTRANT
	.globl	_C_LABEL(errno)
#endif
#ifdef __PIC__
	.protected _C_LABEL(__cerror)
#endif

ENTRY(__cerror)
#ifdef _REENTRANT
	mflr	%r0
	streg	%r0,__SIZEOF_POINTER__(%r1)
	stptru	%r1,-(4*__SIZEOF_POINTER__)(%r1) # allocate new stack frame
	streg	%r31,(3*__SIZEOF_POINTER__)(%r1)
	mr	%r31,%r3		# stash away in callee-saved register
	bl	PIC_PLT(_C_LABEL(__errno))
	nop
	stint	%r31,0(%r3)

	ldreg	%r31,(3*__SIZEOF_POINTER__)(%r1)
	addi	%r1,%r1,4*__SIZEOF_POINTER__
	ldreg	%r0,__SIZEOF_POINTER__(%r1)
	mtlr	%r0
#else
	.pushsection ".toc", "aw"
.Lerrno:.tc	errno[TC], errno
	.popsection
	lwz	%r4,_C_LABEL(.Lerrno)@toc(%r2)
	stw	%r3,0(%r4)
#endif /* _REENTRANT */
	li	%r3,-1
	li	%r4,-1
	blr
END(__cerror)
