/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/17 11:29:55 mrg Exp $	*/

/*
 * This file placed in the public domain.
 * matthew green, July 16, 2001.
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
