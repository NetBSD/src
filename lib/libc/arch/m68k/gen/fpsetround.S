/*
 * Written by J.T. Conklin, Apr 5, 1995
 * Public Domain.
 */

/* #include <machine/asm.h> */

ENTRY(fpsetround)
	movel	d2,sp@-
	movel	sp@(8),d2
	fmovel	fpcr,d1
	bfextu	d1{#26:#2},d0
	bfins	d2,d1{#26:#2}
	fmovel	d1,fpcr
	movel	sp@+,d2
	rts
