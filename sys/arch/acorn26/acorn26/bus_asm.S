/*	$NetBSD: bus_asm.S,v 1.2 2006/10/01 22:47:18 bjh21 Exp $	*/

/*
 * Copyright (c) 2006 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
 * bus_space I/O functions for podulebus
 */

/*
 * read single
 */

ENTRY(iobus_bs_r_1)
	mov	r2, r2, lsl r0
	ldrb	r0, [r1, r2]
	mov	pc, lr

ENTRY(iobus_bs_r_2)
	mov	r2, r2, lsl r0
	ldr	r0, [r1, r2]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000
	mov	pc, lr

/*
 * write single
 */

ENTRY(iobus_bs_w_1)
	mov	r2, r2, lsl r0
	strb	r3, [r1, r2]
	mov	pc, lr

ENTRY(iobus_bs_w_2)
	orr	r3, r3, r3, lsl #16
	mov	r2, r2, lsl r0
	str	r3, [r1, r2]
	mov	pc, lr

/*
 * read multiple
 */

ENTRY(iobus_bs_rm_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldrb	r1, [r0]
	strb	r1, [r3], #0x0001
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

ENTRY(iobus_bs_rm_2)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldr	r1, [r0]
	strb	r1, [r3], #0x0001
	mov	r1, r1, lsr #8
	strb	r1, [r3], #0x0001
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

/*
 * write multiple
 */

ENTRY(iobus_bs_wm_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldrb	r1, [r3], #0x0001
	strb	r1, [r0]
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

ENTRY(iobus_bs_wm_2)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldrb	r1, [r3], #0x0001
	ldrb	r12, [r3], #1
	orr	r1, r1, r12, lsl #8
	orr	r1, r1, r1, lsl #16
	str	r1, [r0]
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

/*
 * read region
 */

ENTRY(iobus_bs_rr_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r12, r1, r2, lsl r0
	mov	r2, #1
	mov	r0, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldrb	r1, [r12], r0
	strb	r1, [r3], #0x0001
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

	/*
	 * r0-r7: scratch
	 * r8: I/O addr
	 * r9: mem addr
	 * r10: count
	 * r12: 0xffff
	 */
ENTRY(iobus_bs_rr_2)
	mov	ip, sp
	stmfd	sp!, {r4-r10, fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r8, r1, r2, lsl r0
	mov	r9, r3
	mov	r2, #1
	mov	r0, r2, lsl r0
	ldr	r10, [sp, #44]
	tst	r9, #3		/* If we're word-aligned */
	teqeq	r0, #4		/* and registers are every 4 bytes */
	bne	2f
	cmp	r10, #8		/* and we're reading >=8 registers */
	bge	3f		/* do it the fast way. */

	/* Make sure that we have a positive length */
2:	cmp	r10, #0
	ldmledb	fp, {r4-r10, fp, sp, pc}

1:	ldr	r1, [r8], r0
	strb	r1, [r9], #1
	mov	r1, r1, lsr #8
	strb	r1, [r9], #1
	subs	r10, r10, #1
	bgt	1b

	ldmdb	fp, {r4-r10, fp, sp, pc}

	/*
	 * Fast read_region_2 code.  This is at its best when dealing with
	 * 16-byte-aligned blocks of memory, which should happen quite
	 * a lot anyway, but the above code could help.
	 */
3:	mov	r12, #0x00ff
	orr	r12, r12, #0xff00
	sub	r10, r10, #7
1:	ldmia	r8!, {r0-r7}
	subs	r10, r10, #8
	and	r0, r0, r12
	and	r2, r2, r12
	and	r4, r4, r12
	and	r6, r6, r12
	orr	r0, r0, r1, lsl #16
	orr	r2, r2, r3, lsl #16
	orr	r4, r4, r5, lsl #16
	orr	r6, r6, r7, lsl #16
	stmia	r9!, {r0, r2, r4, r6}
	bgt	1b
	adds	r10, r10, #7
	ldmeqdb	fp, {r4-r10, fp, sp, pc}
	mov	r0, #4
	b	2b

/*
 * write region
 */

ENTRY(iobus_bs_wr_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r12, r1, r2, lsl r0
	mov	r2, #1
	mov	r0, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldrb	r1, [r3], #0x0001
	strb	r1, [r12], r0
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

ENTRY(iobus_bs_wr_2)
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r12, r1, r2, lsl r0
	mov	r2, #1
	mov	r0, r2, lsl r0
	ldr	r2, [sp, #20]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	ldrb	r1, [r3], #0x0001
	ldrb	r4, [r3], #1
	orr	r1, r1, r4, lsl #8
	orr	r1, r1, r1, lsl #16
	str	r1, [r12], r0
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {r4, fp, sp, pc}

/*
 * set multiple
 */

ENTRY(iobus_bs_sm_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	strb	r3, [r0]
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

ENTRY(iobus_bs_sm_2)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r0, r1, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

	orr	r3, r3, r3, lsl #16
1:
	str	r3, [r0]
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

/*
 * set region
 */

ENTRY(iobus_bs_sr_1)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r12, r1, r2, lsl r0
	mov	r2, #1
	mov	r0, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

1:
	strb	r3, [r12], r0
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}

ENTRY(iobus_bs_sr_2)
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	add	r12, r1, r2, lsl r0
	mov	r2, #1
	mov	r0, r2, lsl r0
	ldr	r2, [sp, #16]

	/* Make sure that we have a positive length */
	cmp	r2, #0x00000000
	ldmledb	fp, {fp, sp, pc}

	orr	r3, r3, r3, lsl #16
1:
	str	r3, [r12], r0
	subs	r2, r2, #0x00000001
	bgt	1b

	ldmdb	fp, {fp, sp, pc}
