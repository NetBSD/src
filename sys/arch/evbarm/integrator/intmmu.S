/*	$NetBSD: intmmu.S,v 1.11.14.1 2014/08/20 00:02:54 tls Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
#include <arm/armreg.h>
#include "assym.h"

	.section .start,"ax",%progbits

ASENTRY_NP(integrator_start)
	mov	r6, #0x16000000		/* UART0 Physical base*/
#ifdef VERBOSE_INIT_ARM
	mov	r3, #'A'
	str	r3, [r6]		/* Let the world know we are alive */
#endif
/* 
 * Check that the processor has a CP15.  Some core modules do not.
 * We can tell by reading CM_PROC.  If it is zero, then we're OK, otherwise
 * let the user know why we've died.
 */
	mov	r7, #0x10000000
	ldr	r3, [r7, #4]
	cmp	r3, #0
	bne	Lno_cp15
/*
 * Now read CP15 and check what sort of core we have.  We need to know
 * if it has an MMU.  There's no simple test for this, but the following
 * hack should be sufficient for all currently supported CM boards:
 *  - Check that the product code has a '2' or '3' in bits 8-11
 */
	mrc	p15, 0, r3, c0, c0, 0
	and	r0, r3, #0x00000f00
	teq	r0, #0x00000200		/* ARM 920, 1020, 1026, etc */
	teqne	r0, #0x00000300		/* ARM 1136 */
	bne	Lno_mmu

/*
 * At this time the MMU is off.
 * We build up an initial memory map at 0x8000 that we can use to get
 * the kernel running from the top of memory.  All mappings in this table
 * use L1 section maps.
 */

/*
 * Set Virtual == Physical
 */
	mov	r3, #(L1_S_AP_KRW)
	add	r3, r3, #(L1_TYPE_S)
	mov	r2, #0x100000		/* advance by 1MB */
	mov	r1, #0x8000		/* page table start */
	mov	r0, #0x1000		/* page table size */

Lflat:
	str	r3, [r1], #0x0004
	add	r3, r3, r2
	subs	r0, r0, #1
	bgt	Lflat

/*
 * Map VA 0xc0000000->0xc03fffff to PA 0x00000000->0x003fffff
 */
	mov	r3, #(L1_S_AP_KRW)
	add	r3, r3, #(L1_TYPE_S)
	mov	r1, #0x8000		/* page table start */
	add	r1, r1, #(0xc00 * 4)	/* offset to 0xc00xxxxx */
#	add	r1, r1, #(0x001 * 4)	/* offset to 0xc01xxxxx */
	mov	r0, #63
Lkern:
	str	r3, [r1], #0x0004	/* 0xc000000-0xc03fffff */
	add	r3, r3, r2
	subs	r0, r0, #1
	bgt	Lkern
/*
 * Mapping the peripheral register region (0x10000000->0x1fffffff) linearly
 * would require 256MB of virtual memory (as much space as the entire kernel
 * virtual space).  So we map the first 1M of each 16MB sub-space into the
 * region VA 0xfd000000->0xfdffffff; this should map enough of the peripheral
 * space to at least get us up and running.
 */
	mov	r3, #(L1_S_AP_KRW)
	add	r3, r3, #L1_TYPE_S
	add	r3, r3, #0x10000000	/* Peripherals base */
	mov	r1, #0x8000		/* page table start */
	add	r1, r1, #(0xfd0 * 4)	
	mov	r2, #0x01000000		/* 16MB increment.  */
	mov	r0, #16
Lperiph:
	str	r3, [r1], #4		/* 0xfd000000-0xfdffffff */
	add	r3, r3, r2
	subs	r0, r0, #1
	bgt	Lperiph

/*
 * We now have our page table ready, so load it up and light the blue
 * touch paper.
 */

	/* set the location of the L1 page table */
	mov	r1, #0x8000
	mcr	p15, 0, r1, c2, c0, 0

	/* Flush the old TLBs (just in case) */
	mcr	p15, 0, r1, c8, c7, 0
	/* And the caches */
	mov	r0, #0
	mcr	p15, 0, r1, c7, c6, 0

#ifdef VERBOSE_INIT_ARM
	mov	r2, #'B'
	strb	r2, [r6]
#endif

	/* Set the Domain Access register.  Very important! */
	mov	r1, #1
	mcr	p15, 0, r1, c3, c0, 0

	/*
	 * set mmu bit (don't set anything else for now, we don't know
	 * what sort of CPU we have yet.
	 */
	mov	r1, #CPU_CONTROL_MMU_ENABLE

/*
 * This is where it might all start to go wrong if the CPU fitted to your
 * integrator does not have an MMU.
 */
	/* fetch current control state */
	mrc	p15, 0, r2, c1, c0, 0
	orr	r2, r2, r1

	/* set new control state */
	mcr	p15, 0, r2, c1, c0, 0

	mov	r0, r0
	mov	r0, r0
	mov	r0, r0

#ifdef VERBOSE_INIT_ARM
	/* emit a char.  Uart is now at 0xfd600000 */
	mov	r6, #0xfd000000
	add	r6, r6, #0x00600000
	mov	r2, #'C'
	strb	r2, [r6]
#endif

	/* jump to kernel space */
	mov	r0, #0x0200

	/* Switch to kernel VM and really set the ball rolling.  */
	ldr	pc, Lstart

Lstart:	.long	start

Lmsg:
	ldrb	r2, [r0], #1
	cmp	r2, #0
	strbne	r2, [r6]
Lwait:
	ldrb	r3, [r6, #0x18]
	tst	r3, #0x80
	beq	Lwait
	cmp	r2, #0
	bne	Lmsg
	/* We're toast! */
	b	.
	
Lno_cp15:
	adr	r0, Lcp15msg
	b	Lmsg
Lno_mmu:
	adr	r0, Lmmumsg
	b	Lmsg

Lcp15msg:
	.ascii "Core has no cp15\r\n\0"
Lmmumsg:
	.ascii "Core has no MMU\r\n\0"
