/*	$NetBSD: fpgetround.S,v 1.6 1995/12/13 19:36:15 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetround)
	fmovel fpcr,d0
	bfextu d0{#26:#2},d0
	rts
