/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_fmod.S,v 1.1 1996/07/08 03:27:25 thorpej Exp $")

ENTRY(__ieee754_fmod)
	fmoved	sp@(4),fp0
	fmodd	sp@(12),fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	rts
