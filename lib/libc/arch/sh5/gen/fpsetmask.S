/*	$NetBSD: fpsetmask.S,v 1.1 2002/07/12 12:20:25 scw Exp $	*/

/*
 * Written by Steve C. Woodford at Wasabi Systems for NetBSD.
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetmask,_fpsetmask)
ENTRY(_fpsetmask)
#else
ENTRY(fpsetmask)
#endif
	fgetscr	fr0
	ptabs/l	r18, tr0
	fmov.sl	fr0, r0
	movi	0x0f80, r1	/* Mask for Enable bits */
	shlli	r2, 7, r2	/* Shift supplied mask to correct position */
	and	r2, r1, r2	/* Zap any unwanted bits in supplied mask */
	andc	r0, r1, r1	/* Clear the existing mask bits */
	or	r1, r2, r1	/* OR-in the new mask */
	fmov.ls	r1, fr0
	shlri	r0, 7, r2	/* Old mask to return value */
	andi	r2, 0x1f, r2	/* Zap unwanted bits */
	fputscr	fr0		/* Update the current mask */
	blink	tr0, r63
