/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_remainder.S,v 1.4 1999/08/15 18:45:18 kleink Exp $")

ENTRY(__ieee754_remainder)
	fmoved	sp@(4),fp0
	fremd	sp@(12),fp0
#ifndef __SVR4_ABI__
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
#endif
	rts
