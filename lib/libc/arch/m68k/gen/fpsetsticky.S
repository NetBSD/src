/*	$NetBSD: fpsetsticky.S,v 1.8 2013/07/16 20:49:42 matt Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetsticky, _fpsetsticky)
ENTRY(_fpsetsticky)
#else
ENTRY(fpsetsticky)
#endif
	movel %d2,-(%sp)
	fmovel %fpsr,%d1
	movel 8(%sp),%d2
	bfextu %d1{#24:#5},%d0
	bfins %d2,%d1{#24:#5}
	movel (%sp)+,%d2
	fmovel %d1,%fpsr
	rts
