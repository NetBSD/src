/*	$NetBSD: obio_space_asm.S,v 1.1 2002/03/27 21:51:30 thorpej Exp $	*/

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
 * bus_space functions for IQ80321 on-board devices
 */

/*
 * Note these functions use ARM Architecture V4 instructions as
 * all IQ80321 boards have Intel 80321 XScale processors on them.
 */

/*
 * read single
 */

ENTRY(obio_bs_r_1)
	ldrb	r0, [r1, r2]
	mov	pc, lr

ENTRY(obio_bs_r_4)
	ldr	r0, [r1, r2] 
	mov	pc, lr

/*
 * write single
 */

ENTRY(obio_bs_w_1)
	strb	r3, [r1, r2]
	mov	pc, lr

ENTRY(obio_bs_w_4)
	str	r3, [r1, r2]
	mov	pc, lr

/*
 * read multiple
 */

ENTRY(obio_bs_rm_1)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lobio_bs_rm_1_loop:
	ldrb	r3, [r0]
	strb	r3, [r1], #1
	subs	r2, r2, #1
	bne	Lobio_bs_rm_1_loop

	mov	pc, lr

/*
 * write multiple
 */

ENTRY(obio_bs_wm_1)
	add	r0, r1, r2
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

Lobio_bs_wm_1_loop:
	ldrb	r3, [r1], #1
	strb	r3, [r0]
	subs	r2, r2, #1
	bne	Lobio_bs_wm_1_loop

	mov	pc, lr

/*
 * read region 1
 *	r0 - space
 *	r1 - handle
 *	r2 - offset
 *	r3 - *dest
 *	[stack #0] - count
 */
ENTRY(obio_bs_rr_1)
	add	r0, r1, r2	/* r0 now *src, r1 & r2 now free */
	mov	r1, r3		/* r1 now *dest, r3 now free */
	ldr	r2, [sp, #0]	/* r2 now count */
	teq	r2, #0		/* is count 0? */
	moveq	pc, lr		/* if so, exit */

Lobio_bs_rr_1_loop:
	ldrb	r3, [r0], #1	/* r3 = *src++ */
	strb	r3, [r1], #1	/* *dest++ = r3 */
	subs	r2, r2, #1	/* if --count != 0, go again */
	bne	Lobio_bs_rr_1_loop

	mov	pc, lr
