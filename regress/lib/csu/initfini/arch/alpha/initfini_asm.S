/*	$NetBSD: initfini_asm.S,v 1.2 2002/11/22 17:25:46 thorpej Exp $	*/

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <machine/asm.h>

	.section .init, "ax", @progbits
	br	ra, 1f
1:	LDGP(ra)
	jsr	ra, i_am_init
	.align	3
	.previous

	.section .fini, "ax", @progbits
	br	ra, 1f
1:	LDGP(ra)
	jsr	ra, i_am_fini
	.align	3
	.previous
