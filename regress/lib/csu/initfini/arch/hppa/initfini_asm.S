/*	$NetBSD: initfini_asm.S,v 1.1 2004/05/18 19:19:34 skrll Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, May 18, 2004.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	copy	%r19, %r4
	bl	i_am_init, %rp
	nop
	copy	%r4, %r19
	.previous

	.section .fini, "ax", @progbits
	copy	%r19, %r4
	bl	i_am_fini, %rp
	nop
	copy	%r4, %r19
	.previous
