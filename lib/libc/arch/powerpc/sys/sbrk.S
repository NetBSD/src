/*	$NetBSD: sbrk.S,v 1.5 1999/01/14 22:48:21 kleink Exp $	*/

#include "SYS.h"

	.globl	_C_LABEL(end)
	.globl	_C_LABEL(__curbrk)

	.data
_C_LABEL(__curbrk):
	.long	_C_LABEL(end)
	.text

ENTRY(sbrk)
#ifdef PIC
	mflr	10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	5
	mtlr	10
	lwz	5,_C_LABEL(__curbrk)@got(5)
	lwz	6,0(5)
#else
	lis	5,_C_LABEL(__curbrk)@ha
	lwz	6,_C_LABEL(__curbrk)@l(5)	# r6 = old break
#endif
	add	3,3,6
	mr	7,3				# r7 = new break
	li	0,SYS_break
	sc					# break(new_break)
	bso	1f
	mr	3,6				# set return value
#ifdef PIC
	stw	7,0(5)
#else
	stw	7,_C_LABEL(__curbrk)@l(5)	# record new break
#endif
	blr
1:
	b	PIC_PLT(_C_LABEL(__cerror))
