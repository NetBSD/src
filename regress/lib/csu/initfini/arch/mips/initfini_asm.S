/*	$NetBSD: initfini_asm.S,v 1.3 2001/08/16 04:06:35 mhitch Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <mips/asm.h>

	.abicalls

	.section .init, "ax", @progbits
	.set noreorder
	la	t9, 1f
1:	.cpload t9
	.set reorder
	.cprestore
	la	t9, i_am_init
	jal	t9
	.previous

	.section .fini, "ax", @progbits
	.set noreorder
	la	t9, 2f
2:	.cpload t9
	.set reorder
	.cprestore
	la	t9, i_am_fini
	jal	t9
	.previous
