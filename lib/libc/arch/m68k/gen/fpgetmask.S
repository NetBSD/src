/*	$NetBSD: fpgetmask.S,v 1.6 1999/10/25 23:48:10 thorpej Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetmask)
	fmovel %fpcr,%d0
	bfextu %d0{#18:#5},%d0
	rts
