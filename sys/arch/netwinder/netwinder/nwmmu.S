/*	$NetBSD: nwmmu.S,v 1.2 2001/04/20 16:09:48 matt Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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
#incude "assym.h"
#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/pte.h>

	.text

ENTRY_NP(kernel_text)

ASENTRY_NP(nwstart)
	mov	r6, #0x7c000000
	add	r6, r6, #0x3f8
	mov	r3, #0
	mov	r3, #(AP_KRW << AP_SECTION_SHIFT)
	add	r3, r3, #(L1_SECTION|PT_CACHEABLE)
	mov	r2, #0x100000		/* advance by 1MB */
	mov	r1, #0x8000		/* page table start */
	mov	r0, #0x1000		/* page table size */

L1:
	str	r3, [r1], #0x0004
	add	r3, r3, r2
	subs	r0, r0, #1
	bgt	L1			/* done */

/*
 * Page Table now maps 1:1 to physical memory.
 */
	mov	r3, #(AP_KRW << AP_SECTION_SHIFT)
	add	r3, r3, #(L1_SECTION|PT_CACHEABLE)
	mov	r2, #0x100000		/* advance by 1MB */
	mov	r1, #0x8000		/* page table start */
	add	r1, r1, #(0x100 * 4)	/* offset to 0x100xxxxx */
	mov	r0, #256
L2:
	str	r3, [r1], #0x0004	/* 0x10000000-0x1fffffff */
	add	r3, r3, r2
	subs	r0, r0, #1
	bgt	L2
/*
 * 0x10000000-0x1fffffff now maps to 0x00000000-0x0fffffff
 */
	mov	r3, #(AP_KRW << AP_SECTION_SHIFT)
	add	r3, r3, #(L1_SECTION|PT_CACHEABLE)
	mov	r1, #0x8000		/* page table start */
	add	r1, r1, #(0xf00 * 4)	/* offset to 0xf03xxxxx */
	mov	r0, #64
L3:
	str	r3, [r1], #0x0004	/* 0xf000000-0xf03fffff */
	add	r3, r3, r2
	subs	r0, r0, #1
	bgt	L3
/*
 * 0xf0000000-0xf3ffffff now maps to 0x00000000-0x03ffffff
 */
	mov	r1, #0x8000		/* page table start */
	add	r1, r1, #(0xfd0 * 4)	/* DC21285_PCI_IO_VBASE */
	add	r1, r1, #8
	mov	r3, #(AP_KRW << AP_SECTION_SHIFT)
	add	r3, r3, #L1_SECTION
	add	r3, r3, #0x7c000000	/* DC21285_PCI_IO_BASE */
	str	r3, [r1]		/* 0xfd20000-0xfd2fffff */
/*
 * 0xfd200000-0xfd2fffff now maps to 0x7c000000-0x7c0fffff
 */


	/* set the location of the L1 page table */
	mov	r1, #0x8000
	mcr	15, 0, r1, c2, c0, 0

	mov	r2, #0x2a
	strb	r2, [r6]

	/* set mmu bit (you may want to set other bits as well) */
	mov	r1, #CPU_CONTROL_MMU_ENABLE

	/* fetch current control state */
	mrc	15, 0, r2, c1, c0, 0
	orr	r2, r2, r1

	/* set new control state */
	mcr	15, 0, r2, c1, c0, 0

	/* jump to kernel space */
	mov	r0, r0
	mov	r0, r0
	mov	r0, r0

	/* emit a char */
	mov	r6, #0
	add	r6, r6, #0xfd200000
	add	r6, r6, #0x3f8
	mov	r2, #0x2a
	strb	r2, [r6]

	mov	r0, #0x0200

	/* now startup for real */
	ldr	pc, Lstart

Lstart:	.long	start
