/*
 * Written by J.T. Conklin, Apr 5, 1995
 * Public domain.
 */

/* #include <machine/asm.h> */

ENTRY(fpgetsticky)
	fmovel fpsr,d0
	bfextu d0{#16:#8},d0
	rts
