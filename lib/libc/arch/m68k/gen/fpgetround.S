/*	$NetBSD: fpgetround.S,v 1.6.12.1 1999/12/27 18:29:29 wrstuden Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetround)
	fmovel %fpcr,%d0
	bfextu %d0{#26:#2},%d0
	rts
