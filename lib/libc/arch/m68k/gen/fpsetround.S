/*	$NetBSD: fpsetround.S,v 1.6.12.1 1999/12/27 18:29:30 wrstuden Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public Domain.
 */

#include <machine/asm.h>

ENTRY(fpsetround)
	movel %d2,%sp@-
	fmovel %fpcr,%d1
	movel %sp@(8),%d2
	bfextu %d1{#26,#2},%d0
	bfins %d2,%d1{#26:#2}
	movel %sp@+,%d2
	fmovel %d1,%fpcr
	rts
