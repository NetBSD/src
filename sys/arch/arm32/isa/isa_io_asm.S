/*	$NetBSD: isa_io_asm.S,v 1.2 1998/06/28 07:27:53 thorpej Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * bus_space I/O functions for isa
 */

#include <machine/asm.h>

fp	.req	r11
ip	.req	r12
sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

/*
 * read single
 */

ENTRY(isa_bs_r_1)
	ldrb	r0, [r1, r2]
#ifdef GPROF
	nop ; nop ; nop ; nop ; nop
#endif
	mov	pc, lr

ENTRY(isa_bs_r_2)
	add	r0, r1, r2
/*	ldrh	r0, [r0] */	.word	0xe1d000b0
#ifdef GPROF
	nop ; nop ; nop ; nop ; nop
#endif
	mov	pc, lr

ENTRY(isa_bs_r_4)
	ldr	r0, [r1, r2]
#ifdef GPROF
	nop ; nop ; nop ; nop ; nop
#endif
	mov	pc, lr
/*
 * read multiple.
 *
 * cloned from assembly generated for:
 * 
 *	void
 *	isa_bs_rm_4(v, h, o, p, s)
 *		void *v;
 *		bus_space_handle_t h;
 *		bus_size_t o, s;
 *		u_int32_t *p;
 *	{
 *		u_int32_t *port, *lim;
 *
 *		port = (u_in32_t *)(h + o);
 *		lim = p + s;
 *
 *		while (p < lim)
 *			*p++ = *port;;
 *	}
 */

ENTRY(isa_bs_rm_1)
	mov     ip, sp
        stmfd   sp!, {fp, ip, lr, pc}
        mov     r0, r3
        add     r1, r1, r2
        sub     fp, ip, #4
        ldr     r3, [fp, #4]
        add     ip, r0, r3
        cmp     r0, ip
        ldmcsea fp, {fp, sp, pc}
Lisa_rm_1_loop:
        ldrb     r3, [r1, #0]
        strb     r3, [r0], #1
        cmp     r0, ip
        bcc     Lisa_rm_1_loop
        ldmea   fp, {fp, sp, pc}

ENTRY(isa_bs_rm_2)
	mov     ip, sp
        stmfd   sp!, {fp, ip, lr, pc}
        mov     r0, r3
        add     r1, r1, r2
        sub     fp, ip, #4
        ldr     r3, [fp, #4]
        add     ip, r0, r3, asl #1
        cmp     r0, ip
        ldmcsea fp, {fp, sp, pc}
Lisa_rm_2_loop:	
	/* ldrh	r3, [r1] */	.word 0xe1d130b0
	/* strh	r3, [r0], #2 */	.word 0xe0c030b2
        cmp     r0, ip
        bcc     Lisa_rm_2_loop
        ldmea   fp, {fp, sp, pc}

ENTRY(isa_bs_rm_4)
	mov     ip, sp
        stmfd   sp!, {fp, ip, lr, pc}
        mov     r0, r3
        add     r1, r1, r2
        sub     fp, ip, #4
        ldr     r3, [fp, #4]
        add     ip, r0, r3, asl #2
        cmp     r0, ip
        ldmcsea fp, {fp, sp, pc}
Lisa_rm_4_loop:
        ldr     r3, [r1, #0]
        str     r3, [r0], #4
        cmp     r0, ip
        bcc     Lisa_rm_4_loop
        ldmea   fp, {fp, sp, pc}

/*
 * read region.
 *
 * cloned from assembly generated for:
 *
 *	void
 *	isa_bs_wr_4(v, h, o, p, s)
 *		void *v;
 *		bus_space_handle_t h;
 *		bus_size_t o, s;
 *		u_int32_t *p;
 *	{
 *		u_int32_t *cur, *lim;
 *	
 *		cur = (u_in32_t *)(h + o);
 *		lim = cur + s;
 *	
 *		while (cur < lim)
 *			*cur++ = *p++;
 *	}
 *
 */

ENTRY(isa_bs_rr_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	mov	r0, r3
	add	r1, r1, r2
	sub	fp, ip, #4
	ldr	r3, [fp, #4]
	add	r2, r1, r3
	cmp	r1, r2
	ldmcsea	fp, {fp, sp, pc}
Lisa_rr_1_loop:
	ldrb	r3, [r1], #1
	strb	r3, [r0], #1
	cmp	r1, r2
	bcc	Lisa_rr_1_loop
	ldmea	fp, {fp, sp, pc}

ENTRY(isa_bs_rr_2)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	mov	r0, r3
	add	r1, r1, r2
	sub	fp, ip, #4
	ldr	r3, [fp, #4]
	add	r2, r1, r3, asl #1
	cmp	r1, r2
	ldmcsea	fp, {fp, sp, pc}
Lisa_rr_2_loop:
	/* ldrh	r3, [r1], #2 */	.word 0xe0d130b2
	/* strh	r3, [r0], #2 */	.word 0xe0c030b2
	cmp	r1, r2
	bcc	Lisa_rr_2_loop
	ldmea	fp, {fp, sp, pc}

ENTRY(isa_bs_rr_4)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	mov	r0, r3
	add	r1, r1, r2
	sub	fp, ip, #4
	ldr	r3, [fp, #4]
	add	r2, r1, r3, asl #2
	cmp	r1, r2
	ldmcsea	fp, {fp, sp, pc}
Lisa_rr_4_loop:
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	cmp	r1, r2
	bcc	Lisa_rr_4_loop
	ldmea	fp, {fp, sp, pc}

/*
 * write single
 */

ENTRY(isa_bs_w_1)
	strb	r3, [r1, r2]
#ifdef GPROF
	nop ; nop ; nop ; nop ; nop
#endif
	mov	pc, lr

ENTRY(isa_bs_w_2)
	add	r0, r1, r2
/*	strh	r3, [r0] */	.word	0xe1c030b0
#ifdef GPROF
	nop ; nop ; nop ; nop ; nop
#endif
	mov	pc, lr

ENTRY(isa_bs_w_4)
	str	r3, [r1, r2]
#ifdef GPROF
	nop ; nop ; nop ; nop ; nop
#endif
	mov	pc, lr

/* write multiple
 *
 * cloned from assembly generated for:
 *
 *	void
 *	isa_bs_wm_4(v, h, o, p, s)
 *		void *v;
 *		bus_space_handle_t h;
 *		bus_size_t o, s;
 *		u_int32_t *p;
 *	{
 *		u_int32_t *port, *lim;
 *
 *		port = (u_in32_t *)(h + o);
 *		lim = p + s;
 *
 *		while (p < lim)
 *			*port = *p++;
 *	}
 */
ENTRY(isa_bs_wm_1)
        mov     ip, sp
        stmfd   sp!, {fp, ip, lr, pc}
        mov     r0, r3
        add     r1, r1, r2
        sub     fp, ip, #4
        ldr     r3, [fp, #4]
        add     ip, r0, r3
        cmp     r0, ip
        ldmcsea fp, {fp, sp, pc}
Lisa_wm_1_loop:
        ldrb     r3, [r0], #1
        strb     r3, [r1]
        cmp     r0, ip
        bcc     Lisa_wm_1_loop
        ldmea   fp, {fp, sp, pc}

ENTRY(isa_bs_wm_2)
        mov     ip, sp
        stmfd   sp!, {fp, ip, lr, pc}
        mov     r0, r3
        add     r1, r1, r2
        sub     fp, ip, #4
        ldr     r3, [fp, #4]
        add     ip, r0, r3, asl #1
        cmp     r0, ip
        ldmcsea fp, {fp, sp, pc}
Lisa_wm_2_loop:	
	/* ldrh	r3, [r0], #2 */	.word 0xe0d030b2
	/* strh	r3, [r1] */	.word 0xe1c130b0
        cmp     r0, ip
        bcc     Lisa_wm_2_loop
        ldmea   fp, {fp, sp, pc}

ENTRY(isa_bs_wm_4)
        mov     ip, sp
        stmfd   sp!, {fp, ip, lr, pc}
        mov     r0, r3
        add     r1, r1, r2
        sub     fp, ip, #4
        ldr     r3, [fp, #4]
        add     ip, r0, r3, asl #2
        cmp     r0, ip
        ldmcsea fp, {fp, sp, pc}
Lisa_wm_4_loop:
        ldr     r3, [r0], #4
        str     r3, [r1]
        cmp     r0, ip
        bcc     Lisa_wm_4_loop
        ldmea   fp, {fp, sp, pc}

/*
 * write region.
 *
 * cloned from assembly generated for:
 *
 *	void
 *	isa_bs_wr_4(v, h, o, p, s)
 *		void *v;
 *		bus_space_handle_t h;
 *		bus_size_t o, s;
 *		u_int32_t *p;
 *	{
 *		u_int32_t *cur, *lim;
 *	
 *		cur = (u_in32_t *)(h + o);
 *		lim = cur + s;
 *	
 *		while (cur < lim)
 *			*cur++ = *p++;
 *	}
 *
 */

ENTRY(isa_bs_wr_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	mov	r0, r3
	add	r1, r1, r2
	sub	fp, ip, #4
	ldr	r3, [fp, #4]
	add	r2, r1, r3
	cmp	r1, r2
	ldmcsea	fp, {fp, sp, pc}
Lisa_wr_1_loop:
	ldrb	r3, [r0], #1
	strb	r3, [r1], #1
	cmp	r1, r2
	bcc	Lisa_wr_1_loop
	ldmea	fp, {fp, sp, pc}

ENTRY(isa_bs_wr_2)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	mov	r0, r3
	add	r1, r1, r2
	sub	fp, ip, #4
	ldr	r3, [fp, #4]
	add	r2, r1, r3, asl #1
	cmp	r1, r2
	ldmcsea	fp, {fp, sp, pc}
Lisa_wr_2_loop:
	/* ldrh	r3, [r0], #2 */	.word 0xe0d030b2
	/* strh	r3, [r1], #2 */	.word 0xe0c130b2
	cmp	r1, r2
	bcc	Lisa_wr_2_loop
	ldmea	fp, {fp, sp, pc}

ENTRY(isa_bs_wr_4)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	mov	r0, r3
	add	r1, r1, r2
	sub	fp, ip, #4
	ldr	r3, [fp, #4]
	add	r2, r1, r3, asl #2
	cmp	r1, r2
	ldmcsea	fp, {fp, sp, pc}
Lisa_wr_4_loop:
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	cmp	r1, r2
	bcc	Lisa_wr_4_loop
	ldmea	fp, {fp, sp, pc}
