/*	$NetBSD: initfini_asm.S,v 1.1.2.2 2001/12/09 17:26:54 he Exp $	*/

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
