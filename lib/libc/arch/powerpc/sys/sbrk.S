/*	$NetBSD: sbrk.S,v 1.8.4.1 2004/07/23 13:56:07 tron Exp $	*/

#include "SYS.h"

	.globl	_C_LABEL(__curbrk)
	.globl	_C_LABEL(_end)

#ifdef WEAK_ALIAS
WEAK_ALIAS(sbrk, _sbrk)
#endif

	.data
_C_LABEL(__curbrk):
	.long	_C_LABEL(_end)
	.text

ENTRY(_sbrk)
#ifdef PIC
	mflr	%r10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	%r5
	mtlr	%r10
	lwz	%r5,_C_LABEL(__curbrk)@got(%r5)
	lwz	%r6,0(%r5)
#else
	lis	%r5,_C_LABEL(__curbrk)@ha
	lwz	%r6,_C_LABEL(__curbrk)@l(%r5)	# r6 = old break
#endif
	add	%r3,%r3,%r6
	mr	%r7,%r3				# r7 = new break
	li	%r0,SYS_break
	sc					# break(new_break)
	bso	1f
	mr	%r3,%r6				# set return value
#ifdef PIC
	stw	%r7,0(%r5)
#else
	stw	%r7,_C_LABEL(__curbrk)@l(%r5)	# record new break
#endif
	blr
1:
	b	PIC_PLT(_C_LABEL(__cerror))
