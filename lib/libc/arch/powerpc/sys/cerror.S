/*	$NetBSD: cerror.S,v 1.5 2000/01/27 14:58:48 kleink Exp $	*/

#include <machine/asm.h>
#include "SYS.h"

#ifndef _REENTRANT
	.globl	_C_LABEL(errno)
#endif

ENTRY(__cerror)
#ifdef _REENTRANT
	mflr	0
	stwu	1,-16(1)		# allocate new stack frame
	stw	0,20(1)
	stw	31,8(1)
	mr	31,3			# stash away in callee-saved register
	bl	PIC_PLT(_C_LABEL(__errno))
	stw	31,0(3)
	lwz	0,20(1)
	lwz	31,8(1)
	mtlr	0
	la	1,16(1)
#else
#ifdef PIC
	mflr	10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	4
	lwz	4,_C_LABEL(errno)@got(4)
	stw	3,0(4)
	mtlr	10
#else
	lis	4,_C_LABEL(errno)@ha
	stw	3,_C_LABEL(errno)@l(4)
#endif /* PIC */
#endif /* _REENTRANT */
	li	3,-1
	li	4,-1
	blr
