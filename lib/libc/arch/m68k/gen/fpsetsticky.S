/*	$NetBSD: fpsetsticky.S,v 1.5.12.1 1999/12/27 18:29:30 wrstuden Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetsticky)
	movel %d2,%sp@-
	fmovel %fpsr,%d1
	movel %sp@(8),%d2
	bfextu %d1{#24:#5},%d0
	bfins %d2,%d1{#24:#5}
	movel %sp@+,%d2
	fmovel %d1,%fpsr
	rts
