/*	$NetBSD: fpgetmask.S,v 1.1 2002/07/12 12:20:25 scw Exp $	*/

/*
 * Written by Steve C. Woodford at Wasabi Systems for NetBSD.
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpgetmask,_fpgetmask)
ENTRY(_fpgetmask)
#else
ENTRY(fpgetmask)
#endif
	fgetscr	fr0
	ptabs/l	r18, tr0
	fmov.sl	fr0, r2
	shlri	r2, 7, r2	/* Get Enable mask to low bits */
	andi	r2, 0x1f, r2	/* Zap unwanted bits */
	blink	tr0, r63
