/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/17 05:49:11 thorpej Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <mips/asm.h>

	.abicalls

	.section .init, "ax", @progbits
	.set noreorder
	.cpload t9
	.set reorder
	.cprestore
	la	t9, i_am_init
	jal	t9
	.align	5
	.previous

	.section .fini, "ax", @progbits
	.set noreorder
	.cpload t9
	.set reorder
	.cprestore
	la	t9, i_am_fini
	jal	t9
	.align	5
	.previous
