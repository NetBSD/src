/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/23 20:39:16 tsubai Exp $	*/

/*
 * This file placed in the public domain.
 * Tsubai Masanari, July 23, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	mov.l	Li_am_init, r0
	jsr	@r0
	nop
	bra	1f
	nop
	.align	2
Li_am_init:
	.long	_C_LABEL(i_am_init)
1:
	.previous

	.section .fini, "ax", @progbits
	mov.l	Li_am_fini, r0
	jsr	@r0
	nop
	bra	1f
	nop
	.align	2
Li_am_fini:
	.long	_C_LABEL(i_am_fini)
1:
	.previous
