/*	$NetBSD: flt_rounds.S,v 1.7 2011/09/29 22:46:00 christos Exp $	*/

#include <machine/asm.h>

/*
 * 00 0 round to zero 
 * 01 1 round to nearest
 * 10 2 round to positive infinity
 * 11 3 round to negative infinity
 */
	.text
	_ALIGN_TEXT
ENTRY(__flt_rounds)
	fnstcw	-4(%esp)
	movl	-4(%esp), %ecx
	shrl	$9, %ecx
	andl	$6, %ecx
	xorl	$1, %eax	/* map 0,1,2,3 -> 1,0,3,2 */
	andl	$3, %eax
	ret
