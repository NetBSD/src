/*
 * Written by J.T. Conklin, Apr 5, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetsticky)
	movl	d2,sp@-
	movl	sp@(8),d2
	fmovel	fpsr,d1
	bfextu	d1{#16:#8},d0
	bfins	d2,d1{#16:#8}
	fmovel	d1,fpsr
	movl	sp@+,d2
	rts
