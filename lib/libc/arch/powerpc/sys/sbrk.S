/*	$NetBSD: sbrk.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

#include "SYS.h"

	.globl	_end
	.globl	curbrk

	.data
curbrk:	.long	_end
	.text

ENTRY(sbrk)
#ifdef PIC
	 XXX
#else
	lis	5,curbrk@ha
	lwz	6,curbrk@l(5)			# r6 = old break
	add	3,3,6
	mr	7,3				# r7 = new break
	li	0,SYS_break
	sc					# break(new_break)
	bso	1f
	mr	3,6				# set return value
	stw	7,curbrk@l(5)			# record new break
	blr
1:
	b	cerror
#endif
