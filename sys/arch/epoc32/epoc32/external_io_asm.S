/*	$NetBSD: external_io_asm.S,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $	*/

/*
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

#include <machine/asm.h>

/*
 * bus_space I/O functions for external bus
 */

/*
 * read single
 */

ENTRY(external_bs_r_1)
	mov	r2, r2, lsl r0
	ldrb	r0, [r1, r2]
	mov	pc, lr

ENTRY(external_bs_r_4)
	mov	r2, r2, lsl r0
	ldr	r0, [r1, r2]
	mov	pc, lr

/*
 * write single
 */

ENTRY(external_bs_w_1)
	mov	r2, r2, lsl r0
	strb	r3, [r1, r2]
	mov	pc, lr

ENTRY(external_bs_w_4)
	mov	r2, r2, lsl r0
	str	r3, [r1, r2]
	mov	pc, lr

/*
 * read multiple
 */

ENTRY(external_bs_rm_1)
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #0]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

external_rm_1_loop:
	ldrb	r1, [r0]
	strb	r1, [r3], #0x0001
	subs	r2, r2, #0x00000001
	bgt	external_rm_1_loop

	mov	pc, lr

ENTRY(external_bs_rm_2)
	add	r0, r1, r2, lsl r0
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_C_LABEL(insw)

ENTRY(external_bs_rm_4)
	add	r0, r1, r2, lsl r0
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

1:	ldr	r3, [r0]
	str	r3, [r1], #4
	subs	r2, r2, #1
	bne	1b

	mov	pc, lr

/*
 * write multiple
 */

ENTRY(external_bs_wm_1)
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #0]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	movle	pc, lr

external_wm_1_loop:
	ldrb	r1, [r3], #0x0001
	strb	r1, [r0]
	subs	r2, r2, #0x00000001
	bgt	external_wm_1_loop

	mov	pc, lr

ENTRY(external_bs_wm_2)
	add	r0, r1, r2, lsl r0
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_C_LABEL(outsw)

ENTRY(external_bs_wm_4)
	add	r0, r1, r2, lsl r0
	mov	r1, r3
	ldr	r2, [sp, #0]
	teq	r2, #0
	moveq	pc, lr

1:	ldr	r3, [r1], #4
	str	r3, [r0]
	subs	r2, r2, #1
	bne	1b

	mov	pc, lr
