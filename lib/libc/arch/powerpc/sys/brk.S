/*	$NetBSD: brk.S,v 1.4 1998/11/24 11:14:54 tsubai Exp $	*/

#include "SYS.h"

	.globl	_ASM_LABEL(curbrk)
	.globl	minbrk

	.data
minbrk:	.long	_C_LABEL(end)		# XXX not used yet

	.text

ENTRY(brk)
#ifdef PIC
	mflr	10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	9
	mtlr	10
	lwz	5,_C_LABEL(end)@got(9)
#else
	lis	5,_C_LABEL(end)@ha	# r5 = &_end
	addi	5,5,_C_LABEL(end)@l
#endif
	cmplw	5,3			# if (&_end <= r3)
	bgt	0f
	mr	5,3			# r5 = r3
0:
	mr	3,5			# new break value
	li	0,SYS_break
	sc				# assume, that r5 is kept
	bso	1f
#ifdef PIC
	lwz	6,_ASM_LABEL(curbrk)@got(9)
	stw	5,0(6)
#else
	lis	6,_ASM_LABEL(curbrk)@ha	# record new break
	stw	5,_ASM_LABEL(curbrk)@l(6)
#endif
	blr				# return 0

1:
	b	PIC_PLT(_ASM_LABEL(cerror))
