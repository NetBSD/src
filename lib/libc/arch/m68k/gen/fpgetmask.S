/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetmask)
	fmovel fpcr,d0
	lsrl #6,d0
	andl #248,d0
	rts
