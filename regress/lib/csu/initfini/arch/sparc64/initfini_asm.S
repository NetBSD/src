/*	$NetBSD: initfini_asm.S,v 1.2 2001/07/17 14:02:41 mrg Exp $	*/

/*
 * This file placed in the public domain.
 * matthew green, July 17, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	call	_C_LABEL(i_am_init)
	 nop
	.previous

	.section .fini, "ax", @progbits
	call	_C_LABEL(i_am_fini)
	 nop
	.previous
