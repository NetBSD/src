/*	$NetBSD: brk.S,v 1.3 1998/05/25 15:35:06 ws Exp $	*/

#include "SYS.h"

	.globl	_ASM_LABEL(curbrk)

	.text

#ifdef PIC
	XXX
#else
ENTRY(brk)
	lis	5,_C_LABEL(end)@ha	# r5 = &_end
	addi	5,5,_C_LABEL(end)@l
	cmplw	5,3			# if (&_end <= r3)
	bgt	0f
	mr	5,3			# r5 = r3
0:
	mr	3,5			# new break value
	li	0,SYS_break
	sc				# assume, that r5 is kept
	bso	1f
	lis	6,_ASM_LABEL(curbrk)@ha	# record new break
	stw	5,_ASM_LABEL(curbrk)@l(6)
	blr				# return 0

1:
	b	_ASM_LABEL(cerror)
#endif
