/*	$NetBSD: strcmp.S,v 1.1.42.1 2004/10/19 15:58:08 skrll Exp $	*/
#include <machine/cdefs.h>
#include <mips/asm.h>

#if defined(_LP64)
#define ADDU    daddu
#define SUBU    dsubu
#else   
#define ADDU    addu    
#define SUBU    subu
#endif 

	.set noreorder

/*
 * NOTE: this version assumes unsigned chars in order to be "8 bit clean".
 */
LEAF(strcmp)
1:
	lbu	t0, 0(a0)		# get two bytes and compare them
	lbu	t1, 0(a1)
	beq	t0, zero, LessOrEq	# end of first string?
	nop
	bne	t0, t1, NotEq
	nop
	lbu	t0, 1(a0)		# unroll loop
	lbu	t1, 1(a1)
	beq	t0, zero, LessOrEq	# end of first string?
	ADDU	a0, a0, 2
	beq	t0, t1, 1b
	ADDU	a1, a1, 2
NotEq:
	j	ra
	SUBU	v0, t0, t1
LessOrEq:
	j	ra
	SUBU	v0, zero, t1
END(strcmp)
