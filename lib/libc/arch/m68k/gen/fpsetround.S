/*	$NetBSD: fpsetround.S,v 1.10 2013/07/16 22:12:20 matt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public Domain.
 */

#include <machine/asm.h>

WEAK_ALIAS(fpsetround, _fpsetround)
ENTRY(_fpsetround)
	movel %d2,-(%sp)
	fmovel %fpcr,%d1
	movel 8(%sp),%d2
	bfextu %d1{#26,#2},%d0
	bfins %d2,%d1{#26:#2}
	movel (%sp)+,%d2
	fmovel %d1,%fpcr
	rts
END(_fpsetround)
