/*	$NetBSD: fpgetmask.S,v 1.8 2013/07/16 22:12:20 matt Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

WEAK_ALIAS(fpgetmask, _fpgetmask)
ENTRY(_fpgetmask)
	fmovel %fpcr,%d0
	bfextu %d0{#18:#5},%d0
	rts
END(_fpgetmask)
