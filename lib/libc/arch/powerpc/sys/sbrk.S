/*	$NetBSD: sbrk.S,v 1.3 1998/05/25 15:35:06 ws Exp $	*/

#include "SYS.h"

	.globl	_C_LABEL(end)
	.globl	_ASM_LABEL(curbrk)

	.data
_ASM_LABEL(curbrk):
	.long	_C_LABEL(end)
	.text

ENTRY(sbrk)
#ifdef PIC
	 XXX
#else
	lis	5,_ASM_LABEL(curbrk)@ha
	lwz	6,_ASM_LABEL(curbrk)@l(5)	# r6 = old break
	add	3,3,6
	mr	7,3				# r7 = new break
	li	0,SYS_break
	sc					# break(new_break)
	bso	1f
	mr	3,6				# set return value
	stw	7,_ASM_LABEL(curbrk)@l(5)	# record new break
	blr
1:
	b	_ASM_LABEL(cerror)
#endif
