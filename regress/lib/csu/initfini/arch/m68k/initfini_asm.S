/*	$NetBSD: initfini_asm.S,v 1.1 2001/10/14 19:47:14 kleink Exp $	*/

/*
 * This file placed in the public domain.
 * Klaus Klein, October 14, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	jsr	_C_LABEL(i_am_init)
	.previous

	.section .fini, "ax", @progbits
	jsr	_C_LABEL(i_am_fini)
	.previous
