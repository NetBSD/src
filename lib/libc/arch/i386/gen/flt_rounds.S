/*
 * Written by J.T. Conklin, Apr 4, 1995
 * Public domain.
 */

#include <machine/asm.h>

	.text
	.align 2
_map:
	.byte 1		/* round to nearest */
	.byte 3		/* round to negative infinity */
	.byte 2		/* round to positive infinity */
	.byte 0		/* round to zero */

ENTRY(__flt_rounds)
	subl $4,%esp
	fnstcw (%esp)
	movw (%esp),%ax
	shrl $10,%eax
	andl $3,%eax
	movb _map(,%eax,1),%al
	addl $4,%esp
	ret
