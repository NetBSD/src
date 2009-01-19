/*	$NetBSD: start.S,v 1.1.8.2 2009/01/19 13:16:01 skrll Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	jra	_C_LABEL(bootxxx)
