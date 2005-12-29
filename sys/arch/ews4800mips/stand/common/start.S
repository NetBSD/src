/*	$NetBSD: start.S,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <mips/regdef.h>

	.set	noreorder
	.set	mips3
	.align	2
	.text
/*
 * Entry point.
 */
	.globl	start
start:
	nop
	nop
	nop
	nop
	la	sp, _ftext

	mfc0	t0, $12
	lui	t1, 0x0040	/* BEV : use IPL's exception vector. */
	and	t0, t0, t1
	mtc0	t0, $12
	nop
	la	t0, main
	move	a1, v0
	jr	t0
	 move	a2, v1		/* v1 = mainfo */
1:	b	1b
	 nop
	/* NOTREACHED */

#if 0 /* ROM putc test */
	li	a0, 50		/* x-pos */
2:	li	a2, 90		/* 'Z' */
	lui	v0, 0xbfc0
	ori	v0, 0xff60
	li	a1, 50		/* y-pos */
	jal	v0
	 nop
	b	2b
	 addiu	a0, a0, 12
	/* NOTREACHED */
#endif
#if 0 /* Frame buffer (TLB kseg2 mapped by IPL) test */
	move	a3, zero
	xori	a3, a3, 0x00
3:	sll	v0, a3, 0x18
	sra	a2, v0, 0x18
	lui	a0, 0xf000
	move	v1, zero
	lui	a1, 0x1
	ori	a1, a1, 0xffff
2:	sb	a2, 0(a0)
	addiu	v1, v1, 1
	slt	v0, a1, v1
	addiu	a0, a0, 1
	beqz	v0, 2b
	 nop
	xori	a3, a3, 0xff
	j	3b
	 nop
	/* NOTREACHED */
#endif

/*
 * void tlb_read(int index, void *)
 */
	.global	tlb_read
	.ent	tlb_read
tlb_read:
	mtc0	a0, $0		/* Index Register */
	nop; nop; nop; nop;
	tlbr
	nop; nop; nop; nop;
	mfc0	t0, $5		/* PageMask */
	nop; nop; nop; nop;
	mfc0	t1, $10		/* EntryHi */
	nop; nop; nop; nop;
	mfc0	t2, $2		/* EntryLo0 */
	nop; nop; nop; nop;
	mfc0	t3, $3		/* EntryLo1 */
	nop; nop; nop; nop;
	sw	t0,  0(a1)
	sw	t1,  4(a1)
	sw	t2,  8(a1)
	sw	t3, 12(a1)
	jr	ra
	 nop
	.end	tlb_read
