/*	$NetBSD: fpsetsticky.S,v 1.1 2002/07/12 12:20:25 scw Exp $	*/

/*
 * Written by Steve C. Woodford at Wasabi Systems for NetBSD
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetsticky,_fpsetsticky)
ENTRY(_fpsetsticky)
#else
ENTRY(fpsetsticky)
#endif
	fgetscr	fr0
	ptabs/l	r18, tr0
	fmov.sl	fr0, r0
	movi	0x7c, r1	/* Mask for the sticky bits */
	shlli	r2, 2, r2	/* Shift new mask to correct bit position */
	and	r2, r1, r2	/* Zap unwanted bits */
	andc	r0, r1, r1	/* Clear sticky bits in existing mask */
	or	r1, r2, r1	/* OR-in the new mask */
	fmov.ls	r1, fr0
	shlri	r0, 2, r2	/* Get old mask in return value */
	andi	r2, 0x1f, r2	/* Zap unwanted bits */
	fputscr	fr0		/* Update the h/w mask */
	blink	tr0, r63
