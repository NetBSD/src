/*
 * Written by J.T. Conklin, Apr 5, 1995
 * Public domain.
 */

/* #include <machine/asm.h> */

ENTRY(fpgetround)
	fmovel fpcr,d0
	bfextu d0{#26:#2},d0
	rts
