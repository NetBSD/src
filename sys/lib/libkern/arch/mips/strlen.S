/*	$NetBSD: strlen.S,v 1.1.42.1 2004/10/19 15:58:08 skrll Exp $	*/
#include <machine/cdefs.h>
#include <mips/asm.h>

#if defined(_LP64)
#define	ADDU	daddu
#define SUBU	dsubu
#else
#define ADDU	addu
#define SUBU	subu
#endif

	.set	noreorder

/*
 * strlen(str)
 */
LEAF(strlen)
	ADDU	v1, a0, 1
1:
	lb	v0, 0(a0)		# get byte from string
	ADDU	a0, a0, 1		# increment pointer
	bne	v0, zero, 1b		# continue if not end
	nop
	j	ra
	SUBU	v0, a0, v1		# compute length - 1 for '\0' char
END(strlen)
