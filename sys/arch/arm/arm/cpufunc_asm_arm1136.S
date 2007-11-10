/* $NetBSD: cpufunc_asm_arm1136.S,v 1.1.8.2 2007/11/10 02:56:24 matt Exp $ */

/*
 * Copyright (c) 2007 Danger Inc.
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
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <machine/cpu.h>
#include <machine/asm.h>

RCSID("$NetBSD: cpufunc_asm_arm1136.S,v 1.1.8.2 2007/11/10 02:56:24 matt Exp $")

/*
 * Workaround Erratum 411920 
 * 
 *	- value of arg 'reg' Should Be Zero
 */
#define Invalidate_I_cache(Rtmp1, Rtmp2) \
	mov	Rtmp1, #0		/* SBZ */			;\
	mrs	Rtmp2, cpsr						;\
	cpsid	ifa							;\
	mcr	p15, 0, Rtmp1, c7, c5, 0 /* Invalidate Entire I cache */;\
	mcr	p15, 0, Rtmp1, c7, c5, 0 /* Invalidate Entire I cache */;\
	mcr	p15, 0, Rtmp1, c7, c5, 0 /* Invalidate Entire I cache */;\
	mcr	p15, 0, Rtmp1, c7, c5, 0 /* Invalidate Entire I cache */;\
	msr	cpsr_cx, Rtmp2						;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop								;\
	nop

#define Flush_D_cache(reg) \
	mov	reg, #0;		/* SBZ */					\
	mcr	p15, 0, reg, c7, c14, 0;/* Clean and Invalidate Entire Data Cache */	\
	mcr	p15, 0, reg, c7, c10, 4;/* Data Synchronization Barrier */

ENTRY(arm1136_setttb)
#ifdef PMAP_CACHE_VIVT
	Flush_D_cache(r1)
#endif
#ifdef PMAP_CACHE_VIVT
	Invalidate_I_cache(r1, r2)
#endif
	mcr	p15, 0, r0, c2, c0, 0	/* load new TTB */
	mcr	p15, 0, r1, c8, c7, 0	/* invalidate I+D TLBs */
	mcr	p15, 0, r1, c7, c10, 4	/* drain write buffer */
	RET

ENTRY_NP(arm1136_idcache_wbinv_all)
	Flush_D_cache(r0)
	Invalidate_I_cache(r0, r1)
	RET

ENTRY_NP(arm1136_dcache_wbinv_all)
	Flush_D_cache(r0)
	RET

ENTRY_NP(arm1136_icache_sync_all)
	mov	r0, #0			/* SBZ */				
	mcr	p15, 0, r0, c7, c10, 0	/* Clean Entire Data Cache */
	mcr	p15, 0, r0, c7, c10, 4	/* Data Synchronization Barrier */
	Invalidate_I_cache(r0, r1)
	RET

ENTRY_NP(arm1136_flush_prefetchbuf)
	mcr	p15, 0, r0, c7, c5, 4	/* Flush Prefetch Buffer */
	RET

ENTRY_NP(arm1136_icache_sync_range)
	add	r1, r1, r0
	sub	r1, r1, #1
	/* Erratum 371025, workaround #2 */
	mrs	r2, cpsr		/* save the CPSR */
	cpsid	ifa			/* disable interrupts (irq,fiq,abort) */
	mov	r3, #0 
	mcr	p15, 0, r3, c13, c0, 0	/* write FCSE (uTLB invalidate) */
	mcr	p15, 0, r3, c7, c5, 4	/* flush prefetch buffer */
	add	r3, pc, #0x24 
	mcr	p15, 0, r3, c7, c13, 1	/* prefetch I-cache line */
	mcrr	p15, 0, r1, r0, c5	/* invalidate I-cache range */
	msr	cpsr_cx, r2		/* local_irq_restore */
	nop 
	nop 
	nop 
	nop 
	nop 
	nop 
	nop 

	mcrr	p15, 0, r1, r0, c12	/* clean and invalidate D cache range */
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	RET

ENTRY_NP(arm1136_idcache_wbinv_range)
	add	r1, r1, r0
	sub	r1, r1, #1
	/* Erratum 371025, workaround #2 */
	mrs	r2, cpsr		/* save the CPSR */
	cpsid	ifa			/* disable interrupts (irq,fiq,abort) */
	mov	r3, #0 
	mcr	p15, 0, r3, c13, c0, 0	/* write FCSE (uTLB invalidate) */
	mcr	p15, 0, r3, c7, c5, 4	/* flush prefetch buffer */
	add	r3, pc, #0x24 
	mcr	p15, 0, r3, c7, c13, 1	/* prefetch I-cache line */
	mcrr	p15, 0, r1, r0, c5	/* invalidate I-cache range */
	msr	cpsr_cx, r2		/* local_irq_restore */
	nop 
	nop 
	nop 
	nop 
	nop 
	nop 
	nop 

	mcrr	p15, 0, r1, r0, c14	/* clean and invalidate D cache range */
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	RET
