/* $NetBSD: coproc15.S,v 1.3 1996/10/15 21:47:51 mark Exp $ */

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *
 * RiscBSD kernel project
 *
 * coproc15.S 
 *
 * Manipulation of the CPU internal coprocessor #15 registers
 *
 * Created      : 29/11/94
 */

#include <machine/vmparam.h>
#include <machine/cpu.h>

lr	.req	r14
pc	.req	r15

.text
/*
 * Functions to read the internal coprocessor registers
 *
 * Currently the only registers that can be read are
 *  r0 - CPU ID
 *  r5 - Fault status
 *  r6 - Fault address
 *
 * Eventually these should be inlined.
 */

	.global	_cpu_id

_cpu_id:
	mrc	15, 0, r0, c0, c0, 0
	mov	pc, lr

	.global	_cpu_faultstatus

_cpu_faultstatus:
	mrc	15, 0, r0, c5, c0, 0
	mov	pc, lr

	.global	_cpu_faultaddress

_cpu_faultaddress:
	mrc	15, 0, r0, c6, c0, 0
	mov	pc, lr


/*
 * Functions to write the internal coprocessor registers
 *
 *
 * Currently the only registers that can be write are
 *  r1 - CPU Control
 *  r2 - TTB
 *  r3 - Domain Access Control
 *  r5 - Flush TLB
 *  r6 - Purge TLB
 *  r7 - Flush IDC
 *
 * Eventually these should be inlined.
 */
 
	.global	_cpu_control

_cpu_control:
	mcr	15, 0, r0, c1, c0, 0
	mov	pc, lr

	.global	_setttb

_setttb:
/* We need to flush the cache as it uses virtual addresses that are about to change */
#ifndef CPU_SA110
        mcr     15, 0, r0, c7, c0, 0
#else	/* CPU_SA110 */
	mrs	r3, cpsr_all
	orr	r1, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r1

	stmfd	sp!, {r0-r3, lr}
	bl	_cache_clean
	ldmfd	sp!, {r0-r3, lr}
	mcr	15, 0, r0, c7, c7, 0
	mcr	15, 0, r0, c7, c10, 4
#endif	/* CPU_SA110 */

/* Write the TTB */
	mcr	15, 0, r0, c2, c0, 0

#ifndef CPU_SA110
/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c5, c0, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c0, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0
#else	/* CPU_SA110 */
/* If we have updated the TTB we must flush the TLB */
        mcr     15, 0, r0, c8, c7, 0

/* For good measure we will flush the IDC as well */
        mcr     15, 0, r0, c7, c7, 0

/* Make sure that pipeline is emptied */
        mov     r0, r0
        mov     r0, r0
	msr	cpsr_all , r3
#endif	/* CPU_SA110 */

	mov	pc, lr

	.global	_cpu_domains

_cpu_domains:
	mcr	15, 0, r0, c3, c0, 0
	mov	pc, lr

/*
 * tlb_flush		- flush the whole TLB
 * itlb_flush		- flush the intstruction TLB
 * dtlb_flush		- flush the data TLB
 * dtlb_purge		- purge an entry from the data TLB
 *
 * cache_flush		- flush whole cache (invalidate)
 * icache_flush		- flush instruction cache (invalidate)
 * dcache_flush		- flush data cache (invalidate)
 * dcache_purge		- purge data cache entry (invalidate)
 * dcache_clean		- clean data cache
 * cache_clean		- clean all caches
 * sync_cache		- clean cache entry
 * drain_writebuf	- drains the write buffer
 * sync_caches		- clean all caches
 * sync_icache		- ensures the icache is in sync with dcache
 *			  and memory
 */

/*
 * Notes on the cache - 
 *
 * sync_icache
 * sync_caches
 *		These functions garentee that the processor caches and
 *		main memory are in sync.
 *		Actually they garentee that any dirty data in the
 *		processor cache on entry to the functions is written
 *		back to main memory and the write buffer is drained.
 *		These functions also flush (invalidate) the instruction
 *		cache.
 *		Interrupts are enabled during this function so they
 *		only garentee that dirty data on entry to the function is
 *		written back and only then if the dirty locations are not
 *		accessed from interrupt routines called while the
 *		sync is in progress.
 *
 *
 * cache_clean
 *		This function cleans the data caches, drains the write
 *		buffer and then flushes the instruction and data caches.
 *		Interrupts are disabled by this routine so that dirty
 *		data is not added to the cache prior to the data cache
 *		flush.
 *
 *
 */

#ifndef CPU_SA110
	.global	_tlb_flush
	.global	_itlb_flush
	.global	_dtlb_flush
_tlb_flush:
_itlb_flush:
_dtlb_flush:
	mcr	15, 0, r0, c5, c0, 0
	mov	pc, lr

	.global	_dtlb_purge
_dtlb_purge:
	mcr	15, 0, r0, c6, c0, 0
	mov	pc, lr

/*	.global	_cache_flush*/
/*	.global	_icache_flush*/
/*	.global	_dcache_flush*/
/*	.global	_dcache_purge_entry*/
/*	.global	_dcache_clean_entry*/
	.global	_cache_clean

_cache_flush:
_icache_flush:
_dcache_flush:
_dcache_purge_entry:
_dcache_clean_entry:
_cache_clean:
	mcr	15, 0, r0, c7, c0, 0
	mov	pc, lr


	.global	_drain_writebuf
	.global	_sync_caches
	.global	_sync_cache
	.global	_sync_icache
_drain_writebuf:
_sync_cache:
_sync_caches:
_sync_icache:
	mov	pc, lr
#else	/* CPU_SA110 */
	.global	_tlb_flush
_tlb_flush:
#if 0
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

tlb_flush_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	tlb_flush_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c7, 0		/* flush i+d cache */
#endif
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c8, c7, 0		/* flush i+d tlb */
/*	msr	cpsr_all , r3*/
	mov	pc, lr

/*	.global	_itlb_flush*/
_itlb_flush:
	mcr	15, 0, r0, c8, c5, 0		/* flush icache */
	mov	pc, lr

/*	.global	_dtlb_flush*/
_dtlb_flush:
	mcr	15, 0, r0, c8, c6, 0		/* flush dcache */
	mov	pc, lr

/*	.global	_dtlb_purge*/
_dtlb_purge:
	mcr	15, 0, r0, c8, c6, 1		/* purge d tlb entry */
	mov	pc, lr

/*	.global	_cache_flush*/
/*	.global	_icache_flush*/
/*	.global	_dcache_flush*/
/*	.global	_dcache_purge_entry*/
/*	.global	_dcache_clean_entry*/
	.global	_drain_writebuf

_cache_flush:
	mcr	15, 0, r0, c7, c7, 0		/* flush i+d cache */
	mov	pc, lr

_icache_flush:
	mcr	15, 0, r0, c7, c5, 0		/* flush icache */
	mov	pc, lr

_dcache_flush:
	mcr	15, 0, r0, c7, c6, 0		/* flush dcache */
	mov	pc, lr

_dcache_purge_entry:
	mcr	15, 0, r0, c7, c6, 1		/* purge dcache entry */
	mov	pc, lr

_dcache_clean_entry:
	mcr	15, 0, r0, c7, c10, 1		/* clean dcache entry */
	mov	pc, lr

_drain_writebuf:
	mcr	15, 0, r0, c7, c10, 4		/* draw write buffer */
	mov	pc, lr

	.global	_dcache_clean
_dcache_clean:
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

dcache_clean_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	dcache_clean_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c6, 0		/* flush d cache */
	msr	cpsr_all , r3
	mov	pc, lr

	.global	_cache_clean
_cache_clean:
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0

	mov	r0, #0xf0000000
	add	r1, r0, #32768

cache_clean_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	cache_clean_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c7, 0		/* flush i+d cache */
	msr	cpsr_all , r3

	mov	pc, lr	


	.global	_sync_icache
_sync_icache:
	.global	_sync_caches
_sync_caches:
	mrs	r3, cpsr_all
	orr	r0, r3, #(I32_bit | F32_bit)
	msr	cpsr_all , r0
	mov	r0, #0xf0000000
	add	r1, r0, #32768

sync_icache_loop:
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	sync_icache_loop

	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c5, 0		/* flush i cache */
	msr	cpsr_all , r3

	mov	pc, lr	

	.global	_sync_cache
_sync_cache:
	mcr	15, 0, r0, c7, c10, 1		/* clean dcache entry */
	mcr	15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	15, 0, r0, c7, c5, 0		/* flush icache */
	mov	pc, lr	
#endif	/* CPU_SA110 */
