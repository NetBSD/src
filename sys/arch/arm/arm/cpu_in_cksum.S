/*	$NetBSD: cpu_in_cksum.S,v 1.2.2.2 2008/02/04 09:21:44 yamt Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
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
 * Hand-optimised in_cksum() and in4_cksum() implementations for ARM/Xscale
 */

#include <machine/asm.h>
RCSID("$NetBSD: cpu_in_cksum.S,v 1.2.2.2 2008/02/04 09:21:44 yamt Exp $")
	
#include "assym.h"

/*
 * int cpu_in_cksum(struct mbuf *m, int len, int off, uint32_t initial_sum)
 *
 * Entry:
 *	r0	m
 *	r1	len
 *	r2	off
 *	r3	initial_sum
 *
 * Function wide register usage
 *	r8	accumulated sum
 *	r9	remaining length to parse
 *	ip	pointer to next mbuf
 */
/* LINTSTUB: Func: int cpu_in_cksum(struct mbuf *, int, int, uint32_t) */
ENTRY(cpu_in_cksum)
	stmfd	sp!, {r4-r11,lr}
	
	mov	r8, r3			/* Accumulate sum in r8 */
	mov	r9, r1			/* save len in r9 */
	mov	ip, r0			/* set ip to the current mbuf */

.Lin_cksum_skip_loop:
	ldr	r1, [ip, #(M_LEN)]
	ldr	r0, [ip, #(M_DATA)]
	ldr	ip, [ip, #(M_NEXT)]
.Lin_cksum_skip_entry:
	subs	r2, r2, r1		/* offset = offset - mbuf length */
	blt	.Lin_cksum_skip_done	/* if offset has gone negative start with this mbuf */
	cmp	ip, #0x00
	bne	.Lin_cksum_skip_loop
	b	.Lin_cksum_whoops

.Lin_cksum_skip_done:
	add	r0, r2, r0		/* data += offset (offset is < 0) */ 
	add	r0, r0, r1		/* data += length of mbuf */
					/* data == start of data to cksum */
	rsb	r1, r2, #0x00		/* length = remainder of mbuf to read */
	mov	r10, #0x00
	b	.Lin_cksum_entry

.Lin_cksum_loop:
	ldr	r1, [ip, #(M_LEN)]
	ldr	r0, [ip, #(M_DATA)]
	ldr	ip, [ip, #(M_NEXT)]
.Lin_cksum_entry:
	cmp	r9, r1
	movlt	r1, r9
	sub	r9, r9, r1
	eor	r11, r10, r0
	add	r10, r10, r1
	adds	r2, r1, #0x00
	blne	_ASM_LABEL(L_cksumdata)
	tst	r11, #0x01
	movne	r2, r2, ror #8
	adds	r8, r8, r2
	adc	r8, r8, #0x00
	cmp	ip, #00
	bne	.Lin_cksum_loop
	
	mov	r1, #0xff
	orr	r1, r1, #0xff00
	and	r0, r8, r1
	add	r0, r0, r8, lsr #16
	add	r0, r0, r0, lsr #16
	and	r0, r0, r1
	eor	r0, r0, r1
	ldmfd	sp!, {r4-r11, pc}

.Lin_cksum_whoops:
	adr	r0, .Lin_cksum_whoops_str
	bl	_C_LABEL(panic)
.Lin_cksum_whoops_str:
	.asciz	"in_cksum: out of mbufs\n"
	.align	5


/*
 * The main in*_cksum() workhorse...
 *
 * Entry parameters:
 *	r0	Pointer to buffer
 *	r1	Buffer length
 *	lr	Return address
 *
 * Returns:
 *	r2	Accumulated 32-bit sum
 *
 * Clobbers:
 *	r0-r7
 */
/* LINTSTUB: Ignore */
ASENTRY_NP(L_cksumdata)
#ifdef __PROG26
	str	lr, [sp, #-4]!		/* for SVC26 mode */
#endif
#ifdef __XSCALE__
	pld	[r0]			/* Pre-fetch the start of the buffer */
#endif
	mov	r2, #0

	/* We first have to word-align the buffer.  */
	ands	r7, r0, #0x03
	beq	.Lcksumdata_wordaligned
	rsb	r7, r7, #0x04
	cmp	r1, r7			/* Enough bytes left to make it? */
	blt	.Lcksumdata_endgame
	cmp	r7, #0x02
	ldrb	r4, [r0], #0x01		/* Fetch 1st byte */
	ldrgeb	r5, [r0], #0x01		/* Fetch 2nd byte */
	movlt	r5, #0x00
	ldrgtb	r6, [r0], #0x01		/* Fetch 3rd byte */
	movle	r6, #0x00
	/* Combine the three bytes depending on endianness and alignment */
#ifdef __ARMEB__
	orreq	r2, r5, r4, lsl #8
	orreq	r2, r2, r6, lsl #24
	orrne	r2, r4, r5, lsl #8
	orrne	r2, r2, r6, lsl #16
#else
	orreq	r2, r4, r5, lsl #8
	orreq	r2, r2, r6, lsl #16
	orrne	r2, r5, r4, lsl #8
	orrne	r2, r2, r6, lsl #24
#endif
	subs	r1, r1, r7		/* Update length */
#ifdef __PROG26
	ldreq	pc, [sp], #4		/* All done? */
#else
	moveq	pc, lr			/* All done? */
#endif

	/* Buffer is now word aligned */
.Lcksumdata_wordaligned:
#ifdef __XSCALE__
	cmp	r1, #0x04		/* Less than 4 bytes left? */
	blt	.Lcksumdata_endgame	/* Yup */

	/* Now quad-align, if necessary */
	ands	r7, r0, #0x04
	ldrne	r7, [r0], #0x04
	subne	r1, r1, #0x04
	subs	r1, r1, #0x40
	blt	.Lcksumdata_bigloop_end	/* Note: C flag clear if branch taken */

	/*
	 * Buffer is now quad aligned. Sum 64 bytes at a time.
	 * Note: First ldrd is hoisted above the loop, together with
	 * setting r6 to zero to avoid stalling for results in the
	 * loop. (r7 is live, from above).
	 */
	ldrd	r4, [r0], #0x08
	mov	r6, #0x00
.Lcksumdata_bigloop:
	pld	[r0, #0x18]
	adds	r2, r2, r6
	adcs	r2, r2, r7
	ldrd	r6, [r0], #0x08
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldrd	r4, [r0], #0x08
	adcs	r2, r2, r6
	adcs	r2, r2, r7
	ldrd	r6, [r0], #0x08
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldrd	r4, [r0], #0x08
	adcs	r2, r2, r6
	adcs	r2, r2, r7
	pld	[r0, #0x18]
	ldrd	r6, [r0], #0x08
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldrd	r4, [r0], #0x08
	adcs	r2, r2, r6
	adcs	r2, r2, r7
	ldrd	r6, [r0], #0x08
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	adc	r2, r2, #0x00
	subs	r1, r1, #0x40
	ldrged	r4, [r0], #0x08
	bge	.Lcksumdata_bigloop

	adds	r2, r2, r6		/* r6/r7 still need summing */
.Lcksumdata_bigloop_end:
	adcs	r2, r2, r7
	adc	r2, r2, #0x00

#else	/* !__XSCALE__ */

	subs	r1, r1, #0x40
	blt	.Lcksumdata_bigloop_end

.Lcksumdata_bigloop:
	ldmia	r0!, {r3, r4, r5, r6}
	adds	r2, r2, r3
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldmia	r0!, {r3, r4, r5, r7}
	adcs	r2, r2, r6
	adcs	r2, r2, r3
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldmia	r0!, {r3, r4, r5, r6}
	adcs	r2, r2, r7
	adcs	r2, r2, r3
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldmia	r0!, {r3, r4, r5, r7}
	adcs	r2, r2, r6
	adcs	r2, r2, r3
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	adcs	r2, r2, r7
	adc	r2, r2, #0x00
	subs	r1, r1, #0x40
	bge	.Lcksumdata_bigloop
.Lcksumdata_bigloop_end:
#endif

	adds	r1, r1, #0x40
#ifdef __PROG26
	ldreq	pc, [sp], #4
#else
	moveq	pc, lr
#endif
	cmp	r1, #0x20

#ifdef __XSCALE__
	ldrged	r4, [r0], #0x08		/* Avoid stalling pld and result */
	blt	.Lcksumdata_less_than_32
	pld	[r0, #0x18]
	ldrd	r6, [r0], #0x08
	adds	r2, r2, r4
	adcs	r2, r2, r5
	ldrd	r4, [r0], #0x08
	adcs	r2, r2, r6
	adcs	r2, r2, r7
	ldrd	r6, [r0], #0x08
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	adcs	r2, r2, r6		/* XXX: Unavoidable result stall */
	adcs	r2, r2, r7
#else
	blt	.Lcksumdata_less_than_32
	ldmia	r0!, {r3, r4, r5, r6}
	adds	r2, r2, r3
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	ldmia	r0!, {r3, r4, r5, r7}
	adcs	r2, r2, r6
	adcs	r2, r2, r3
	adcs	r2, r2, r4
	adcs	r2, r2, r5
	adcs	r2, r2, r7
#endif
	adc	r2, r2, #0x00
	subs	r1, r1, #0x20
#ifdef __PROG26
	ldreq	pc, [sp], #4
#else
	moveq	pc, lr
#endif

.Lcksumdata_less_than_32:
	/* There are less than 32 bytes left */
	and	r3, r1, #0x18
	rsb	r4, r3, #0x18
	sub	r1, r1, r3
	adds	r4, r4, r4, lsr #1	/* Side effect: Clear carry flag */
	addne	pc, pc, r4

/*
 * Note: We use ldm here, even on Xscale, since the combined issue/result
 * latencies for ldm and ldrd are the same. Using ldm avoids needless #ifdefs.
 */
	/* At least 24 bytes remaining... */
	ldmia	r0!, {r4, r5}
	nop
	adcs	r2, r2, r4
	adcs	r2, r2, r5

	/* At least 16 bytes remaining... */
	ldmia	r0!, {r4, r5}
	adcs	r2, r2, r4
	adcs	r2, r2, r5

	/* At least 8 bytes remaining... */
	ldmia	r0!, {r4, r5}
	adcs	r2, r2, r4
	adcs	r2, r2, r5

	/* Less than 8 bytes remaining... */
	adc	r2, r2, #0x00
	subs	r1, r1, #0x04
	blt	.Lcksumdata_lessthan4

	ldr	r4, [r0], #0x04
	sub	r1, r1, #0x04
	adds	r2, r2, r4
	adc	r2, r2, #0x00

	/* Deal with < 4 bytes remaining */
.Lcksumdata_lessthan4:
	adds	r1, r1, #0x04
#ifdef __PROG26
	ldreq	pc, [sp], #4
#else
	moveq	pc, lr
#endif

	/* Deal with 1 to 3 remaining bytes, possibly misaligned */
.Lcksumdata_endgame:
	ldrb	r3, [r0]		/* Fetch first byte */
	cmp	r1, #0x02
	ldrgeb	r4, [r0, #0x01]		/* Fetch 2nd and 3rd as necessary */
	movlt	r4, #0x00
	ldrgtb	r5, [r0, #0x02]
	movle	r5, #0x00
	/* Combine the three bytes depending on endianness and alignment */
	tst	r0, #0x01
#ifdef __ARMEB__
	orreq	r3, r4, r3, lsl #8
	orreq	r3, r3, r5, lsl #24
	orrne	r3, r3, r4, lsl #8
	orrne	r3, r3, r5, lsl #16
#else
	orreq	r3, r3, r4, lsl #8
	orreq	r3, r3, r5, lsl #16
	orrne	r3, r4, r3, lsl #8
	orrne	r3, r3, r5, lsl #24
#endif
	adds	r2, r2, r3
	adc	r2, r2, #0x00
#ifdef __PROG26
	ldr	pc, [sp], #4
#else
	mov	pc, lr
#endif
