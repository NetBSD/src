/*	$NetBSD: fpgetmask.S,v 1.5 1995/12/13 19:36:13 thorpej Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetmask)
	fmovel fpcr,d0
	bfextu d0{#18:#5},d0
	rts
