/*	$NetBSD: in_cksum.S,v 1.3.8.4 2005/02/04 11:44:56 skrll Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * in_cksum() and in4_cksum() implementations for SH5.
 *
 * The SH5 does not have a carry flag, which complicates matters somewhat.
 * On the plus side, misaligned buffers are a piece of cake to deal with
 * thanks to the ldlo.q and ldhi.q instructions.
 */

#include "opt_inet.h"

#include <machine/asm.h>
#include "assym.h"

/*
 * Add With Carry two quads, "q1" and "q2. Put the result in "result".
 *
 * Trashes r0.
 */
#define	ADDC(q1, q2, result)		 \
	add	q2, q1, result		;\
	cmpgtu	q1, result, r0		;\
	add	r0, result, result

/*
 * Reduce the quad in "q" to a 32-bit sum, dealing with any
 * resulting carry. Put the result in "result".
 *
 * Trashes r0 and q.
 */
#define	REDUCE32(q, result)		 \
	mshflo.l q, r63, r0		;\
	mshfhi.l q, r63, q		;\
	add	r0, q, q		;\
	mshflo.l q, r63, r0		;\
	mshfhi.l q, r63, q		;\
	add	r0, q, result

/*
 * Reduce the 32-bit int in "i" to a 16-bit sum, dealing with any
 * resulting carry. Put the result in "result".
 *
 * Trashes r0 and i.
 */
#define	REDUCE16(i, result)		 \
	mshflo.w i, r63, r0		;\
	shlri	i, 16, i		;\
	addz.l	r0, i, i		;\
	mshflo.w i, r63, r0		;\
	shlri	i, 16, i		;\
	add	r0, i, result


/*
 * The main in*_cksum() workhorse...
 *
 * Entry parameters:
 *
 *	r3	Buffer length
 *	r4	Pointer to buffer
 *	r17	Must be set to 0x1f
 *	r18	Return address
 *
 * Returns:
 *
 *	r7	Accumulated 64-bit sum
 *
 * Trashes:
 *	r0, r1, r3, r4, r19, r20, r21, r22
 *	tr0, tr1, tr2
 */

ASENTRY(cksumdata)
	movi	0, r7
	ld.b	r4, 0, r63		/* Pre-fetch the start of the buffer */
	ptabs/u	r18, tr0
	pta/u	Lend_game, tr1

	/*
	 * We first have to quad-align the buffer.
	 */
	pta/u	Lalready_aligned, tr2
	andi	r4, 0x7, r1
	xori	r1, 0x7, r0
	beqi/u	r0, 0x7, tr2		/* Jump if already quad aligned */
	addi	r0, 1, r0		/* r0 == # bytes to next quad */
	bgtu/u	r0, r3, tr1		/* Not enough bytes left to make it */
	ldlo.q	r4, 0, r19		/* Fetch 1 to 7 bytes */
	shlli	r1, 3, r1
	add	r4, r0, r4		/* r4 is now quad-aligned */
	sub	r3, r0, r3		/* Update remaining length */
	shlld	r19, r1, r19
	ADDC	(r19, r7, r7)		/* Accumulate the bytes we just read */
	beq/u	r3, r63, tr0		/* Return to caller if done */

	/*
	 * Buffer is now quad-aligned.
	 * We now need to align it to a 32-byte boundary.
	 */
Lalready_aligned:
	and	r4, r17, r1
	xor	r1, r17, r0
	addi	r0, 1, r0		/* r0 == # bytes to 32-byte boundary */
	bgtu/u	r0, r3, tr1		/* Jump if not enough left to align */
	add	r4, r0, r4		/* Update buffer pointer */
	sub	r3, r0, r3		/* Update remaining bytes */
	shlri	r1, 1, r1		/* Compute loop entry-point in order */
	addi	r1, 17, r1		/* align buffer to 32-byte boundaey */
	movi	0, r19
	movi	0, r20
	movi	0, r21
	pta/u	Lbig_loop, tr2
	ptrel/l	r1, tr1
	blink	tr1, r63		/* Go for it. */

	/*
	 * At this point:
	 *
	 *  r3 == # of bytes remaining, AFTER loop entry.
	 *  r4 -> *next* 32-byte aligned chunk of buffer.
	 *
	 * The "big_loop" checksums 32 bytes at a time.
	 */

Lbig_loop:
	addi	r4, 32, r4
	addi	r3, -32, r3
	ld.q	r4, -32, r19
	ld.q	r4, -24, r20
	ld.q	r4, -16, r21
	ld.q	r4, -8, r22
	ld.q	r4, 0, r63		/* Pre-fetch next chunk */
	ADDC	(r19, r7, r7)
	ADDC	(r20, r7, r7)
	ADDC	(r21, r7, r7)
	ADDC	(r22, r7, r7)
	bgt/l	r3, r17, tr2

	/*
	 * There are less than 32-bytes left.
	 */
Lend_game:
	beq/u	r3, r63, tr0		/* Exit if all done */
	add	r3, r4, r0
	andi	r3, 0x18, r1
	add	r4, r1, r4
	xor	r1, r17, r1
	shlri	r1, 1, r1
	addi	r1, 6, r1
	movi	0, r19
	movi	0, r20
	movi	0, r21
	ptrel/l	r1, tr1
	blink	tr1, r63
	ld.q	r4, -24, r19
	ld.q	r4, -16, r20
	ld.q	r4, -8, r21
	ldhi.q	r0, -1, r22		/* The last quad needs special care */
	movi	-1, r1			/* to deal with mis-alignment, and */
	andi	r3, 7, r3		/* to ensure we don't include any */
	shlli	r3, 3, r3		/* bytes past the end of the buffer */
#ifdef __LITTLE_ENDIAN__
	shlrd	r1, r3, r1
#else
	shlld	r1, r3, r1
#endif
	cmveq	r3, r3, r1
	andc	r22, r1, r22
	xori	r0, 7, r0
	addi	r0, 1, r0
	andi	r0, 7, r0
	cmveq	r0, r0, r22
	shlli	r0, 3, r0
#ifdef __LITTLE_ENDIAN__
	shlrd	r22, r0, r22
#else
	shlld	r22, r0, r22
#endif
	ADDC	(r19, r7, r7)
	ADDC	(r20, r7, r7)
	ADDC	(r21, r7, r7)
	ADDC	(r22, r7, r7)
	blink	tr0, r63

/*
 * int in_cksum(struct mbuf *m, int len)
 */
ENTRY(in_cksum)
#ifndef _LP64
	add.l	r2, r63, r5
#else
	add	r2, r63, r5
#endif
	addz.l	r3, r63, r6
	movi	0, r2
	pta/l	Lcksum_entry, tr2

	/*
	 * r2  == current sum
	 * r5  == m
	 * r6  == len
	 */
Lcksum_top:
	pta/u	_ASM_LABEL(cksumdata), tr4
	pta/u	Lcksum_loop, tr3
	movi	0, r7
	movi	0, r8
	movi	0x1f, r17
	add	r18, r63, r23
	blink	tr2, r63

Lcksum_loop:
	ld.l	r5, M_LEN, r3
	LDPTR	r5, M_DATA, r4
	LDPTR	r5, M_NEXT, r5
Lcksum_entry4:
	cmpgt	r3, r6, r0
	cmvne	r0, r6, r3
	sub	r6, r3, r6
	xor	r8, r4, r0
	add	r8, r3, r8
	andi	r0, 1, r0
	shlli	r0, 3, r9
	blink	tr4, r18
	REDUCE32(r7, r7)
	shlld	r7, r9, r7
	add	r7, r2, r2
Lcksum_entry:
	pta/u	4f, tr0
	beq/u	r6, r63, tr0
	bne/l	r5, r63, tr3


	/*
	 * XXX: Do we need to take account of odd final r8?
	 */
4:	REDUCE32(r2, r2)
	REDUCE16(r2, r2)
	ptabs/l	r23, tr0
	xori	r2, -1, r2
	shlli	r2, 48, r2
	shlri	r2, 48, r2
	blink	tr0, r63


#ifdef INET
/*
 * int in4_cksum(struct mbuf *m, u_int8_t nxt, int off, int len)
 */
ENTRY(in4_cksum)
	addz.l	r5, r63, r6
#ifndef _LP64
	add.l	r2, r63, r5
#else
	add	r2, r63, r5
#endif
	shlli	r3, 56, r3
	shlri	r3, 56, r2
	addz.l	r4, r63, r8

	/*
	 * r2 == nxt (sum)
	 * r5 == m
	 * r6 == len
	 * r8 == off
	 */

	/*
	 * First, deal with a pseudo header, if present
	 */
	pta/l	Lno_pseudo, tr0
	beq/l	r2, r63, tr0		/* Jump if no pseudo header */
	LDPTR	r5, M_DATA, r4
	add	r2, r6, r2		/* sum += len */
#ifdef __LITTLE_ENDIAN__
	shlli	r2, 8, r2		/* sum = htons(sum) */
#endif
	ldlo.q	r4, IP_SRC, r19		/* Note: Assumes ip_src/ip_dst are */
	ldhi.q	r4, IP_SRC+7, r20	/*       contiguous in memory */
	or	r19, r20, r19
	ADDC	(r19, r2, r2)		/* sum += ip->ip_{src,dst} */
	movi	0, r3
#endif	/* INET */


#if defined(INET) || defined(INET6)
Lno_pseudo:
	pta/u	Lskip_loop, tr0
	pta/l	Lskip_entry, tr1
	pta/u	Lskip_done, tr2
	blink	tr1, r63

Lskip_loop:
	ld.l	r5, M_LEN, r3
	LDPTR	r5, M_DATA, r4
	sub	r8, r3, r8		/* off -= m->m_len */
	LDPTR	r5, M_NEXT, r5
Lskip_entry:
	bgt/l	r63, r8, tr2		/* Break loop if off < 0 */
	bne/l	r5, r63, tr0		/* Go back until out of mbufs */

	/*
	 * Well wha' d'ya know, wan out of widdle piggies...
	 */
	pta/l	Lout_of_mbufs, tr0
	blink	tr0, r63

Lskip_done:
	add	r8, r4, r4
	xori	r8, -1, r8
	add	r3, r4, r4
	addi	r8, 1, r3
	pta/l	Lcksum_entry4, tr2
	pta/l	Lcksum_top, tr0
	blink	tr0, r63

Lout_of_mbufs:
	LEA(Lmbuf_msg, r2)
	pta/l	_C_LABEL(printf), tr0
	blink	tr0, r63

Lmbuf_msg:
	.asciz	"cksum: out of mbufs\n"
#endif /* INET || INET6 */

