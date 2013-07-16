/*	$NetBSD: fpgetround.S,v 1.9 2013/07/16 22:12:20 matt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#include <machine/asm.h>

WEAK_ALIAS(fpgetround, _fpgetround)
ENTRY(_fpgetround)
	fmovel %fpcr,%d0
	bfextu %d0{#26:#2},%d0
	rts
END(_fpgetround)
