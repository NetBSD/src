/*	$NetBSD: initfini_asm.S,v 1.2 2001/07/17 15:24:28 simonb Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <mips/asm.h>

	.section .init, "ax", @progbits
	la	t9, i_am_init
	jal	t9
	.align	5
	.previous

	.section .fini, "ax", @progbits
	la	t9, i_am_fini
	jal	t9
	.align	5
	.previous
