	/* $NetBSD: vidc_mc.S,v 1.1.4.2 2001/10/05 22:27:46 reinoud Exp $ */

/*
 * Copyright (c) 1994-1995 Melvyn Tang-Richardson
 * Copyright (c) 1994-1995 RiscBSD kernel team
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
 *	This product includes software developed by the RiscBSD kernel team
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RISCBSD TEAM ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * vidc_mc.S
 *
 * Console assembly functions
 *
 * Created      : 17/09/94
 */

#include <machine/asm.h>

/*
 * On entry:
 *	r0 = addr
 *	r1 = font
 *	r2 = col
 *	r3 = xres
 */

/* Optimised for 8x16 font at the moment */

ENTRY(vidcrendermc_render)
	stmfd	sp!, {r0-r9, lr}

	/* Generate colour mask */

	mov	r9, r2, lsr #16
	mov	r4, r2, lsr #8
	and	r9, r9, #0xff
	and	r4, r4, #0xff
	and	r2, r2, #0xff

	/* For each row (8 pixels) */

Lvidcrendermc_render_loop:
	ldrb	r5, [r1], #1

	mov	r7, #0
	mov	r8, #0

	/* This loop will get *SO* much faster with the new algorithmn */

	tst	r5, #8
	orrne	r7, r2, r7, lsl #8
	orreq	r7, r4, r7, lsl #8
	tst	r5, #4
	orrne	r7, r2, r7, lsl #8
	orreq	r7, r4, r7, lsl #8
	tst	r5, #2
	orrne	r7, r2, r7, lsl #8
	orreq	r7, r4, r7, lsl #8
	tst	r5, #1
	orrne	r7, r2, r7, lsl #8
	orreq	r7, r4, r7, lsl #8
	tst	r5, #0x80
	orrne	r8, r2, r8, lsl #8
	orreq	r8, r4, r8, lsl #8
	tst	r5, #0x40
	orrne	r8, r2, r8, lsl #8
	orreq	r8, r4, r8, lsl #8
	tst	r5, #0x20
	orrne	r8, r2, r8, lsl #8
	orreq	r8, r4, r8, lsl #8
	tst	r5, #0x10
	orrne	r8, r2, r8, lsl #8
	orreq	r8, r4, r8, lsl #8

	stmia	r0, {r7, r8}
	add	r0, r0, r3

	subs	r9, r9, #1
	bne	Lvidcrendermc_render_loop

	ldmfd	sp!, {r0-r9, pc}
	
/*
 * On entry:
 *	r0 = start
 *	r1 = end
 *	r2 = col
 */

ENTRY(vidcrendermc_cls)
	stmfd	sp!, {r0-r9, lr}
	
	mov	r3, r2
	mov	r4, r2
	mov	r5, r2
	mov	r6, r2
	mov	r7, r2
	mov	r8, r2
	mov	r9, r2

Lvidcrendermc_cls_loop:
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	stmia	r0!, {r2-r9}
	
	cmp	r0, r1
	ble	Lvidcrendermc_cls_loop

	ldmfd	sp!, {r0-r9, pc}
