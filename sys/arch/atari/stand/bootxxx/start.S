/*	$NetBSD: start.S,v 1.1.12.2 2009/05/04 08:10:48 yamt Exp $	*/

#include <m68k/asm.h>

	.text

ASENTRY_NOPROFILE(start)
	jra	_C_LABEL(bootxxx)
