/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/17 04:03:53 thorpej Exp $	*/

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
