/*
 * Written by J.T. Conklin, Apr 4, 1995
 * Public domain.
 */

#include <machine/asm.h>

	.text
	.align 2
_map:
	.long 1		/* round to nearest */
	.long 3		/* round to negative infinity */
	.long 2		/* round to positive infinity */
	.long 0		/* round to zero */

ENTRY(__flt_rounds)
	subl $4,%esp
	fnstcw 2(%esp)
	movw 2(%esp),%ax
	shrl $10,%eax
	andl $3,%eax
	movl _map(,%eax,4),%eax
	addl $4,%esp
	ret
