/*	$NetBSD: icside_io_asm.S,v 1.2 1997/03/15 18:09:35 is Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
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
 * bus_space I/O functions for ICS IDE podule
 */

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

/*
 * read single
 */

	.global	_icside_r_1
_icside_r_1:
	ldrb	r0, [r1, r2, lsl #6]
	mov	pc, lr

	.global	_icside_r_2
_icside_r_2:
	ldr	r0, [r1, r2, lsl #6]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000
	mov	pc, lr

	.global	_icside_r_4
_icside_r_4:
	ldr	r0, [r1, r2, lsl #6]
	mov	pc, lr

	.global	_icside_r_8
_icside_r_8:
	adr	r0, icside_r_8_text
	b	_panic

icside_r_8_text:
	.asciz	"icside_r_8(%x,%x): Not implemented\n"
	.align	0

/*
 * write single
 */

	.global	_icside_w_1
_icside_w_1:
	strb	r3, [r1, r2, lsl #6]
	mov	pc, lr

	.global	_icside_w_2
_icside_w_2:
	mov	r3, r3, lsl #16
	orr	r3, r3, r3, lsr #16
	str	r3, [r1, r2, lsl #6]
	mov	pc, lr

	.global	_icside_w_4
_icside_w_4:
	str	r3, [r1, r2, lsl #6]
	mov	pc, lr

	.global	_icside_w_8
_icside_w_8:
	adr	r0, icside_w_8_text
	b	_panic

icside_w_8_text:
	.asciz	"icside_w_8(%x,%x,%x): Not implemented\n"
	.align	0

/*
 * read multiple
 */

	.global	_icside_rm_1
_icside_rm_1:
	adr	r0, icside_rm_1_text
	b	_panic

icside_rm_1_text:
	.asciz	"icside_rm_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_rm_2
_icside_rm_2:
	add	r0, r1, r2, lsl #6
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_inswm8

	.global	_icside_rm_4
_icside_rm_4:
	adr	r0, icside_rm_4_text
	b	_panic

icside_rm_4_text:
	.asciz	"icside_rm_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_rm_8
_icside_rm_8:
	adr	r0, icside_rm_8_text
	b	_panic

icside_rm_8_text:
	.asciz	"icside_rm_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * write multiple
 */

	.global	_icside_wm_1
_icside_wm_1:
	adr	r0, icside_wm_1_text
	b	_panic

icside_wm_1_text:
	.asciz	"icside_wm_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_wm_2
_icside_wm_2:
	add	r0, r1, r2, lsl #6
	mov	r1, r3
	ldr	r2, [sp, #0]
	b	_outswm8

	.global	_icside_wm_4
_icside_wm_4:
	adr	r0, icside_wm_4_text
	b	_panic

icside_wm_4_text:
	.asciz	"icside_wm_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_wm_8
_icside_wm_8:
	adr	r0, icside_wm_8_text
	b	_panic

icside_wm_8_text:
	.asciz	"icside_wm_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * read region
 */

	.global	_icside_rr_1
_icside_rr_1:
	adr	r0, icside_rr_1_text
	b	_panic

icside_rr_1_text:
	.asciz	"icside_rr_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_rr_2
_icside_rr_2:
	adr	r0, icside_rr_2_text
	b	_panic

icside_rr_2_text:
	.asciz	"icside_rr_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_rr_4
_icside_rr_4:
	adr	r0, icside_rr_4_text
	b	_panic

icside_rr_4_text:
	.asciz	"icside_rr_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_rr_8
_icside_rr_8:
	adr	r0, icside_rr_8_text
	b	_panic

icside_rr_8_text:
	.asciz	"icside_rr_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * write region
 */

	.global	_icside_wr_1
_icside_wr_1:
	adr	r0, icside_wr_1_text
	b	_panic

icside_wr_1_text:
	.asciz	"icside_wr_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_wr_2
_icside_wr_2:
	adr	r0, icside_wr_2_text
	b	_panic

icside_wr_2_text:
	.asciz	"icside_wr_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_wr_4
_icside_wr_4:
	adr	r0, icside_wr_4_text
	b	_panic

icside_wr_4_text:
	.asciz	"icside_wr_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_wr_8
_icside_wr_8:
	adr	r0, icside_wr_8_text
	b	_panic

icside_wr_8_text:
	.asciz	"icside_wr_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * set multiple
 */

	.global	_icside_sm_1
_icside_sm_1:
	adr	r0, icside_sm_1_text
	b	_panic

icside_sm_1_text:
	.asciz	"icside_sm_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_sm_2
_icside_sm_2:
	adr	r0, icside_sm_2_text
	b	_panic

icside_sm_2_text:
	.asciz	"icside_sm_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_sm_4
_icside_sm_4:
	adr	r0, icside_sm_4_text
	b	_panic

icside_sm_4_text:
	.asciz	"icside_sm_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_sm_8
_icside_sm_8:
	adr	r0, icside_sm_8_text
	b	_panic

icside_sm_8_text:
	.asciz	"icside_sm_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * set region
 */

	.global	_icside_sr_1
_icside_sr_1:
	adr	r0, icside_sr_1_text
	b	_panic

icside_sr_1_text:
	.asciz	"icside_sr_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_sr_2
_icside_sr_2:
	adr	r0, icside_sr_2_text
	b	_panic

icside_sr_2_text:
	.asciz	"icside_sr_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_sr_4
_icside_sr_4:
	adr	r0, icside_sr_4_text
	b	_panic

icside_sr_4_text:
	.asciz	"icside_sr_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_sr_8
_icside_sr_8:
	adr	r0, icside_sr_8_text
	b	_panic

icside_sr_8_text:
	.asciz	"icside_sr_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

/*
 * copy
 */

	.global	_icside_c_1
_icside_c_1:
	adr	r0, icside_c_1_text
	b	_panic

icside_c_1_text:
	.asciz	"icside_c_1(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_c_2
_icside_c_2:
	adr	r0, icside_c_2_text
	b	_panic

icside_c_2_text:
	.asciz	"icside_c_2(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_c_4
_icside_c_4:
	adr	r0, icside_c_4_text
	b	_panic

icside_c_4_text:
	.asciz	"icside_c_4(%x,%x,%x,%x): Not implemented\n"
	.align	0

	.global	_icside_c_8
_icside_c_8:
	adr	r0, icside_c_8_text
	b	_panic

icside_c_8_text:
	.asciz	"icside_c_8(%x,%x,%x,%x): Not implemented\n"
	.align	0

