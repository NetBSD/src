/*	$NetBSD: nwmmu.S,v 1.6 2002/04/05 16:58:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2000, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>, and by Jason R. Thorpe.
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

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pte.h>

	.section .start,"ax",%progbits

	.global	_C_LABEL(nwstart)
_C_LABEL(nwstart):
	/*
	 * The NetWinder's NeTTrom has disabled the MMU
	 * and loaded us at 0x0000c000.
	 *
	 * We are going to plop a 16K L1 table at 0x00008000 and:
	 *
	 *	1. Map the entire address space VA==PA.
	 *
	 *	2. Double-map the first 64MB of RAM at 0xf0000000.
	 *
	 *	3. Map the 21285's PCI I/O space at the virtual
	 *	   address that we will later map it to in
	 *	   netwinder_start().
	 */

	/*
	 * Step 1: Map the entire address space VA==PA.
	 */
	add	r0, pc, #(Ltable - . - 8)
	ldr	r0, [r0]		/* r0 = &l1table */

	mov	r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	mov	r2, #0x100000		/* advance by 1MB */
	mov	r1, #0x1000		/* 4096MB */
1:
	str	r3, [r0], #0x04
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Step 2: Map VA 0xf0000000->0xf3ffffff to PA 0x00000000->0x03ffffff.
	 */
	add	r0, pc, #(Ltable - . - 8)
	ldr	r0, [r0]		/* r0 = &l1table */

	mov	r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	add	r0, r0, #(0xf00 * 4)	/* offset to 0xf00xxxxx */
	mov	r1, #0x40		/* 64MB */
1:
	str	r3, [r0], #0x04
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Step 3: Map VA 0xfd200000 to PA 0x7c000000
	 */
	add	r0, pc, #(Ltable - . - 8)
	ldr	r0, [r0]		/* r0 = &l1table */

	mov	r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #0x7c000000
	add	r0, r0, #(0xfd0 * 4)	/* offset to 0xfd0xxxxx */
	str	r3, [r0, #0x08]		/* -> 0xfd2xxxxx */

	/* OK!  Page table is set up.  Give it to the CPU. */
	add	r0, pc, #(Ltable - . - 8)
	ldr	r0, [r0]
	mcr	p15, 0, r0, c2, c0, 0

	/* Flush the old TLBs, just in case. */
	mcr	p15, 0, r0, c8, c7, 0

	/* Set the Domain Access register.  Very important! */
	mov	r0, #1
	mcr	p15, 0, r0, c3, c0, 0

	/* OK, let's enable the MMU. */
	mrc	p15, 0, r2, c1, c0, 0
	orr	r2, r2, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	/* ...and now we jump to the "real" kernel entry point! */
	add	r0, pc, #(Lstart - . - 8)
	ldr	pc, [r0]

Ltable:
	.word	0x00008000

Lstart:
	.word	start
