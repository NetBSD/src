/*	$NetBSD: ptrace.S,v 1.3 2000/02/23 20:16:57 kleink Exp $	*/

#include "SYS.h"

ENTRY(ptrace)
#ifdef _REENTRANT
	mflr	0
	stwu	1,-32(1)
	stw	0,36(1)
	stw	3,8(1)
	stw	4,12(1)
	stw	5,16(1)
	stw	6,20(1)

	bl	PIC_PLT(_C_LABEL(__errno))
	li	7,0
	stw	7,0(3)

	lwz	3,8(1)
	lwz	4,12(1)
	lwz	5,16(1)
	lwz	0,36(1)
	lwz	6,20(1)
	mtlr	0
	la	1,32(1)
#else
#ifdef PIC
	mflr	0
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	7
	lwz	7,_C_LABEL(errno)@got(7)
	li	8,0
	stw	8,0(7)
	mtlr	0
#else
	lis	7,_C_LABEL(errno)@ha
	li	8,0
	stw	8,_C_LABEL(errno)@l(7)
#endif /* PIC */
#endif /* _REENTRANT */
	li	0,SYS_ptrace
	sc
	bso	1f
	blr
1:
	b	PIC_PLT(_C_LABEL(__cerror))
