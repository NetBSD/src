/*	$NetBSD: e_fmod.S,v 1.2.4.1 1999/12/27 18:30:01 wrstuden Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_fmod.S,v 1.2.4.1 1999/12/27 18:30:01 wrstuden Exp $")

ENTRY(__ieee754_fmod)
	fmoved	%sp@(4),%fp0
	fmodd	%sp@(12),%fp0
#ifndef __SVR4_ABI__
	fmoved	%fp0,%sp@-
	movel	%sp@+,%d0
	movel	%sp@+,%d1
#endif
	rts
