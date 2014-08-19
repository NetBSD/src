/* $NetBSD: ixm1200_start.S,v 1.4.14.1 2014/08/20 00:02:55 tls Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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


#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

	.section .start,"ax",%progbits

RCSID("$NetBSD: ixm1200_start.S,v 1.4.14.1 2014/08/20 00:02:55 tls Exp $")

	.global _C_LABEL(ixm1200_start)
_C_LABEL(ixm1200_start):
	/*
	 * Disable IRQ and FIQ
	 */
	mrs     r3, cpsr
	orr     r1, r3, #(I32_bit | F32_bit)
	msr     cpsr_all, r1

	/*
	 * Setup coprocessor 15.
	 *
	 * We assume we've been loaded VA == PA, or that the MMU is
	 * disabled.  We will go ahead and disable the MMU here
	 * so that we don't have to worry about flushing caches, etc.
	 */

	/* CONTROL_CP15 */
	mrc	p15, 0, r0, c1, c0 ,0	/* read ctrl */
	bic	r0, r0, #CPU_CONTROL_MMU_ENABLE
	bic	r0, r0, #CPU_CONTROL_AFLT_ENABLE
	orr	r0, r0, #CPU_CONTROL_DC_ENABLE
	orr	r0, r0, #CPU_CONTROL_WBUF_ENABLE
	bic	r0, r0, #CPU_CONTROL_BEND_ENABLE
	orr	r0, r0, #CPU_CONTROL_SYST_ENABLE
	bic	r0, r0, #CPU_CONTROL_ROM_ENABLE
	orr	r0, r0, #CPU_CONTROL_IC_ENABLE
	bic	r0, r0, #CPU_CONTROL_VECRELOC
	mcr	p15, 0, r0, c1, c0 ,0	/* write ctrl */

	nop
	nop
	nop

	/* TRANSLATION_TABLE_BASE */
	mov	r0, #0
	mcr	p15, 0, r0, c2, c0 ,0	/* write trans table base */

	/* DOMAIN_ACCESS_CONTROL */
	mov	r0, #0x00000001		/* use domain 0 as client */
	mcr	p15, 0, r0, c3, c0 ,0	/* write domain */

	/* CACHE_CONTROL_OPERATIONS */
	mrc	p15, 0, r0, c7, c7 ,0	/* flush D and I cache */
	mrc	p15, 0, r0, c7, c10 ,4	/* drain write buffer */

	/* TLB_OPERATIONS */
	mcr	p15, 0, r0, c8, c7 ,0	/* flush D and I TLB */

	/* READ_BUFFER_OPERATIONS */
	mcr	p15, 0, r0, c9, c0 ,0	/* flush all entries */
	mcr	p15, 0, r0, c9, c0 ,4	/* disable user mode MCR access */

	/* PROCESS_ID_VIRTUAL_ADDR_MAPPING */
	mov	r0, #0
	mcr	p15, 0, r0, c13, c0 ,0	/* process ID 0

	/* BREAKPOINT_DEBUG_SUPPORT */
	mov	r0, #0
	mcr	p15, 0, r0, c15, c0 ,0	/* DBAR = 0 */
	mcr	p15, 0, r0, c15, c1 ,0	/* DBVR = 0 */
	mcr	p15, 0, r0, c15, c2 ,0	/* DBMR = 0 */
	mcr	p15, 0, r0, c15, c3 ,0	/* DBCR = 0 (never watch) */
	mcr	p15, 0, r0, c15, c8 ,0	/* IBCR = 0 (never watch) */

	/*
	 * We want to construct a memory map that maps us
	 * VA == PA (SDRAM at 0xc0000000).  We create these mappings
	 * uncached and unbuffered to be safe.
	 *
	 * We also want to map the various devices we want to
	 * talk to VA == PA during bootstrap.
	 *
	 * We also want to map the v0xf0000000 == p0x90000000
	 * to output eary bootstrup messages to the console.
	 *
	 * We just use section mappings for all of this to make it easy.
	 *
	 * We will put the L1 table to do all this at c01fc000
	 * where is our KERNEL_TEXT_BASE - sizeof(L1 table).
	 */

	/*
	 * Step 1: Map the entire address space VA == PA.
	 */
	ldr	r0, Ltable
	mov	r1, #(L1_TABLE_SIZE / 4)	/* 4096 entry */
	mov	r2, #(L1_S_SIZE)		/* 1MB / section */
	mov	r3, #(L1_S_AP_KRW)		/* kernel read/write */
	orr	r3, r3, #(L1_TYPE_S)		/* L1 entry is section */
1:
	str	r3, [r0], #4
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Step 2: Map VA 0xf0000000->0xf0100000 to PA 0x90000000->0x90100000.
	 */
	ldr	r0, Ltable
	add	r0, r0, #(0xf00 * 4)		/* offset to 0xf0000000 */
	mov	r3, #0x90000000
	add	r3, r3, #0x00100000		/* set 0x90100000 to r3 */
	orr	r3, r3, #(L1_S_AP_KRW)	/* kernel read/write */
	orr	r3, r3, #(L1_TYPE_S)		/* L1 entry is section */
	str	r3, [r0]
	
	/* OK!  Page table is set up.  Give it to the CPU. */
	ldr	r0, Ltable
	mcr	p15, 0, r0, c2, c0 ,0	/* write trans table base */
	mcr	p15, 0, r0, c8, c7 ,0	/* flush D and I TLB */

	/* Get ready to jump to the "real" kernel entry point... */
	ldr	r0, Lstart

	/* OK, let's enable the MMU. */
	mrc	p15, 0, r1, c1, c0 ,0	/* read ctrl */
        orr	r1, r1, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r1, c1, c0 ,0	/* write ctrl */

	nop
	nop
	nop
	
	/* CPWAIT sequence to make sure the MMU is on... */
	mrc	p15, 0, r2, c2, c0, 0	/* arbitrary read of CP15 */
	mov	r2, r2			/* force it to complete */
	mov	pc, r0			/* leap to kernel entry point! */

Lstart:
	.word   start

Ltable:
	.word	0xc0200000 - 0x4000	/* our KERNEL_TEXT_BASE - 16KB */

/* end of ixm1200_start.S */
