/*	$NetBSD: fpgetmask.S,v 1.6.8.1 2002/01/28 20:49:52 nathanw Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpgetmask, _fpgetmask)
ENTRY(_fpgetmask)
#else
ENTRY(fpgetmask)
#endif
	fmovel %fpcr,%d0
	bfextu %d0{#18:#5},%d0
	rts
