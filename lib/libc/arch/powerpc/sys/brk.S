/*	$NetBSD: brk.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

#include "SYS.h"

	.globl	curbrk

	.text

#ifdef PIC
	XXX
#else
ENTRY(brk)
	lis	5,_end@ha		# r5 = &_end
	addi	5,5,_end@l
	cmplw	5,3			# if (&_end <= r3)
	bgt	0f
	mr	5,3			# r5 = r3
0:
	mr	3,5			# new break value
	li	0,SYS_break
	sc				# assume, that r5 is kept
	bso	1f
	lis	6,curbrk@ha
	stw	5,curbrk@l(6)		# record new break
	blr				# return 0

1:
	b	cerror
#endif
