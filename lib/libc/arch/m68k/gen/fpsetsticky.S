/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetsticky)
	movel d2,sp@-
	fmovel fpsr,d0
	movel d0,d1
	andl #248,d0
	subl d0,d1
	movel sp@(8),d2
	andl #248,d2
	addl d2,d1
	fmovel d1,fpsr
	movel sp@+,d2
	rts
