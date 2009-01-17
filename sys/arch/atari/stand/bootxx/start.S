/*	$NetBSD: start.S,v 1.1.6.2 2009/01/17 13:27:55 mjf Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	bra	_C_LABEL(bootxx)	| bootxx is small enough so bra works
