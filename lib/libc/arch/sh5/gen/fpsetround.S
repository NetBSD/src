/*	$NetBSD: fpsetround.S,v 1.1 2002/07/12 12:20:25 scw Exp $	*/

/*
 * Written by Steve C. Woodford at Wasabi Systems for NetBSD.
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetround,_fpsetround)
ENTRY(_fpsetround)
#else
ENTRY(fpsetround)
#endif
	fgetscr	fr0
	ptabs/l	r18, tr0
	fmov.sl	fr0, r0
	movi	1, r1		/* Mask for the rounding mode bit */
	and	r2, r1, r2	/* Ensure the supplied mask is valid */
	xor	r2, r1, r2	/* Mode needs to be inverted for the h/w */
	andc	r0, r1, r1	/* Clear existing mode bit */
	or	r1, r2, r1	/* OR-in the new mode */
	xori	r0, 1, r2	/* Copy inv of previous mode to return value */
	fmov.ls	r1, fr0
	andi	r2, 1, r2	/* Keep only relevant bit of result */
	fputscr	fr0		/* Update mode in h/w */
	blink	tr0, r63
