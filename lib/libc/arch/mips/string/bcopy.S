/*	$NetBSD: bcopy.S,v 1.7 1997/10/18 05:21:44 jonathan Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1993 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *	File:	mips_bcopy.s
 *	Author:	Chris Maeda
 *	Date:	June 1993
 *
 *	Fast copy routine.  Derived from aligned_block_copy.
 */	


#include <mips/asm.h>
#define _LOCORE		/* XXX not really, just assembly-code source */
#include <machine/endian.h>


#if defined(LIBC_SCCS) && !defined(lint)
	ASMSTR("from: @(#)mips_bcopy.s	2.2 CMU 18/06/93")
	ASMSTR("$NetBSD: bcopy.S,v 1.7 1997/10/18 05:21:44 jonathan Exp $")
#endif /* LIBC_SCCS and not lint */

#ifdef ABICALLS
	.abicalls
#endif

/*
 *	bcopy(caddr_t src, caddr_t dst, unsigned int len)
 *
 *	a0 	src address
 *	a1	dst address
 *	a2	length
 */

LEAF(bcopy)
	.set	noat
	.set	noreorder
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
	
	.set	reorder
	.set	at
	END(bcopy)
