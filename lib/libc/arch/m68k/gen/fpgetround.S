/*	$NetBSD: fpgetround.S,v 1.7 1999/10/25 23:48:10 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetround)
	fmovel %fpcr,%d0
	bfextu %d0{#26:#2},%d0
	rts
