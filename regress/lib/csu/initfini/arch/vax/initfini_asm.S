/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/17 06:42:38 matt Exp $	*/

/*
 * This file placed in the public domain.
 * Matt A. Thomas, July 16, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	calls	$0,_C_LABEL(i_am_init)
	.previous

	.section .fini, "ax", @progbits
	calls	$0,_C_LABEL(i_am_fini)
	.previous
