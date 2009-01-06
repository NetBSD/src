/*	$NetBSD: start.S,v 1.1 2009/01/06 13:35:30 tsutsui Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	jra	_C_LABEL(bootxxx)
