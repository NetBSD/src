/*	$NetBSD: fpgetround.S,v 1.1 2002/07/12 12:20:25 scw Exp $	*/

/*
 * Written by Steve C. Woodford at Wasabi Systems for NetBSD.
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpgetround,_fpgetround)
ENTRY(_fpgetround)
#else
ENTRY(fpgetround)
#endif
	fgetscr	fr0
	ptabs/l	r18, tr0
	fmov.sl	fr0, r2
	andi	r2, 1, r2	/* Rounding mode is in bit#0 */
	xori	r2, 1, r2	/* We use inverse of h/w bit */
	blink	tr0, r63
