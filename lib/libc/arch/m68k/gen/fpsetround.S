/*	$NetBSD: fpsetround.S,v 1.7.8.1 2002/01/28 20:49:53 nathanw Exp $	*/

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
	movel %d2,%sp@-
	fmovel %fpcr,%d1
	movel %sp@(8),%d2
	bfextu %d1{#26,#2},%d0
	bfins %d2,%d1{#26:#2}
	movel %sp@+,%d2
	fmovel %d1,%fpcr
	rts
