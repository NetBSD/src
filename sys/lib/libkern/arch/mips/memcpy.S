/*	$NetBSD: memcpy.S,v 1.2 1999/01/15 08:44:27 castor Exp $	*/
/* XXXX We need to define this in a way which supports multiple architectures */

#include <machine/cdefs.h>
#include <mips/asm.h>
#include <machine/endian.h>

	.sdata
	.globl	__no_memcpy
__no_memcpy:	.word 0

.set	push
.set noreorder

/*
 * memcpy(to, from, len)
 */
LEAF(memcpy)
	move	v0, a0			# swap from and to
	move	a0, a1
	move	a1, v0

/*
 * {ov}bcopy(from, to, len)
 */
ALEAF(ovbcopy)
	.set	noat
	/*
	 *	Make sure we can copy forwards.
	 */
	sltu	t0,a0,a1	# t0 == a0 < a1
	addu	a3,a0,a2	# a3 == end of source
	sltu	t1,a1,a3	# t1 == a1 < a0+a2
	and	t2,t0,t1	# overlap -- copy backwards
	bne	t2,zero,backcopy

	/*
	 * 	There are four alignment cases (with frequency)
	 *	(Based on measurements taken with a DECstation 5000/200
	 *	 inside a Mach kernel.)
	 *
	 * 	aligned   -> aligned		(mostly)
	 * 	unaligned -> aligned		(sometimes)
	 * 	aligned,unaligned -> unaligned	(almost never)
	 *
	 *	Note that we could add another case that checks if
	 *	the destination and source are unaligned but the 
	 *	copy is alignable.  eg if src and dest are both
	 *	on a halfword boundary.
	 */
	andi	t1,a1,3		# get last 3 bits of dest
	bne	t1,zero,bytecopy
	andi	t0,a0,3		# get last 3 bits of src
	bne	t0,zero,destaligned

	/*
	 *	Forward aligned->aligned copy, 8*4 bytes at a time.
	 */
	li	AT,-32
	and	t0,a2,AT	/* count truncated to multiple of 32 */
	addu	a3,a0,t0	/* run fast loop up to this address */
	sltu	AT,a0,a3	/* any work to do? */
	beq	AT,zero,wordcopy
	subu	a2,t0

	/*
	 *	loop body
	 */
cp:
	lw	v0,0(a0)
	lw	v1,4(a0)
	lw	t0,8(a0)
	lw	t1,12(a0)
	addu	a0,32
	sw	v0,0(a1)
	sw	v1,4(a1)
	sw	t0,8(a1)
	sw	t1,12(a1)
	lw	t1,-4(a0)
	lw	t0,-8(a0)
	lw	v1,-12(a0)
	lw	v0,-16(a0)
	addu	a1,32
	sw	t1,-4(a1)
	sw	t0,-8(a1)
	sw	v1,-12(a1)
	bne	a0,a3,cp
	sw	v0,-16(a1)

	/*
	 *	Copy a word at a time, no loop unrolling.
	 */
wordcopy:
	andi	t2,a2,3		# get byte count / 4
	subu	t2,a2,t2	# t2 = number of words to copy * 4
	beq	t2,zero,bytecopy
	addu	t0,a0,t2	# stop at t0
	subu	a2,a2,t2
1:
	lw	v0,0(a0)
	addu	a0,4
	sw	v0,0(a1)
	bne	a0,t0,1b
	addu	a1,4

bytecopy:
	beq	a2,zero,copydone	# nothing left to do?
	nop
2:
	lb	v0,0(a0)
	addu	a0,1
	sb	v0,0(a1)
	subu	a2,1
	bgtz	a2,2b
	addu	a1,1

copydone:
	j	ra
	nop

	/*
	 *	Copy from unaligned source to aligned dest.
	 */
destaligned:
	andi	t0,a2,3		# t0 = bytecount mod 4
	subu	a3,a2,t0	# number of words to transfer
	beq	a3,zero,bytecopy
	nop
	move	a2,t0		# this many to do after we are done
	addu	a3,a0,a3	# stop point

3:
	LWHI	v0,0(a0)
	LWLO	v0,3(a0)
	addi	a0,4
	sw	v0,0(a1)
	bne	a0,a3,3b
	addi	a1,4

	j	bytecopy
	nop

	/*
	 *	Copy by bytes backwards.
	 */
backcopy:
	blez	a2,copydone	# nothing left to do?
	addu	t0,a0,a2	# end of source
	addu	t1,a1,a2	# end of destination
4:
	lb	v0,-1(t0)	
	subu	t0,1
	sb	v0,-1(t1)
	bne	t0,a0,4b
	subu	t1,1
	j	ra
	nop
	
	.set	at
END(memcpy)
.set	pop
