/*	$NetBSD: initfini_asm.S,v 1.1.32.1 2009/02/02 00:10:00 snj Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, May 18, 2004.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	bl	i_am_init, %rp
	nop
	.previous

	.section .fini, "ax", @progbits
	bl	i_am_fini, %rp
	nop
	.previous
