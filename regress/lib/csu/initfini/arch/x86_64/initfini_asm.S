/*	$NetBSD: initfini_asm.S,v 1.1 2004/03/02 14:26:12 drochner Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	call	_C_LABEL(i_am_init)
	.previous

	.section .fini, "ax", @progbits
	call	_C_LABEL(i_am_fini)
	.previous
