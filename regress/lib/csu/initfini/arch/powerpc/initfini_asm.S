/*	$NetBSD: initfini_asm.S,v 1.1.2.2 2001/12/09 17:26:55 he Exp $	*/

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
