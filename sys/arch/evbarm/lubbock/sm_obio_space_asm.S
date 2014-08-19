/*	$NetBSD: sm_obio_space_asm.S,v 1.4.122.1 2014/08/20 00:02:55 tls Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * These are special bus space functions for Lubbock's on-board I/O.
 * Especially for SMC91c96 chip in 8-bit mode.
 */	

#include <machine/asm.h>

RCSID("$NetBSD: sm_obio_space_asm.S,v 1.4.122.1 2014/08/20 00:02:55 tls Exp $")

/*
 * bus_space I/O functions with offset*4, 8-bit access.
 */

/*
 * read single
 */

ENTRY(smobio8_bs_r_2)
	add	r1, r1, r2, LSL #2
	ldrb	r0, [r1], #4
	ldrb	r2, [r1]
	orr	r0, r0, r2, LSL #8
	RET
END(smobio8_bs_r_2)

/*
 * write single
 */

ENTRY(smobio8_bs_w_2)
	add	r1, r1, r2, LSL #2
	strb	r3, [r1], #4
	mov	r3, r3, LSR #8
	strb	r3, [r1]
	RET
END(smobio8_bs_w_2)

/*
 * read multiple
 */
ENTRY(smobio8_bs_rm_2)
	add	r0, r1, r2, LSL #2
	ldr	r2, [sp, #0]
	cmp     r2, #0x00000000
	RETc(le)

Lbs_rm_2_loop:
	ldrb	r1, [r0]
	ldrb	lr, [r0, #4]
	subs	r2, r2, #0x00000001
	orr	r1, r1, lr, LSL #8
	strh	r1, [r3], #0x0002
	bgt	Lbs_rm_2_loop

	RET
END(smobio8_bs_rm_2)



/*
 * write multiple
 */
ENTRY(smobio8_bs_wm_2)
	add	r0, r1, r2, LSL #2
	ldr	r2, [sp, #0]
	cmp     r2, #0x00000000
	RETc(le)

Lbs_wm_2_loop:
	ldrh	r1, [r3], #0x0002
	subs	r2, r2, #0x00000001
	strb	r1, [r0]
	mov	r1, r1, LSR #8
	strb	r1, [r0,#4]
	bgt	Lbs_wm_2_loop

	RET
END(smobio8_bs_wm_2)


/*
 * For 16-bit mode
 */
	
/*
 * read single
 */

ENTRY(smobio16_bs_r_1)
	tst	r2, #1    /* Even/Odd ? */
	ldrbeq	r0, [r1, r2, LSL #2]
	RETc(eq)

	/* Odd byte.  read 16bits and get high byte */
	bic	r2, r2, #1
	add	r1, r1, r2, LSL #2
	ldrh	r0, [r1]
	mov	r0, r0, LSR #8
	RET
END(smobio16_bs_r_1)


/*
 * write single
 */

ENTRY(smobio16_bs_w_1)
	tst	r2, #1    /* Even/Odd ? */
	strbeq	r3, [r1, r2, LSL #2]
	RETc(eq)

	/* Odd byte.  write 16bit with low byte is 0. */
	bic	r2, r2, #1
	mov	r3, r3, LSL #8
	add	r1, r1, r2, LSL #2
	strh	r3, [r1]
	RET
END(smobio16_bs_w_1)
