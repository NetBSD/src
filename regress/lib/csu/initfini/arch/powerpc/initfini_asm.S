/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/17 12:09:47 tsubai Exp $	*/

/*
 * This file placed in the public domain.
 * Tsubai Masanari, July 17, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	bl	i_am_init
	.previous

	.section .fini, "ax", @progbits
	bl	i_am_fini
	.previous
