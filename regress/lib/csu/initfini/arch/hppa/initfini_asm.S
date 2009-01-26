/*	$NetBSD: initfini_asm.S,v 1.2 2009/01/26 18:39:47 skrll Exp $	*/

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
