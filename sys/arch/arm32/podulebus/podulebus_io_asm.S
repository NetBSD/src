/*	$NetBSD: podulebus_io_asm.S,v 1.3 1997/07/30 23:52:08 mark Exp $	*/

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
 * bus_space I/O functions for podulebus
 */

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

/*
 * read single
 */

	.global	_podulebus_r_1
_podulebus_r_1:
	mov	r2, r2, lsl r0
	ldrb	r0, [r1, r2]
	mov	pc, lr

	.global	_podulebus_r_2
_podulebus_r_2:
	mov	r2, r2, lsl r0
	ldr	r0, [r1, r2]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000
	mov	pc, lr

	.global	_podulebus_r_4
_podulebus_r_4:
	mov	r2, r2, lsl r0
	ldr	r0, [r1, r2]
	mov	pc, lr

	.global	_podulebus_r_8
_podulebus_r_8:
	adr	r0, podulebus_r_8_text
	b	_panic

podulebus_r_8_text:
	.asciz	"podulebus_r_8(%x,%x): Not implemented\n"
	.align	0

/*
 * write single
 */

	.global	_podulebus_w_1
_podulebus_w_1:
	mov	r2, r2, lsl r0
	strb	r3, [r1, r2]
	mov	pc, lr

	.global	_podulebus_w_2
_podulebus_w_2:
	mov	r3, r3, lsl #16
	orr	r3, r3, r3, lsr #16
	mov	r2, r2, lsl r0
	str	r3, [r1, r2]
	mov	pc, lr

	.global	_podulebus_w_4
_podulebus_w_4:
	mov	r2, r2, lsl r0
	str	r3, [r1, r2]
	mov	pc, lr

	.global	_podulebus_w_8
_podulebus_w_8:
	adr	r0, podulebus_w_8_text
	b	_panic

podulebus_w_8_text:
	.asciz	"podulebus_w_8(%x,%x,%x): Not implemented\n"
	.align	0

/*
 * read multiple
 */

	.global	_podulebus_rm_1
_podulebus_rm_1:
	adr	r0, podulebus_rm_1_text
	b	_panic

podulebus_rm_1_text:
	.asciz	"podulebus_rm_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_rm_2
_podulebus_rm_2:
	add	r0, r1, r2, lsl r0
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_insw

	.global	_podulebus_rm_4
_podulebus_rm_4:
	adr	r0, podulebus_rm_4_text
	b	_panic

podulebus_rm_4_text:
	.asciz	"podulebus_rm_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_rm_8
_podulebus_rm_8:
	adr	r0, podulebus_rm_8_text
	b	_panic

podulebus_rm_8_text:
	.asciz	"podulebus_rm_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * write multiple
 */

	.global	_podulebus_wm_1
_podulebus_wm_1:
	adr	r0, podulebus_wm_1_text
	b	_panic

podulebus_wm_1_text:
	.asciz	"podulebus_wm_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_wm_2
_podulebus_wm_2:
	add	r0, r1, r2, lsl r0
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_outsw

	.global	_podulebus_wm_4
_podulebus_wm_4:
	adr	r0, podulebus_wm_4_text
	b	_panic

podulebus_wm_4_text:
	.asciz	"podulebus_wm_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_wm_8
_podulebus_wm_8:
	adr	r0, podulebus_wm_8_text
	b	_panic

podulebus_wm_8_text:
	.asciz	"podulebus_wm_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * read region
 */

	.global	_podulebus_rr_1
_podulebus_rr_1:
	adr	r0, podulebus_rr_1_text
	b	_panic

podulebus_rr_1_text:
	.asciz	"podulebus_rr_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_rr_2
_podulebus_rr_2:
	adr	r0, podulebus_rr_2_text
	b	_panic

podulebus_rr_2_text:
	.asciz	"podulebus_rr_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_rr_4
_podulebus_rr_4:
	adr	r0, podulebus_rr_4_text
	b	_panic

podulebus_rr_4_text:
	.asciz	"podulebus_rr_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_rr_8
_podulebus_rr_8:
	adr	r0, podulebus_rr_8_text
	b	_panic

podulebus_rr_8_text:
	.asciz	"podulebus_rr_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * write region
 */

	.global	_podulebus_wr_1
_podulebus_wr_1:
	adr	r0, podulebus_wr_1_text
	b	_panic

podulebus_wr_1_text:
	.asciz	"podulebus_wr_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_wr_2
_podulebus_wr_2:
	adr	r0, podulebus_wr_2_text
	b	_panic

podulebus_wr_2_text:
	.asciz	"podulebus_wr_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_wr_4
_podulebus_wr_4:
	adr	r0, podulebus_wr_4_text
	b	_panic

podulebus_wr_4_text:
	.asciz	"podulebus_wr_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_wr_8
_podulebus_wr_8:
	adr	r0, podulebus_wr_8_text
	b	_panic

podulebus_wr_8_text:
	.asciz	"podulebus_wr_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * set multiple
 */

	.global	_podulebus_sm_1
_podulebus_sm_1:
	adr	r0, podulebus_sm_1_text
	b	_panic

podulebus_sm_1_text:
	.asciz	"podulebus_sm_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_sm_2
_podulebus_sm_2:
	adr	r0, podulebus_sm_2_text
	b	_panic

podulebus_sm_2_text:
	.asciz	"podulebus_sm_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_sm_4
_podulebus_sm_4:
	adr	r0, podulebus_sm_4_text
	b	_panic

podulebus_sm_4_text:
	.asciz	"podulebus_sm_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_sm_8
_podulebus_sm_8:
	adr	r0, podulebus_sm_8_text
	b	_panic

podulebus_sm_8_text:
	.asciz	"podulebus_sm_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * set region
 */

	.global	_podulebus_sr_1
_podulebus_sr_1:
	adr	r0, podulebus_sr_1_text
	b	_panic

podulebus_sr_1_text:
	.asciz	"podulebus_sr_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_sr_2
_podulebus_sr_2:
	adr	r0, podulebus_sr_2_text
	b	_panic

podulebus_sr_2_text:
	.asciz	"podulebus_sr_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_sr_4
_podulebus_sr_4:
	adr	r0, podulebus_sr_4_text
	b	_panic

podulebus_sr_4_text:
	.asciz	"podulebus_sr_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_sr_8
_podulebus_sr_8:
	adr	r0, podulebus_sr_8_text
	b	_panic

podulebus_sr_8_text:
	.asciz	"podulebus_sr_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * copy
 */

	.global	_podulebus_c_1
_podulebus_c_1:
	adr	r0, podulebus_c_1_text
	b	_panic

podulebus_c_1_text:
	.asciz	"podulebus_c_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_c_2
_podulebus_c_2:
	adr	r0, podulebus_c_2_text
	b	_panic

podulebus_c_2_text:
	.asciz	"podulebus_c_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_c_4
_podulebus_c_4:
	adr	r0, podulebus_c_4_text
	b	_panic

podulebus_c_4_text:
	.asciz	"podulebus_c_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_podulebus_c_8
_podulebus_c_8:
	adr	r0, podulebus_c_8_text
	b	_panic

podulebus_c_8_text:
	.asciz	"podulebus_c_8(%x,%x,%x,%x): Not implemented\n"
	.align	0
