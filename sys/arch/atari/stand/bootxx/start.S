/*	$NetBSD: start.S,v 1.1.4.2 2009/01/16 22:57:33 bouyer Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	bra	_C_LABEL(bootxx)	| bootxx is small enough so bra works
