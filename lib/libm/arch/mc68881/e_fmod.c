/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_fmod.c,v 1.1 1996/06/26 14:36:14 jtc Exp $")

ENTRY(__ieee754_fmod)
	fmoved	sp@(4),fp0
	fmodd	sp@(12),fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	rts
