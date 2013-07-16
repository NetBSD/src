/*	$NetBSD: fpsetmask.S,v 1.9 2013/07/16 22:12:20 matt Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public Domain.
 */

#include <machine/asm.h>

WEAK_ALIAS(fpsetmask, _fpsetmask)
ENTRY(_fpsetmask)
	movel %d2,-(%sp)
	fmovel %fpcr,%d1
	movel 8(%sp),%d2
	bfextu %d1{#18:#5},%d0
	bfins %d2,%d1{#18:#5}
	movel (%sp)+,%d2
	fmovel %d1,%fpcr
	rts
END(_fpsetmask)
