/*	$NetBSD: ifpga_io_asm.S,v 1.1 2001/10/27 16:19:08 rearnsha Exp $ */

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
 *
 * From arm/footbridge/footbridge_io_asm.S
 */

#include <machine/asm.h>

/*
 * bus_space I/O functions for the IFPGA
 */

/*
 * Note these functions use ARM Architecture V4 instructions as
 * all IFPGA based systems will be using processors that support
 * V4 or later architectures
 */

/*
 * read single
 */

ENTRY(ifpga_bs_r_1)
	ldrb	r0, [r1, r2]
	mov	pc, lr

ENTRY(ifpga_bs_r_2)
	ldrh	r0, [r1, r2]
	mov	pc, lr

ENTRY(ifpga_bs_r_4)
	ldr	r0, [r1, r2]
	mov	pc, lr

/*
 * write single
 */

ENTRY(ifpga_bs_w_1)
	strb	r3, [r1, r2]
	mov	pc, lr

ENTRY(ifpga_bs_w_2)
	strh	r3, [r1, r2]
	mov	pc, lr

ENTRY(ifpga_bs_w_4)
	str	r3, [r1, r2]
	mov	pc, lr

/*
 * read multiple
 */

ENTRY(ifpga_bs_rm_1)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_rm_1_loop:
	ldrb	r3, [r0]
	strb	r3, [r1], #1
	subs	r2, r2, #1
	bne	Lifpga_bs_rm_1_loop

	mov	pc, lr

ENTRY(ifpga_bs_rm_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_rm_2_loop:
	ldrh	r3, [r0]
	strh	r3, [r1], #2
	subs	r2, r2, #1
	bne	Lifpga_bs_rm_2_loop

	mov	pc, lr

ENTRY(ifpga_bs_rm_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_rm_4_loop:
	ldr	r3, [r0]
	str	r3, [r1], #4
	subs	r2, r2, #1
	bne	Lifpga_bs_rm_4_loop

	mov	pc, lr

/*
 * write multiple
 */

ENTRY(ifpga_bs_wm_1)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_wm_1_loop:
	ldrb	r3, [r1], #1
	strb	r3, [r0]
	subs	r2, r2, #1
	bne	Lifpga_bs_wm_1_loop

	mov	pc, lr

ENTRY(ifpga_bs_wm_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_wm_2_loop:
	ldrh	r3, [r1], #2
	strh	r3, [r0]
	subs	r2, r2, #1
	bne	Lifpga_bs_wm_2_loop

	mov	pc, lr

ENTRY(ifpga_bs_wm_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_wm_4_loop:
	ldr	r3, [r1], #4
	str	r3, [r0]
	subs	r2, r2, #1
	bne	Lifpga_bs_wm_4_loop

	mov	pc, lr

/*
 * read region
 */

ENTRY(ifpga_bs_rr_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_rr_2_loop:
	ldrh	r3, [r0], #2
	strh	r3, [r1], #2
	subs	r2, r2, #1
	bne	Lifpga_rr_2_loop

	mov	pc, lr

ENTRY(ifpga_bs_rr_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_rr_4_loop:
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	subs	r2, r2, #1
	bne	Lifpga_rr_4_loop

	mov	pc, lr

/*
 * write region.
 */

ENTRY(ifpga_bs_wr_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_wr_2_loop:	
	ldrh	r3, [r1], #2
	strh	r3, [r0], #2
	subs	r2, r2, #1
	bne	Lifpga_wr_2_loop

	mov	pc, lr

ENTRY(ifpga_bs_wr_4)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_wr_4_loop:	
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	subs	r2, r2, #1
	bne	Lifpga_wr_4_loop

	mov	pc, lr

/*
 * set region
 */

ENTRY(ifpga_bs_sr_2)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lifpga_bs_sr_2_loop:
	strh	r1, [r0], #2
	subs	r2, r2, #1
	bne	Lifpga_bs_sr_2_loop

	mov	pc, lr

/*
 * copy region
 */

ENTRY(ifpga_bs_c_2)
	add	r0, r1, r2
	ldr	r2, [sp, #0]
	add	r1, r2, r3
	ldr	r2, [sp, #4]
	teq	r2, #0
	moveq	pc, lr

	cmp	r0, r1
	blt	Lifpga_bs_c_2_backwards

Lifpga_bs_cf_2_loop:
	ldrh	r3, [r0], #2
	strh	r3, [r1], #2
	subs	r2, r2, #1
	bne	Lifpga_bs_cf_2_loop

	mov	pc, lr

Lifpga_bs_c_2_backwards:
	add	r0, r0, r2, lsl #1
	add	r1, r1, r2, lsl #1
	sub	r0, r0, #2
	sub	r1, r1, #2

Lifpga_bs_cb_2_loop:
	ldrh	r3, [r0], #-2
	strh	r3, [r1], #-2
	subs	r2, r2, #1
	bne	Lifpga_bs_cb_2_loop

	mov	pc, lr
