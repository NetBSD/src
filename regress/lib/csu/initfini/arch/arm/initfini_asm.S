/*	$NetBSD: initfini_asm.S,v 1.2 2004/12/27 00:47:57 mrg Exp $	*/

/*
 * This file placed in the public domain.
 * Matt A. Thomas, July 16, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", "progbits"
	bl	_C_LABEL(i_am_init)
	.previous

	.section .fini, "ax", "progbits"
	bl	_C_LABEL(i_am_fini)
	.previous
