/*	$NetBSD: initfini_asm.S,v 1.1 2001/07/17 03:41:04 thorpej Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	br	ra, 1f
1:	LDGP(ra)
	jsr	ra, i_am_init
	.align	5
	.previous

	.section .fini, "ax", @progbits
	br	ra, 1f
1:	LDGP(ra)
	jsr	ra, i_am_fini
	.align	5
	.previous
