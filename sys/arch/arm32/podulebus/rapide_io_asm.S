/*	$NetBSD: rapide_io_asm.S,v 1.2 1997/03/15 18:09:41 is Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
 * All rights reserved.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * bus_space I/O functions for Yellowstone RapIDE podule
 *
 * These are optimised 32 bit transfer routines
 */

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

	.global	_rapide_rm_4
_rapide_rm_4:
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	add	r0, r1, r2
	mov	r1, r3

	/* Test length */
	ldr	r2, [sp, #0]
	tst	r2, #0x7f
	beq	rapide_rm_4_m128
	tst	r2, #0x07
	beq	rapide_rm_4_m8

	/* xfer 4 bytes at a time */
rapide_rm_4_loop:
	ldr	r3, [r0]
	str	r3, [r1], #0x0004
	subs	r2, r2, #1
	bne	rapide_rm_4_loop

	mov	pc, r14

rapide_rm_4_m8:
	/* xfer 32 bytes at a time */
	stmfd	sp!, {r4-r10}

rapide_rm_4_m8_loop:
	ldmia	r0, {r3-r10}
	stmia	r1!, {r3-r10}
	subs	r2, r2, #8
	bne	rapide_rm_4_m8_loop

	ldmfd	sp!, {r4-r10}
	mov	pc, r14

rapide_rm_4_m128:
	/* xfer 512 bytes at a time */
	stmfd	sp!, {r4-r12, r14}

rapide_rm_4_m128_loop:
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r12, r14}
	stmia	r1!, {r3-r12, r14}
	ldmia	r0, {r3-r9}
	stmia	r1!, {r3-r9}
	subs	r2, r2, #128
	bne	rapide_rm_4_m128_loop

	ldmfd	sp!, {r4-r12, pc}

	.global	_rapide_wm_4
_rapide_wm_4:
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	tst	r2, #0x7f
	beq	rapide_wm_4_m128
	tst	r2, #0x07
	beq	rapide_wm_4_m8

	/* xfer 4 bytes at a time */
rapide_wm_4_loop:
	ldr	r3, [r1], #0x0004
	str	r3, [r0]
	subs	r2, r2, #1
	bne	rapide_wm_4_loop

	mov	pc, r14

rapide_wm_4_m8:
	/* xfer 32 bytes at a time */
	stmfd	sp!, {r4-r10}

rapide_wm_4_m8_loop:
	ldmia	r1!, {r3-r10}
	stmia	r0, {r3-r10}
	subs	r2, r2, #8
	bne	rapide_wm_4_m8_loop

	ldmfd	sp!, {r4-r10}
	mov	pc, r14

rapide_wm_4_m128:
	/* xfer 512 bytes at a time */
	stmfd	sp!, {r4-r12}

rapide_wm_4_m128_loop:
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r12}
	stmia	r0, {r3-r12}
	ldmia	r1!, {r3-r10}
	stmia	r0, {r3-r10}
	subs	r2, r2, #128
	bne	rapide_wm_4_m128_loop

	ldmfd	sp!, {r4-r12}
	mov	pc, r14
