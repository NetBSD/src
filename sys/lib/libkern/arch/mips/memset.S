/*	$NetBSD: memset.S,v 1.1 1999/01/28 03:08:57 jonathan Exp $	*/
#include <machine/cdefs.h>
#include <mips/asm.h>
#include <machine/endian.h>

	.set	noreorder


/*
 * memset(void *s1, int c, int len)
 */
LEAF(memset)
	blt	a2, 12, smallclr	# small amount to clear?
	sll	t1, a1, 8
	or	t1, t1, a1
	sll	t1, a1, 16
	or	t1, t1, a1
	subu	t0, zero, a0		# compute # bytes to word align address
	and	t0, t0, 3
	beq	t0, zero, 1f		# skip if word aligned
	subu	a2, a2, t0		# subtract from remaining count
	SWHI	t1, 0(a0)		# store 1, 2, or 3 bytes to align
	addu	a0, a0, t0
1:
	and	v0, a2, 3		# compute number of words left
	subu	t0, a2, v0
	move	a2, v0
	addu	t0, t0, a0		# compute ending address
2:
	addu	a0, a0, 4		# clear words
	bne	a0, t0, 2b		#  unrolling loop does not help
	sw	t1, -4(a0)		#  since we are limited by memory speed
smallclr:
	ble	a2, zero, 2f
	addu	t0, a2, a0		# compute ending address
1:
	addu	a0, a0, 1		# clear bytes
	bne	a0, t0, 1b
	sb	a1, -1(a0)
2:
	j	ra
	nop
END(memset)
