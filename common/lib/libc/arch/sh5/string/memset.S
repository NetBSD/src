/*	$NetBSD: memset.S,v 1.1 2005/12/20 19:28:50 christos Exp $	*/

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

#include <machine/asm.h>

/*
 * void *memset(void *buf, int c, size_t bytes)
 * void bzero(void *buf, size_t bytes)
 *
 * A fast memset(3)/bzero(3) for SH5.
 *
 * This code uses an "alloco" instruction to pre-allocate cache-lines in
 * its inner loop. Great care is taken to ensure we only pre-allocate on
 * a cache-line boundary, and only if there is at least another cache-
 * line's worth of buffer left.
 *
 * XXXX: This routine will need to be extended to support any future cpus
 * which have a cache-line size > 32 bytes.
 */
#ifdef _BZERO
ENTRY(bzero)
	or	r3, r63, r4
	movi	0, r3
#else
ENTRY(memset)
#endif
#ifndef _LP64
	add.l	r2, r63, r2
	addz.l	r4, r63, r4
#endif
	mshflo.b r3, r3, r3
	ptabs/u	r18, tr0
	beq/u	r4, r63, tr0		/* Bail now if "bytes" is zero */
	andi	r2, 7, r0
	pta/u	Laligned, tr1
	mperm.w	r3, r63, r3
	add	r4, r0, r1
	movi	8, r8
	bgtu/l	r1, r8, tr1

	/*
	 * The buffer is <= 8 bytes in length, and start+len does not
	 * straddle a quad boundary.
	 * In this case, "ldlo.q" and some masking, followed by "stlo.q"
	 * are all that's needed.
	 */
	ldlo.q	r2, 0, r0		/* Fetch the current buffer contents */
	shlli	r4, 2, r4
	movi	-1, r1
#ifdef __LITTLE_ENDIAN__
	shlld	r1, r4, r1		
	shlld	r1, r4, r1		
#else
	shlrd	r1, r4, r1
	shlrd	r1, r4, r1
#endif
	/*
	 * r1 contains a mask which will allow us to preserve the existing
	 * contents of memory between the end of the buffer and the end of
	 * the quad.
	 */
	mcmv	r0, r1, r3		/* Magic :-) */
	stlo.q	r2, 0, r3
	blink	tr0, r63		/* We're done. Return to caller */


	/*
	 * The buffer is > 8 bytes in length. The start may still be
	 * misaligned.
	 */
Laligned:
	stlo.q	r2, 0, r3		/* Deal with a misaligned start */
	sub	r8, r0, r0		/* r0 = 8 - r0 */
	sub	r4, r0, r4		/* Update r4 with remaining bytes */
	add	r2, r0, r7		/* r7 is now quad aligned */

	/*
	 * The buffer (r7) is quad aligned. Now align it to a 32-byte boundary
	 *
	 * The remaining number of bytes to set is in r4.
	 */
	movi	0x1f, r6
	pta/u	Ltrailer, tr2
	bgeu/u	r6, r4, tr2		/* Jump if less than 32 bytes left */
	add	r7, r63, r5
	add	r7, r6, r7
	andc	r7, r6, r7		/* Round up to quad boundary */
	sub	r7, r5, r1
	sub	r4, r1, r4
	xor	r1, r6, r1
	shlri	r1, 1, r1
	addi	r1, 6, r1
	ptrel/l	r1, tr1	
	blink	tr1, r63
	st.q	r7, -24, r3
	st.q	r7, -16, r3
	st.q	r7, -8, r3

	/*
	 * The buffer, in r7, is now aligned to a multiple of 32.
	 */
	bgeu/u	r6, r4, tr2		/* Jump if less than 32 bytes left */
	pta/l	Lcache_enter, tr1
	pta/u	Lcache_loop, tr2
	add	r7, r4, r5
	and	r4, r6, r4
	andc	r5, r6, r5
	blink	tr1, r63

Lcache_loop:
	alloco	r7, 0			/* Allocate one cache-line in advance */
	st.q	r7, -32, r3		/* Clear the previous cache-line */
	st.q	r7, -24, r3
	st.q	r7, -16, r3
	st.q	r7, -8, r3
Lcache_enter:
	addi	r7, 32, r7		/* Next cache-line */
	bne/l	r5, r7, tr2

	st.q	r7, -32, r3
	st.q	r7, -24, r3
	st.q	r7, -16, r3
	st.q	r7, -8, r3

	/*
	 * We have, at most, 31 bytes left to deal with, the actual number is
	 * in r4. The current buffer pointer is in r7, and it is quad aligned.
	 */
Ltrailer:
	beq/u	r4, r63, tr0		/* Return to caller if done. */
	add	r4, r7, r8
	andi	r4, 0x18, r4
	add	r7, r4, r7
	xori	r4, 0x1f, r4
	shlri	r4, 1, r4
	addi	r4, 6, r4
	ptrel/l	r4, tr1
	blink	tr1, r63
	st.q	r7, -24, r3
	st.q	r7, -16, r3
	st.q	r7, -8, r3
	sthi.q	r8, -1, r3
	blink	tr0, r63
