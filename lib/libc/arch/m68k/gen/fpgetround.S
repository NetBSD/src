/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpgetround)
	fmovel fpcr,d0
	lsrl #4,d0
	andl #3,d0
	rts
