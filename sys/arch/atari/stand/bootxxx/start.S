/*	$NetBSD: start.S,v 1.1.2.2 2009/01/15 23:20:02 bouyer Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	jra	_C_LABEL(bootxxx)
