/*	$NetBSD: fpgetsticky.S,v 1.6 1999/10/25 23:48:11 thorpej Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetsticky)
	fmovel %fpsr,%d0
	bfextu %d0{#24:#5},%d0
	rts
