/* $Id: imx23_olinuxino_start.S,v 1.1.6.2 2013/02/25 00:28:37 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <machine/asm.h>
#include <machine/pmap.h>
#include <arm/armreg.h>
#include <arm/imx/imx23var.h>
#include <arm/imx/imx23_uartdbgreg.h>

.section .start,"ax",%progbits

.global	_C_LABEL(olinuxino_start)
_C_LABEL(olinuxino_start):

	/*
	 * Set up the first level page table. The page table has 4096 section
	 * page table entries which each one maps 1MB of virtual memory.
	 * Section entries are mapped from mmu_init_table to the page table.
	 */
	l1pt_p	.req r0
	mit_p	.req r1
	va	.req r2
	pa	.req r3
	n_sec	.req r4
	attr	.req r5
	pte_p	.req r6
	sec	.req r7
	tmp	.req r8
	tmp2	.req r9

	ldr	l1pt_p, Ll1_pt

	/* Zero the page table. */
	mov	tmp, #0
	add	tmp2, l1pt_p, #L1_TABLE_SIZE
1:	str	tmp, [l1pt_p], #4
	cmp	l1pt_p, tmp2
	blt	1b

	ldr	l1pt_p, Ll1_pt

	/* Map sections. */
	ldr	mit_p, =mmu_init_table
	ldmia	mit_p!, {va, pa, n_sec, attr}

	/*
	 * Calculate PTE addresses for a MVA's.
	 *
	 * Bits[31:14] of the Translation Table Base register are concatenated
	 * with bits[31:20] of the modified virtual address and two zero bits
	 * to produce a physical address of the page table entry for a MVA:
	 *
	 * PTE = (TTBR & 0xffffc000) | ((MVA & 0xfff00000)>>18)
	 */
3:	ldr	tmp, =0xffffc000
	and	pte_p, l1pt_p, tmp
	ldr	tmp, =0xfff00000
	and	va, va, tmp
	mov	va, va, LSR #18
	orr	pte_p, pte_p, va

2:	orr	sec, pa, attr
	str	sec, [pte_p], #4	/* Store #n_sec sections to the page */
	add	pa, pa, #0x100000	/* table. */
	subs	n_sec, #1
	bne	2b

	ldmia	mit_p!, {va, pa, n_sec, attr}
	cmp	n_sec, #0
	bne	3b

	/*
	 * The Translation Table Base Register holds the physical address of
	 * the page table.
	 */
	mcr	p15, 0, l1pt_p, c2, c0, 0

	.unreq	l1pt_p
	.unreq	mit_p
	.unreq	va
	.unreq	pa
	.unreq	n_sec
	.unreq	attr
	.unreq	pte_p
	.unreq	sec
	.unreq	tmp
	.unreq	tmp2

	/*
	 * Sections are in domain 0 and we set D0 access control to client
	 * mode, which means AP bits are checked. Since we are running
	 * privileged mode and APs are kernel read/write, access is granted.
	 */
	mov	r0, #DOMAIN_CLIENT<<(PMAP_DOMAIN_KERNEL*2)
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Enable the MMU.
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	/* Invalidate TLB. */

	mrc	p15, 0, r0, c1, c0, 0
	ldr	r1, =(CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE \
			| CPU_CONTROL_AFLT_ENABLE | CPU_CONTROL_MMU_ENABLE)
	orr	r0, r0, r1
	mcr	p15, 0, r0, c1, c0, 0
	nop	/* Fetch flat. */
	nop	/* Fetch flat. */

	/*
	 * Jump to start in locore.S. start sets the sp point to DRAM, zeroes
	 * the .bss and calls initarm. start never returns.
	 */
	ldr	pc, =start

	/* NOTREACHED */ 

/*
 * Initial first level translation table on a 16kB boundary located at the
 * end of the DRAM.
 *
 * The translation table has 4096 32-bit section entries, each describing 1MB of
 * virtual memory which means 4GB of virtual memory to be addressed.
 */
Ll1_pt:
	.word (DRAM_BASE + MEMSIZE * 1024 * 1024 - L1_TABLE_SIZE)

#define MMU_INIT(va,pa,n_sec,attr)					\
	.word va;							\
	.word pa;							\
	.word n_sec;							\
	.word attr;

mmu_init_table:
	/* On-chip RAM */
	MMU_INIT(0x00000000, 0x00000000,
		1,
		L1_S_AP(AP_KRW) | L1_S_DOM(PMAP_DOMAIN_KERNEL) | L1_S_PROTO)

	/* On-chip ROM (Vectors) */
	MMU_INIT(0xFFFF0000, 0xFFFF0000,
		1,
		L1_S_AP(AP_KRW) | L1_S_DOM(PMAP_DOMAIN_KERNEL) | L1_S_PROTO)

	/* DRAM */
	MMU_INIT(KERNEL_BASE_virt, KERNEL_BASE_phys,
		MEMSIZE,
		L1_S_AP(AP_KRW) | L1_S_DOM(PMAP_DOMAIN_KERNEL) | L1_S_C |\
			L1_S_B | L1_S_PROTO)

	/* Peripherals */
	MMU_INIT(APBH_BASE, APBH_BASE,
		1,
		L1_S_AP(AP_KRW) | L1_S_DOM(PMAP_DOMAIN_KERNEL) | L1_S_PROTO)

	MMU_INIT(0, 0, 0, 0)
