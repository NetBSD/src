/*	$NetBSD: initfini_asm.S,v 1.3.48.1 2009/09/05 02:59:59 matt Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <mips/asm.h>

#if defined(__mips_o32) || defined(__mips_o64)
	.section .init, "ax", @progbits
	.set noreorder
	PTR_LA	t9, 1f
1:	.cpload t9
	.set reorder
	.cprestore
	PTR_LA	t9, i_am_init
	jal	t9
	.previous

	.section .fini, "ax", @progbits
	.set noreorder
	PTR_LA	t9, 2f
2:	.cpload t9
	.set reorder
	.cprestore
	PTR_LA	t9, i_am_fini
	jal	t9
	.previous
#else
	/*
	 * In N32/N64, gp is preserved across function calls.
	 */
	.section .init, "ax", @progbits
	.set noreorder
	PTR_LA	t9, i_am_init
	jal	t9
	 nop
	.previous

	.section .fini, "ax", @progbits
	.set noreorder
	PTR_LA	t9, i_am_fini
	jal	t9
	 nop
	.previous
#endif

