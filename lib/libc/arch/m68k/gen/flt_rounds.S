/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#include <machine/asm.h>

	.text
	.even
_map:
	.long 1		/* round to nearest */
	.long 0		/* round to zero */
	.long 3		/* round to negative infinity */
	.long 2		/* round to positive infinity */

ENTRY(__flt_rounds)
	fmovel fpcr,d0
	bfextu d0{#26:#2},d0
	lea _map,a0
	movel a0@(d0:l:4),d0
	rts
