/*	$NetBSD: strlen.S,v 1.1 1999/01/15 08:44:27 castor Exp $	*/
#include <machine/cdefs.h>
#include <mips/asm.h>

	.set	noreorder

/*
 * strlen(str)
 */
LEAF(strlen)
	addu	v1, a0, 1
1:
	lb	v0, 0(a0)		# get byte from string
	addu	a0, a0, 1		# increment pointer
	bne	v0, zero, 1b		# continue if not end
	nop
	j	ra
	subu	v0, a0, v1		# compute length - 1 for '\0' char
END(strlen)
