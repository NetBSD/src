/*	$NetBSD: flt_rounds_softfloat.S,v 1.1.2.2 2001/10/08 20:17:39 nathanw Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#include <machine/asm.h>

	.text
	.even

	/* NB: this is tied to the gcc-2.95 lb1sf68.asm: */
_map:
	.byte 1		/* round to nearest */
	.byte 0		/* round to zero */
	.byte 2		/* round to positive infinity */
	.byte 3		/* round to negative infinity */

ENTRY(__flt_rounds)
	lea	_C_LABEL(_fpCCR),%a0	| check the rounding mode
	movew	%a0@(6),%d0	| rounding mode in d0
	lea	_map,%a0
	moveb	%a0@(%d0:l:1),%d0
	rts


		
