/*	$NetBSD: footbridge_io_asm.S,v 1.3.18.1 2000/06/26 05:35:38 mark Exp $	*/

/*
 * Copyright (c) 1997 Causality Limited.
 * Copyright (c) 1997 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
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

#include <machine/asm.h>

/*
 * bus_space I/O functions for the footbridge
 */

/*
 * Note these functions use ARM Architecture V4 instructions as
 * all footbridge based systems will be using processors that support
 * V4 or later architectures
 */

/*
 * read single
 */

ENTRY(footbridge_bs_r_1)
	ldrb	r0, [r1, r2]
	mov	pc, lr

ENTRY(footbridge_bs_r_2)
	ldrh	r0, [r1, r2]	/*.word 0xe19100b2*/
	mov	pc, lr

ENTRY(footbridge_bs_r_4)
	ldr	r0, [r1, r2]
	mov	pc, lr

/*
 * write single
 */

ENTRY(footbridge_bs_w_1)
	strb	r3, [r1, r2]
	mov	pc, lr

ENTRY(footbridge_bs_w_2)
	strh	r3, [r1, r2]	/*.word 0xe18130b2*/
	mov	pc, lr

ENTRY(footbridge_bs_w_4)
	str	r3, [r1, r2]
	mov	pc, lr

/*
 * read multiple
 */

ENTRY(footbridge_bs_rm_1)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_rm_1_loop:
	ldrb	r3, [r0]
	strb	r3, [r1], #1
	subs	r2, r2, #1
	bne	Lfootbridge_bs_rm_1_loop

	mov	pc, lr

ENTRY(footbridge_bs_rm_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_rm_2_loop:
	ldrh	r3, [r0]	/*.word 0xe1d030b0*/
	strh	r3, [r1], #2	/*.word 0xe0c130b2*/
	subs	r2, r2, #1
	bne	Lfootbridge_bs_rm_2_loop

	mov	pc, lr

ENTRY(footbridge_bs_rm_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_rm_4_loop:
	ldr	r3, [r0]
	str	r3, [r1], #4
	subs	r2, r2, #1
	bne	Lfootbridge_bs_rm_4_loop

	mov	pc, lr

/*
 * write multiple
 */

ENTRY(footbridge_bs_wm_1)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_wm_1_loop:
	ldrb	r3, [r1], #1
	strb	r3, [r0]
	subs	r2, r2, #1
	bne	Lfootbridge_bs_wm_1_loop

	mov	pc, lr

ENTRY(footbridge_bs_wm_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_wm_2_loop:
	ldrh	r3, [r1], #2	/*.word 0xe0d130b2*/
	strh	r3, [r0]	/*.word 0xe1c030b0*/
	subs	r2, r2, #1
	bne	Lfootbridge_bs_wm_2_loop

	mov	pc, lr

ENTRY(footbridge_bs_wm_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_wm_4_loop:
	ldr	r3, [r1], #4
	str	r3, [r0]
	subs	r2, r2, #1
	bne	Lfootbridge_bs_wm_4_loop

	mov	pc, lr

/*
 * read region
 */

ENTRY(footbridge_bs_rr_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_rr_2_loop:
	ldrh	r3, [r0], #2
	strh	r3, [r1], #2	/*.word 0xe0c130b2*/
	subs	r2, r2, #1
	bne	Lfootbridge_rr_2_loop

	mov	pc, lr

ENTRY(footbridge_bs_rr_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_rr_4_loop:
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	subs	r2, r2, #1
	bne	Lfootbridge_rr_4_loop

	mov	pc, lr

/*
 * write region.
 */

ENTRY(footbridge_bs_wr_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_wr_2_loop:	
	ldrh	r3, [r1], #2	/*.word 0xe0d130b2*/
	strh	r3, [r0], #2
	subs	r2, r2, #1
	bne	Lfootbridge_wr_2_loop

	mov	pc, lr

ENTRY(footbridge_bs_wr_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_wr_4_loop:	
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	subs	r2, r2, #1
	bne	Lfootbridge_wr_4_loop

	mov	pc, lr

/*
 * set region
 */

ENTRY(footbridge_bs_sr_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lfootbridge_bs_sr_2_loop:
	strh	r1, [r0], #2		/*.word e0c010b2*/
	subs	r2, r2, #1
	bne	Lfootbridge_bs_sr_2_loop

	mov	pc, lr

/*
 * copy region
 */

ENTRY(footbridge_bs_c_2)
	add	r0, r1, r2
	ldr	r2, [sp, #0]
	add	r1, r2, r3
	ldr	r2, [sp, #4]
	teq	r2, #0
	moveq	pc, lr

	cmp	r0, r1
	blt	Lfootbridge_bs_c_2_backwards

Lfootbridge_bs_cf_2_loop:
	ldrh	r3, [r0], #2
	strh	r3, [r1], #2
	subs	r2, r2, #1
	bne	Lfootbridge_bs_cf_2_loop

	mov	pc, lr

Lfootbridge_bs_c_2_backwards:
	add	r0, r0, r2, lsl #1
	add	r1, r1, r2, lsl #1
	sub	r0, r0, #2
	sub	r1, r1, #2

Lfootbridge_bs_cb_2_loop:
	ldrh	r3, [r0], #-2
	strh	r3, [r1], #-2
	subs	r2, r2, #1
	bne	Lfootbridge_bs_cb_2_loop

	mov	pc, lr
