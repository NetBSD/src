/*
 * Written by J.T. Conklin, Apr 5, 1995
 * Public Domain.
 */

#include <machine/asm.h>

ENTRY(fpsetmask)
	movel	d2,sp@-
	movel	sp@(8),d2
	fmovel	fpcr,d1
	bfextu	d1{#16:#8},d0
	bfins	d2,d1{#16:#8}
	fmovel	d1,fpcr
	movel	sp@+,d2
	rts
