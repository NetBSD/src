/*	$NetBSD: fpgetmask.S,v 1.5.12.1 1999/12/27 18:29:29 wrstuden Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetmask)
	fmovel %fpcr,%d0
	bfextu %d0{#18:#5},%d0
	rts
