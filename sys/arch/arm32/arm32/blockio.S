/*	$NetBSD: blockio.S,v 1.8.12.1 1999/12/27 18:31:37 wrstuden Exp $	*/

/*
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * blockio.S
 *
 * optimised block read/write from/to IO routines.
 *
 * Created      : 08/10/94
 */

#include <machine/asm.h>

/*
 * Reads short ints (16 bits) from an I/O address into a block of memory
 *
 * r0 = address to read from (IO)
 * r1 = address to write to (memory)
 * r2 = length
 */

ENTRY(insw)
/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

/* If the destination address and the size is word aligned, do it fast */

	tst	r2, #0x00000001
	tsteq	r1, #0x00000003
	beq	fastinsw

/* Non aligned insw */

inswloop:
	ldr	r3, [r0]
	strb	r3, [r1], #0x0001
	mov	r3, r3, lsr #8
	strb	r3, [r1], #0x0001
	subs	r2, r2, #0x00000001
	bgt	inswloop

	mov	pc, lr

/* Word aligned insw */

fastinsw:
	stmfd	sp!, {r4}

fastinswloop:
	ldr	r3, [r0, #0x0002]	/* take advantage of nonaligned
					 * word accesses */
	ldr	r4, [r0]
	mov	r3, r3, lsr #16		/* Put the two shorts together */
	orr	r3, r3, r4, lsl #16
	str	r3, [r1], #0x0004	/* Store */
	subs	r2, r2, #0x00000002	/* Next */
	bgt	fastinswloop

	ldmfd	sp!, {r4}

	mov	pc, lr


/*
 * Writes short ints (16 bits) from a block of memory to an I/O address
 *
 * r0 = address to write to (IO)
 * r1 = address to read from (memory)
 * r2 = length
 */

ENTRY(outsw)
/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

/* If the destination address and the size is word aligned, do it fast */

	tst	r2, #0x00000001
	tsteq	r1, #0x00000003
	beq	fastoutsw

/* Non aligned outsw */

	stmfd	sp!, {r4}

outswloop:
	ldrb	r3, [r1], #0x0001
	ldrb	r4, [r1], #0x0001
	orr	r3, r3, r4, lsl #8
	orr	r3, r3, r3, lsl #16
	str	r3, [r0]
	subs	r2, r2, #0x00000001
	bgt	outswloop

	ldmfd	sp!, {r4}

	mov	pc, lr

/* Word aligned outsw */

fastoutsw:
	stmfd	sp!, {r4}

fastoutswloop:
	ldr	r3, [r1], #0x0004

	mov	r4, r3, lsl #16
	orr	r4, r4, r4, lsr #16
	str	r4, [r0]

	mov	r4, r3, lsr #16
	orr	r4, r4, r4, lsl #16
	str	r4, [r0]

	subs	r2, r2, #0x00000002
	bgt	fastoutswloop

	ldmfd	sp!, {r4}

	mov	pc, lr

/*
 * reads short ints (16 bits) from an I/O address into a block of memory
 * with a length garenteed to be a multiple of 16 bytes
 * with a word aligned destination address
 *
 * r0 = address to read from (IO)
 * r1 = address to write to (memory)
 * r2 = length
 */

ENTRY(insw16)
/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

/* If the destination address is word aligned and the size suitably aligned, do it fast */

	tst	r2, #0x00000007
	tsteq	r1, #0x00000003

	bne	_C_LABEL(insw)

/* Word aligned insw */

	stmfd	sp!, {r4-r7}

insw16loop:
	ldr	r3, [r0, #0x0002]	/* take advantage of nonaligned
					 * word accesses */
	ldr	r7, [r0]
	mov	r3, r3, lsr #16		/* Put the two shorts together */
	orr	r3, r3, r7, lsl #16

	ldr	r4, [r0, #0x0002]	/* take advantage of nonaligned
					 * word accesses */
	ldr	r7, [r0]
	mov	r4, r4, lsr #16		/* Put the two shorts together */
	orr	r4, r4, r7, lsl #16

	ldr	r5, [r0, #0x0002]	/* take advantage of nonaligned
					 * word accesses */
	ldr	r7, [r0]
	mov	r5, r5, lsr #16		/* Put the two shorts together */
	orr	r5, r5, r7, lsl #16

	ldr	r6, [r0, #0x0002]	/* take advantage of nonaligned
					 * word accesses */
	ldr	r7, [r0]
	mov	r6, r6, lsr #16		/* Put the two shorts together */
	orr	r6, r6, r7, lsl #16

	stmia	r1!, {r3-r6}
	subs	r2, r2, #0x00000008	/* Next */
	bgt	insw16loop

	ldmfd	sp!, {r4-r7}

	mov	pc, lr


/*
 * Writes short ints (16 bits) from a block of memory to an I/O address
 *
 * r0 = address to write to (IO)
 * r1 = address to read from (memory)
 * r2 = length
 */

ENTRY(outsw16)
/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

/* If the destination address is word aligned and the size suitably aligned, do it fast */

	tst	r2, #0x00000007
	tsteq	r1, #0x00000003

	bne	_C_LABEL(outsw)

/* Word aligned outsw */

	stmfd	sp!, {r4-r7}

outsw16loop:
	ldmia	r1!, {r4-r7}

	mov	r3, r4, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r0]

	mov	r3, r4, lsr #16
	orr	r3, r3, r3, lsl #16
	str	r3, [r0]

	mov	r3, r5, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r0]

	mov	r3, r5, lsr #16
	orr	r3, r3, r3, lsl #16
	str	r3, [r0]

	mov	r3, r6, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r0]

	mov	r3, r6, lsr #16
	orr	r3, r3, r3, lsl #16
	str	r3, [r0]

	mov	r3, r7, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r0]

	mov	r3, r7, lsr #16
	orr	r3, r3, r3, lsl #16
	str	r3, [r0]

	subs	r2, r2, #0x00000008
	bgt	outsw16loop

	ldmfd	sp!, {r4-r7}

	mov	pc, lr

/*
 * reads short ints (16 bits) from an I/O address into a block of memory
 * The I/O address is assumed to be mapped multiple times in a block of
 * 8 words.
 * The destination address should be word aligned.
 *
 * r0 = address to read from (IO)
 * r1 = address to write to (memory)
 * r2 = length
 */

ENTRY(inswm8)
/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

/* If the destination address is word aligned and the size suitably aligned, do it fast */

	tst	r1, #0x00000003

	bne	_C_LABEL(insw)

/* Word aligned insw */

	stmfd	sp!, {r4-r11}

	mov	r11, #0xff000000
	orr	r11, r11, #0x00ff0000

inswm8_loop8:
	cmp	r2, #8
	bcc	inswm8_l8

	ldmia	r0, {r3-r10}

	bic	r3, r3, r11
	orr	r3, r3, r4, lsl #16
	bic	r5, r5, r11
	orr	r4, r5, r6, lsl #16
	bic	r7, r7, r11
	orr	r5, r7, r8, lsl #16
	bic	r9, r9, r11
	orr	r6, r9, r10, lsl #16

	stmia	r1!, {r3-r6}

	subs	r2, r2, #0x00000008	/* Next */
	bne	inswm8_loop8
	beq	inswm8_l1

inswm8_l8:
	cmp	r2, #4
	bcc	inswm8_l4

	ldmia	r0, {r3-r6}

	bic	r3, r3, r11
	orr	r3, r3, r4, lsl #16
	bic	r5, r5, r11
	orr	r4, r5, r6, lsl #16

	stmia	r1!, {r3-r4}

	subs	r2, r2, #0x00000004
	beq	inswm8_l1

inswm8_l4:
	cmp	r2, #2
	bcc	inswm8_l2

	ldmia	r0, {r3-r4}

	bic	r3, r3, r11
	orr	r3, r3, r4, lsl #16
	str	r3, [r1], #0x0004

	subs	r2, r2, #0x00000002
	beq	inswm8_l1

inswm8_l2:
	cmp	r2, #1
	bcc	inswm8_l1

	ldr	r3, [r0]

	strb	r3, [r1], #0x0001
	mov	r3, r3, lsr #8
	strb	r3, [r1], #0x0001

	subs	r2, r2, #0x00000001

inswm8_l1:
	ldmfd	sp!, {r4-r11}

	mov	pc, lr

/*
 * write short ints (16 bits) to an I/O address from a block of memory
 * The I/O address is assumed to be mapped multiple times in a block of
 * 8 words.
 * The source address should be word aligned.
 *
 * r0 = address to read to (IO)
 * r1 = address to write from (memory)
 * r2 = length
 */

ENTRY(outswm8)
/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

/* If the destination address is word aligned and the size suitably aligned, do it fast */

	tst	r1, #0x00000003

	bne	_C_LABEL(outsw)

/* Word aligned outsw */

	stmfd	sp!, {r4-r10}

outswm8_loop8:
	cmp	r2, #8
	bcc	outswm8_l8

	ldmia	r1!, {r3,r5,r7,r9}

	mov	r4, r3, lsr #16
	orr	r4, r4, r4, lsl #16
	mov	r3, r3, lsl #16
	orr	r3, r3, r3, lsr #16
	mov	r6, r5, lsr #16
	orr	r6, r6, r6, lsl #16
	mov	r5, r5, lsl #16
	orr	r5, r5, r5, lsr #16
	mov	r8, r7, lsr #16
	orr	r8, r8, r8, lsl #16
	mov	r7, r7, lsl #16
	orr	r7, r7, r7, lsr #16
	mov	r10, r9, lsr #16
	orr	r10, r10, r10, lsl #16
	mov	r9, r9, lsl #16
	orr	r9, r9, r9, lsr #16

	stmia	r0, {r3-r10}

	subs	r2, r2, #0x00000008	/* Next */
	bne	outswm8_loop8
	beq	outswm8_l1

outswm8_l8:
	cmp	r2, #4
	bcc	outswm8_l4

	ldmia	r1!, {r3-r4}

	mov	r5, r3, lsl #16
	orr	r5, r5, r5, lsr #16
	mov	r6, r3, lsr #16
	orr	r6, r6, r6, lsl #16
	mov	r7, r4, lsl #16
	orr	r7, r7, r7, lsr #16
	mov	r8, r4, lsr #16
	orr	r8, r8, r8, lsl #16

	stmia	r0, {r5-r8}

	subs	r2, r2, #0x00000004
	beq	outswm8_l1

outswm8_l4:
	cmp	r2, #2
	bcc	outswm8_l2

	ldr	r3, [r1], #0x0004

	mov	r4, r3, lsl #16
	orr	r4, r4, r4, lsr #16
	mov	r5, r3, lsr #16
	orr	r5, r5, r5, lsl #16

	stmia	r0, {r4, r5}

	subs	r2, r2, #0x00000002
	beq	outswm8_l1

outswm8_l2:
	cmp	r2, #1
	bcc	outswm8_l1

	ldrb	r3, [r1], #0x0001
	ldrb	r4, [r1], #0x0001
	orr	r3, r3, r4, lsl #8
	orr	r3, r3, r3, lsl #16
	str	r3, [r0]

	subs	r2, r2, #0x00000001

outswm8_l1:
	ldmfd	sp!, {r4-r10}

	mov	pc, lr

