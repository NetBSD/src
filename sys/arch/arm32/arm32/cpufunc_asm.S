/*	$NetBSD: cpufunc_asm.S,v 1.1.4.1 1997/03/11 21:53:08 is Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufunc.S 
 *
 * Assembly functions for CPU / MMU / TLB specific operations
 *
 * Created      : 30/01/97
 */

#include <machine/vmparam.h>
#include <machine/cpu.h>

#define	ENTRY(func)	.global	func;	func:;

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text
	.align	0

ENTRY(_cpufunc_nullop)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mov	pc, lr

/*
 * Generic functions to read the internal coprocessor registers
 *
 * Currently these registers are :
 *  r0 - CPU ID
 *  r5 - Fault status
 *  r6 - Fault address
 *
 */

ENTRY(_cpufunc_id)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrc	15, 0, r0, c0, c0, 0
	mov	pc, lr

ENTRY(_cpufunc_faultstatus)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrc	15, 0, r0, c5, c0, 0
	mov	pc, lr

ENTRY(_cpufunc_faultaddress)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrc	15, 0, r0, c6, c0, 0
	mov	pc, lr

/*
 * Generic functions to write the internal coprocessor registers
 *
 *
 * Currently these registers are 
 *  r1 - CPU Control
 *  r3 - Domain Access Control
 *
 * All other registers are CPU architecture specific
 */
 
ENTRY(_cpufunc_control)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mcr	15, 0, r0, c1, c0, 0
	mov	pc, lr

ENTRY(_cpufunc_domains)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mcr	15, 0, r0, c3, c0, 0
	mov	pc, lr

/*
 * Functions to set the MMU Translation Table Base register
 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
ENTRY(_arm67_setttb)

/* We need to flush the cache as it uses virtual addresses that are about to change */
        mcr     15, 0, r0, c7, c0, 0

/* Write the TTB */
	mcr	15, 0, r0, c2, c0, 0

/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c5, c0, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0

	mov	pc, lr
#endif	/* CPU_ARM6 || CPU_ARM7 */

#ifdef CPU_SA110
ENTRY(_sa110_setttb)
/* We need to flush the cache as it uses virtual addresses that are about to change */
	mrs	r3, cpsr_all
	orr	r1, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r1

	stmfd	sp!, {r0-r3, lr}
	bl	_sa110_cache_cleanID
	ldmfd	sp!, {r0-r3, lr}
	mcr	15, 0, r0, c7, c7, 0
	mcr	15, 0, r0, c7, c10, 4

/* Write the TTB */
	mcr	15, 0, r0, c2, c0, 0

/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c8, c7, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c7, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0
	msr	cpsr_all , r3

	mov	pc, lr
#endif	/* CPU_SA110 */

/*
 * TLB functions
 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
ENTRY(_arm67_tlb_flush)
	mcr	15, 0, r0, c5, c0, 0
	mov	pc, r14

ENTRY(_arm67_tlb_purge)
	mcr	15, 0, r0, c6, c0, 0
	mov	pc, lr
#endif	/* CPU_ARM6 || CPU_ARM7 */

#ifdef CPU_SA110
ENTRY(_sa110_tlb_flushID)
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */ /* XXX */
	mcr	15, 0, r0, c8, c7, 0		/* flush I+D tlb */
	mov	pc, lr

ENTRY(_sa110_tlb_flushI)
	mcr	15, 0, r0, c8, c5, 0		/* flush I tlb */
	mov	pc, lr

ENTRY(_sa110_tlb_flushD)
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */ /* XXX */
	mcr	15, 0, r0, c8, c6, 0		/* flush D tlb */
	mov	pc, lr

ENTRY(_sa110_tlb_flushD_SE)
	mcr	15, 0, r0, c8, c6, 1		/* flush D tlb single entry */
	mov	pc, lr
#endif	/* CPU_SA110 */

/*
 * Cache functions
 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
ENTRY(_arm67_cache_flush)
	mcr	15, 0, r0, c7, c0, 0
	mov	pc, lr
#endif	/* CPU_ARM6 || CPU_ARM7 */

#ifdef CPU_SA110
ENTRY(_sa110_cache_flushID)
	mcr	15, 0, r0, c7, c7, 0		/* flush I+D cache */
	mov	pc, lr

ENTRY(_sa110_cache_flushI)
	mcr	15, 0, r0, c7, c5, 0		/* flush I cache */
	mov	pc, lr

ENTRY(_sa110_cache_flushD)
	mcr	15, 0, r0, c7, c6, 0		/* flush D cache */
	mov	pc, lr

ENTRY(_sa110_cache_flushD_SE)
	mcr	15, 0, r0, c7, c6, 1		/* flush D cache single entry */
	mov	pc, lr

ENTRY(_sa110_cache_cleanD_E)
	mcr	15, 0, r0, c7, c10, 1		/* clean D cache entry */
	mov	pc, lr

ENTRY(_sa110_cache_cleanID)
ENTRY(_sa110_cache_cleanD)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

Lsa110_cache_cleanD_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	Lsa110_cache_cleanD_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	msr	cpsr_all , r3

	mov	pc, lr	

ENTRY(_sa110_cache_purgeID)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

Lsa110_cache_purgeID_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	Lsa110_cache_purgeID_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c7, 0		/* flush I+D cache */
	msr	cpsr_all , r3

	mov	pc, lr	

ENTRY(_sa110_cache_purgeD)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

Lsa110_cache_purgeD_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	Lsa110_cache_purgeD_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c6, 0		/* flush D cache */
	msr	cpsr_all , r3

	mov	pc, lr	

ENTRY(_sa110_cache_purgeID_E)
	mcr	15, 0, r0, c7, c10, 1		/* clean dcache entry */
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */	
	mcr	15, 0, r0, c7, c5, 0		/* flush I cache */
	mcr	15, 0, r0, c7, c6, 1		/* flush D cache single entry */
	mov	pc, lr

ENTRY(_sa110_cache_purgeD_E)
	mcr	15, 0, r0, c7, c10, 1		/* clean dcache entry */
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */	
	mcr	15, 0, r0, c7, c6, 1		/* flush D cache single entry */
	mov	pc, lr
#endif	/* CPU_SA110 */

/*
 * Other functions
 */

#ifdef CPU_SA110
ENTRY(_sa110_drain_writebuf)
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mov	pc, lr
#endif	/* CPU_SA110 */

/*
 * Soft functions
 */

#ifdef CPU_SA110
/*
 * These functions need to be written. Until then act on the whole cache
 */

ENTRY(_sa110_cache_syncI)
#if defined(GPROF) && defined(PROFILE_ASM)
	stmfd	sp!, {lr}
	mov	ip, lr
	bl	mcount
	ldmfd	sp!, {lr}
#endif
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

Lsa110_cache_syncI_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	Lsa110_cache_syncI_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c5, 0		/* flush I cache */
	msr	cpsr_all , r3

	mov	pc, lr	

ENTRY(_sa110_cache_cleanID_rng)
ENTRY(_sa110_cache_cleanD_rng)
	b	_sa110_cache_cleanID
ENTRY(_sa110_cache_purgeID_rng)
	b	_sa110_cache_purgeID
ENTRY(_sa110_cache_purgeD_rng)
	b	_sa110_cache_purgeD
ENTRY(_sa110_cache_syncI_rng)
	b	_sa110_cache_syncI
#endif	/* CPU_SA110 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
ENTRY(_arm67_context_switch)
/* Switch the memory to the new process */

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0		/* flush cache */

/* Write the TTB */
        mcr     15, 0, r0, c2, c0, 0

/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c5, c0, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0
        mov     pc, r14
#endif

#ifdef CPU_SA110
ENTRY(_sa110_context_switch)
/* Switch the memory to the new process */

/* For good measure we will flush the IDC as well */
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c7, 0		/* flush i+d cache */

/* Write the TTB */
        mcr     15, 0, r0, c2, c0, 0

/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c8, c7, 0		/* flush the i+d tlb */

/* For good measure we will flush the IDC as well */	
        mcr     15, 0, r0, c7, c7, 0		/* flush the i+d cache */

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0
        mov     pc, r14
#endif

/*
 * other potentially useful software functions are:
 *  clean D cache entry and flush I cache entry
 *   for the moment use cache_purgeID_E
 */
