/*	$NetBSD: memcpy.S,v 1.1 2005/12/20 19:28:50 christos Exp $	*/

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
 * void *memcpy(void *dest, void *src, size_t bytes)
 *
 * This is reasonably fast memcpy() routine.
 *
 * If the src/dest parameters are suitably aligned, it will try to align
 * things such that "alloco" can be used to pre-allocate a cache-line for
 * "dest".
 *
 * If the alignment of src and dest are different, the routine falls back
 * to a byte-wise copy. This ain't great, but it serves the caller right.
 *
 * This algorithm could be improved upon, but I'm wary of trying to be
 * too smart, given the lossage experienced with SuperH's memcpy() from
 * newlib.
 */

ENTRY(memcpy)
#ifndef _LP64
	add.l	r2, r63, r7
	add.l	r3, r63, r3
	addz.l	r4, r63, r4
#else
	add	r2, r63, r7
#endif
	ptabs/u	r18, tr0
	beq/u	r4, r63, tr0		/* Bail now if bytes == 0 */

	/*
	 * First, try to align operands. This can only be done if the low 3
	 * bits match.
	 */
	pta/l	Laligned, tr1
	or	r7, r3, r1
	andi	r1, 7, r1
	beq/l	r1, r63, tr1		/* Operands are already aligned */

	pta/u	Lbyte_copy, tr1
	xor	r7, r3, r0
	andi	r0, 7, r0		/* Operands misaligned differently? */
	bne/u	r0, r63, tr1		/* Yup. Fallback to copying byte-wise */

	add	r4, r1, r0
	movi	8, r8
	bgtu/l	r8, r0, tr1

	ldlo.q	r3, 0, r0
	stlo.q	r7, 0, r0
	sub	r8, r1, r0
	sub	r4, r0, r4
	add	r7, r0, r7
	add	r3, r0, r3

	/*
	 * The buffers are quad aligned. Now align src to a 32-byte boundary
	 * if possible.
	 */
Laligned:
	movi	0x1f, r6
	pta/u	Ltrailer, tr2
	bgeu/u	r6, r4, tr2		/* Jump if less than 32 bytes left */
	add	r7, r63, r5
	add	r7, r6, r7
	andc	r7, r6, r7		/* Round dst up to 32-byte boundary */
	sub	r7, r5, r1
	add	r3, r1, r3		/* Adjust src to match */
	sub	r4, r1, r4
	xor	r1, r6, r1
	addi	r1, 2, r1
	ptrel/l	r1, tr1	
	blink	tr1, r63
	ld.q	r3, -24, r0
	st.q	r7, -24, r0
	ld.q	r3, -16, r0
	st.q	r7, -16, r0
	ld.q	r3, -8, r0
	st.q	r7, -8, r0

	/*
	 * "src" is now aligned to a multiple of 32 bytes
	 */
	bgeu/u	r6, r4, tr2		/* Jump if less than 32 bytes left */
	pta/l	Lcache_enter, tr1
	pta/u	Lcache_loop, tr2
	ld.q	r3, 0, r63		/* Prefetch one cache-line in advance */
	alloco	r7, 0			/* Allocate one cache-line in advance */
	add	r7, r4, r5
	and	r4, r6, r4
	andc	r5, r6, r5
	blink	tr1, r63

Lcache_loop:
	ld.q	r3, 0, r63		/* Prefetch in advance */
	alloco	r7, 0			/* Allocate one cache-line in advance */
	ld.q	r3, -32, r19
	ld.q	r3, -24, r20
	ld.q	r3, -16, r21
	ld.q	r3, -8, r22
	st.q	r7, -32, r19		/* Copy the previous cache-line */
	st.q	r7, -24, r20
	st.q	r7, -16, r21
	st.q	r7, -8, r22
Lcache_enter:
	addi	r7, 32, r7		/* Next cache-line */
	addi	r3, 32, r3
	bne/l	r5, r7, tr2

	ld.q	r3, -32, r19
	ld.q	r3, -24, r20
	ld.q	r3, -16, r21
	ld.q	r3, -8, r22
	st.q	r7, -32, r19
	st.q	r7, -24, r20
	st.q	r7, -16, r21
	st.q	r7, -8, r22

	/*
	 * We have, at most, 31 bytes left to deal with.
	 */
Ltrailer:
	beq/u	r4, r63, tr0		/* Return to caller if done. */
	add	r4, r7, r8
	add	r4, r3, r9
	andi	r4, 0x18, r4
	add	r7, r4, r7
	add	r3, r4, r3
	xori	r4, 0x1f, r4
	addi	r4, 2, r4
	ptrel/l	r4, tr1
	blink	tr1, r63
	ld.q	r3, -24, r0
	st.q	r7, -24, r0
	ld.q	r3, -16, r0
	st.q	r7, -16, r0
	ld.q	r3, -8, r0
	st.q	r7, -8, r0
	ldhi.q	r9, -1, r0
	sthi.q	r8, -1, r0
	blink	tr0, r63


	/*
	 * Either the alignment of src/dest is shot to pieces, or we're
	 * dealing with a misaligned short buffer. Either way, do things
	 * the Slow Way.
	 */
Lbyte_copy:
	movi	0, r1
	pta/l	1f, tr1
1:	ldx.b	r3, r1, r0
	stx.b	r7, r1, r0
	addi	r1, 1, r1
	bne/l	r1, r4, tr1
	blink	tr0, r63
