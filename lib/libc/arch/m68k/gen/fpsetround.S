/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public Domain.
 */

#include <machine/asm.h>

ENTRY(fpsetround)
	movel d2,sp@-
	fmovel fpcr,d0
	rorl #4,d0
	movel d0,d1
	andl #3,d0
	subl d0,d1
	movel sp@(8),d2
	andl #3,d2
	addl d2,d1
	roll #4,d1
	fmovel d1,fpcr
	movel sp@+,d2
	rts
