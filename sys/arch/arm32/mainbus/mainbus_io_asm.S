/*	$NetBSD: mainbus_io_asm.S,v 1.1 1997/01/13 00:37:53 mark Exp $	*/

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

/*
 * bus_space I/O functions for mainbus
 */

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

/*
 * read single
 */

	.global	_mainbus_r_1
_mainbus_r_1:
	ldrb	r0, [r1, r2, lsl #2]
	mov	pc, lr

	.global	_mainbus_r_2
_mainbus_r_2:
	ldr	r0, [r1, r2, lsl #2]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000
	mov	pc, lr

	.global	_mainbus_r_4
_mainbus_r_4:
	ldr	r0, [r1, r2, lsl #2]
	mov	pc, lr

	.global	_mainbus_r_8
_mainbus_r_8:
	adr	r0, mainbus_r_8_text
	b	_panic

mainbus_r_8_text:
	.asciz	"mainbus_r_8(%x,%x): Not implemented\n"
	.align	0

/*
 * write single
 */

	.global	_mainbus_w_1
_mainbus_w_1:
	strb	r3, [r1, r2, lsl #2]
	mov	pc, lr

	.global	_mainbus_w_2
_mainbus_w_2:
	mov	r3, r3, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r1, r2, lsl #2]
	mov	pc, lr

	.global	_mainbus_w_4
_mainbus_w_4:
	str	r3, [r1, r2, lsl #2]
	mov	pc, lr

	.global	_mainbus_w_8
_mainbus_w_8:
	adr	r0, mainbus_w_8_text
	b	_panic

mainbus_w_8_text:
	.asciz	"mainbus_w_8(%x,%x): Not implemented\n"
	.align	0

/*
 * read multiple
 */

	.global	_mainbus_rm_1
_mainbus_rm_1:
	adr	r0, mainbus_rm_1_text
	b	_panic

mainbus_rm_1_text:
	.asciz	"mainbus_rm_1(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_rm_2
_mainbus_rm_2:
	add	r0, r1, r2, lsl #2
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_insw16

	.global	_mainbus_rm_4
_mainbus_rm_4:
	adr	r0, mainbus_rm_4_text
	b	_panic

mainbus_rm_4_text:
	.asciz	"mainbus_rm_4(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_rm_8
_mainbus_rm_8:
	adr	r0, mainbus_rm_8_text
	b	_panic

mainbus_rm_8_text:
	.asciz	"mainbus_rm_8(%x,%x,%x): Not implemented\n"
	.align	0

/*
 * write multiple
 */

	.global	_mainbus_wm_1
_mainbus_wm_1:
	adr	r0, mainbus_wm_1_text
	b	_panic

mainbus_wm_1_text:
	.asciz	"mainbus_wm_1(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_wm_2
_mainbus_wm_2:
	add	r0, r1, r2, lsl #2
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_outsw16

	.global	_mainbus_wm_4
_mainbus_wm_4:
	adr	r0, mainbus_wm_4_text
	b	_panic

mainbus_wm_4_text:
	.asciz	"mainbus_wm_4(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_wm_8
_mainbus_wm_8:
	adr	r0, mainbus_wm_8_text
	b	_panic

mainbus_wm_8_text:
	.asciz	"mainbus_wm_8(%x,%x,%x): Not implemented\n"
	.align	0

/*
 * read region
 */

	.global	_mainbus_rr_1
_mainbus_rr_1:
	adr	r0, mainbus_rr_1_text
	b	_panic

mainbus_rr_1_text:
	.asciz	"mainbus_rr_1(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_rr_2
_mainbus_rr_2:
	adr	r0, mainbus_rr_2_text
	b	_panic

mainbus_rr_2_text:
	.asciz	"mainbus_rr_2(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_rr_4
_mainbus_rr_4:
	adr	r0, mainbus_rr_4_text
	b	_panic

mainbus_rr_4_text:
	.asciz	"mainbus_rr_4(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_rr_8
_mainbus_rr_8:
	adr	r0, mainbus_rr_8_text
	b	_panic

mainbus_rr_8_text:
	.asciz	"mainbus_rr_8(%x,%x,%x): Not implemented\n"
	.align	0

/*
 * write region
 */

	.global	_mainbus_wr_1
_mainbus_wr_1:
	adr	r0, mainbus_wr_1_text
	b	_panic

mainbus_wr_1_text:
	.asciz	"mainbus_wr_1(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_wr_2
_mainbus_wr_2:
	adr	r0, mainbus_wr_2_text
	b	_panic

mainbus_wr_2_text:
	.asciz	"mainbus_wr_2(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_wr_4
_mainbus_wr_4:
	adr	r0, mainbus_wr_4_text
	b	_panic

mainbus_wr_4_text:
	.asciz	"mainbus_wr_4(%x,%x,%x): Not implemented\n"
	.align	0

	.global	_mainbus_wr_8
_mainbus_wr_8:
	adr	r0, mainbus_wr_8_text
	b	_panic

mainbus_wr_8_text:
	.asciz	"mainbus_wr_8(%x,%x,%x): Not implemented\n"
	.align	0

