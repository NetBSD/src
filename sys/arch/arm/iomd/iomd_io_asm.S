/*	$NetBSD: iomd_io_asm.S,v 1.1.4.2 2001/10/05 22:27:42 reinoud Exp $	*/

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

#include <machine/asm.h>

/*
 * bus_space I/O functions for iomd
 */

/*
 * read single
 */

ENTRY(iomd_bs_r_1)
	ldrb	r0, [r1, r2, lsl #2]
	mov	pc, lr

ENTRY(iomd_bs_r_2)
	ldr	r0, [r1, r2, lsl #2]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000
	mov	pc, lr

ENTRY(iomd_bs_r_4)
	ldr	r0, [r1, r2, lsl #2]
	mov	pc, lr

/*
 * write single
 */

ENTRY(iomd_bs_w_1)
	strb	r3, [r1, r2, lsl #2]
	mov	pc, lr

ENTRY(iomd_bs_w_2)
	mov	r3, r3, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r1, r2, lsl #2]
	mov	pc, lr

ENTRY(iomd_bs_w_4)
	str	r3, [r1, r2, lsl #2]
	mov	pc, lr


/*
 * read multiple
 */

ENTRY(iomd_bs_rm_2)
	add	r0, r1, r2, lsl #2
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_C_LABEL(insw16)

/*
 * write multiple
 */

ENTRY(iomd_bs_wm_2)
	add	r0, r1, r2, lsl #2
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_C_LABEL(outsw16)
