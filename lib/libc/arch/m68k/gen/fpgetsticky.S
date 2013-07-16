/*	$NetBSD: fpgetsticky.S,v 1.8 2013/07/16 22:12:20 matt Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

WEAK_ALIAS(fpgetsticky, _fpgetsticky)
ENTRY(_fpgetsticky)
	fmovel %fpsr,%d0
	bfextu %d0{#24:#5},%d0
	rts
END(_fpgetsticky)
