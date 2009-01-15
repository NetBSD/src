/*	$NetBSD: start.S,v 1.1.2.2 2009/01/15 23:20:02 bouyer Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	bra	_C_LABEL(bootxx)	| bootxx is small enough so bra works
