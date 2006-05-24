/*	$NetBSD: viper_start.S,v 1.2.8.1 2006/05/24 10:56:46 yamt Exp $	*/

/*
 * Copyright (c) 2005 Antti Kantee.  All Rights Reserved.
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
 *        This product includes software developed by The NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pmap.h>
#include <arm/arm32/pte.h>

/*
 * We start out with RAM mapped to the bottom 64MB.  We are jogging
 * happily there with the MMU on.  Our mission: map some important
 * bootstrap devices (such as console port) in addition to mapping
 * the physical RAM to 0xc0...
 *
 * If I try to create a mapping from scratch, something important gets
 * wiped out (never could figure out exactly what), so we do this with
 * the MMU on adding to the existing translation table.
 */

#define CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0  /* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */

#ifndef SDRAM_START
#define SDRAM_START 0xa0000000
#endif
#define XSCALE_DCACHE_SIZE 0x8000

	.text

	.global _C_LABEL(viper_start)
_C_LABEL(viper_start):

	/* Figure out where we want to jump to when the time comes */
	adr	r8, .Linva
	ldr	r8, [r8]

	/*
	 * Start playing with the virtual address space mapping
	 * for initial bootstrap.
	 *
	 * Load registers, which will remain constant throughout
	 * building the VA mapping.
	 */
	mov     r2, #(L1_S_SIZE)		/* 1MB chunks */

	/*
	 * First map SDRAM VA == PA.  This enables us to cut&waste
	 * some existing initarm() code without modification
	 * (and, if, god forbid, someone would like to unify them
	 * some day, this'll make that job easier)
	 */
	mrc	p15, 0, r0, c2, c0, 0		/* Get L1 */
	bic	r0, r0, #0xff000000
	add	r0, r0, #(0xa00 * 4)		/* offset to 0xa0.. */

	mov	r3, #SDRAM_START		/* map to 0xa00.. */
	orr	r3, r3, #(L1_S_AP(AP_KRW))	/* the usual perms & stuff */
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	mov	r1, #0x40			/* 64 1MB entries */

1:
	/* and looplooploop */
	str	r3, [r0], #4
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Map SDRAM also to VA 0xc00...
	 */
	mrc	p15, 0, r0, c2, c0, 0		/* Get L1 */
	bic	r0, r0, #0xff000000
	add	r0, r0, #(0xc00 * 4)		/* start from 0xc00.. */

	mov	r3, #SDRAM_START		/* map to 0xa00.. */
	orr	r3, r3, #(L1_S_AP(AP_KRW))	/* the usual perms & stuff */
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	mov	r1, #0x40			/* 64 1MB entries */

1:
	/* and looplooploop */
	str	r3, [r0], #4
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Here come the devices.  Map an L1 section for each device
	 * to make this easy.
	 */

	/* INTCTL */
	mrc	p15, 0, r0, c2, c0, 0		/* Get L1 */
	bic	r0, r0, #0xff000000
	add	r0, r0, #(0xfd0 * 4)		/* offset to 0xfd000000 */

	mov	r3,	#0x40000000	
	orr	r3, r3,	#0x00d00000	
	orr	r3, r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	str	r3, [r0], #4

	/* GPIO */
	mov	r3,	#0x40000000	
	orr	r3, r3,	#0x00e00000	
	orr	r3, r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	str	r3, [r0], #4

	/* CLKMAN */
	mov	r3,	#0x41000000	
	orr	r3, r3, #0x00300000	
	orr	r3, r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	str	r3, [r0], #4

	/* FFUART */
	mov	r3,	#0x40000000	
	orr	r3, r3, #0x00100000	
	orr	r3, r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	str	r3, [r0], #4

	/* BTUART */
	mov	r3,	#0x40000000	
	orr	r3, r3, #0x00200000	
	orr	r3, r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(L1_S_DOM(PMAP_DOMAIN_KERNEL))
	str	r3, [r0], #4

#if 0
	/*
	 * Cache cleanup.  Not needed here?  Slight speedup in booting.
	 */
	mov	r3, #(XSCALE_DCACHE_SIZE)
	subs	r3, r3, #32
1:
	mcr	p15, 0, r3, c7, c10, 2
	subs	r3, r3, #32
	bne	1b
	CPWAIT(r3)

	/* Drain write buffer */
	mcr	p15, 0, r6, c7, c10, 4
#endif

	/*
	 * Make domain control go ful fart.
	 * We probably could be slightly more sensible about this,
	 * but it'll be replaced soon anyway, so why bother.
	 */
	mov	r0, #0xffffffff
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Relocate the kernel to where we want it, not where Redboot
	 * let's us load it.  Don't bother jumping after this stage,
	 * we'll do that soon enough anyway, and to the correct virtual
	 * address space region I might add.
	 */
	adr	r0, _C_LABEL(viper_start)	/* start copy from here */
	add	r0, r0, #SDRAM_START		/* offset to SDRAM mapping */

	ldr	r1, .Lcopy_size			/* copy this much (bytes) */
	add	r1, r1, #3			/* prepare for roundup */
	mov	r1, r1, LSR #2			/* make it words */

	mov	r2, #SDRAM_START		/* target address, */
	add	r2, r2, #0x00200000		/* kernel offsets by 2megs */

	/* after this it's just a load-store-loop */
1:
	ldr	r3, [r0], #4
	str	r3, [r2], #4
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Now let's clean the cache again to make sure everything
	 * is in place.
	 *
	 * XXX: should this take into account the XScale cache clean bug?
	 */
	mov	r3, #(XSCALE_DCACHE_SIZE)
	subs	r3, r3, #32
1:
	mcr	p15, 0, r3, c7, c10, 2
	subs	r3, r3, #32
	bne	1b
	CPWAIT(r3)

	/* Drain write buffer */
	mcr	p15, 0, r6, c7, c10, 4

	/* Invalidate TLBs just to be sure */
	mcr     p15, 0, r0, c8, c7, 0

	/*
	 * You are standing at the gate to NetBSD. --More--
	 * Unspeakable cruelty and harm lurk down there. --More--
	 * Are you sure you want to enter?
	 */
	mov	pc, r8				/* So be it */

/* symbol to use for address calculation in the right VA */
.Linva:
	.word	start

/*
 * Calculate size of kernel to copy.  Don't bother to copy bss,
 * although I guess the CPU could use the warmup exercise ...
 */
.Lcopy_size:
	.word _edata - _C_LABEL(viper_start)
