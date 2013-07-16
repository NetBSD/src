/*	$NetBSD: fpsetround.S,v 1.9 2013/07/16 20:49:42 matt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public Domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetround, _fpsetround)
ENTRY(_fpsetround)
#else
ENTRY(fpsetround)
#endif
	movel %d2,-(%sp)
	fmovel %fpcr,%d1
	movel 8(%sp),%d2
	bfextu %d1{#26,#2},%d0
	bfins %d2,%d1{#26:#2}
	movel (%sp)+,%d2
	fmovel %d1,%fpcr
	rts
