/*	$NetBSD: start.S,v 1.1 2009/01/06 13:35:30 tsutsui Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	bra	_C_LABEL(bootxx)	| bootxx is small enough so bra works
