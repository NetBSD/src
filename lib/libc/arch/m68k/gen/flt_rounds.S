/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

	.text
	.even
_map:
	.byte 1		/* round to nearest */
	.byte 0		/* round to zero */
	.byte 3		/* round to negative infinity */
	.byte 2		/* round to positive infinity */

ENTRY(__flt_rounds)
	fmovel fpcr,d0
	lsrl #4,d0
	andl #3,d0
	lea _map,a0
	moveb a0@(d0:l:1),d0
	rts
