/*	$NetBSD: cpufunc_asm_armv8.S,v 1.8 2021/02/11 08:35:11 ryo Exp $	*/

/*-
 * Copyright (c) 2014 Robin Randhawa
 * Copyright (c) 2015 The FreeBSD Foundation
 * All rights reserved.
 *
 * Portions of this software were developed by Andrew Turner
 * under sponsorship from the FreeBSD Foundation
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/arm64/arm64/cpufunc_asm.S 313347 2017-02-06 17:50:09Z andrew $
 */

#include "opt_cputypes.h"
#include "opt_gprof.h"
#include "opt_multiprocessor.h"
#include <aarch64/asm.h>

	.text
	.align	2

/*
 * Macro to handle the cache. This takes the start address in x0, length
 * in x1. It will corrupt x2-x5.
 */
.macro cache_handle_range dcop = "", icop = ""
	mrs	x3, ctr_el0
	mov	x4, #4			/* size of word */
.ifnb \dcop
	ubfx	x2, x3, #16, #4		/* x2 = D cache shift */
	lsl	x2, x4, x2		/* x2 = D cache line size */
.endif
.ifnb \icop
	and	x3, x3, #15		/* x3 = I cache shift */
	lsl	x3, x4, x3		/* x3 = I cache line size */
.endif
.ifnb \dcop
	sub	x4, x2, #1		/* Get the address mask */
	and	x4, x0, x4		/* Get the low bits of the address */
	add	x5, x1, x4		/* Add these to the size */
	bic	x4, x0, x4		/* Clear the low bit of the address */
1:
	dc	\dcop, x4
	add	x4, x4, x2		/* Move to the next line */
	subs	x5, x5, x2		/* Reduce the size */
	b.hi	1b			/* Check if we are done */
	dsb	ish
.endif
.ifnb \icop
	sub	x4, x3, #1		/* Get the address mask */
	and	x4, x0, x4		/* Get the low bits of the address */
	add	x5, x1, x4		/* Add these to the size */
	bic	x4, x0, x4		/* Clear the low bit of the address */
1:
	ic	\icop, x4
	add	x4, x4, x3		/* Move to the next line */
	subs	x5, x5, x3		/* Reduce the size */
	b.hi	1b			/* Check if we are done */
	dsb	ish
	isb
.endif
.endm


ENTRY(aarch64_nullop)
	ret
END(aarch64_nullop)

ENTRY(aarch64_cpuid)
	mrs	x0, midr_el1
	ret
END(aarch64_cpuid)

/*
 * void aarch64_dcache_wb_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_dcache_wb_range)
	cache_handle_range	dcop = cvac
	ret
END(aarch64_dcache_wb_range)

/*
 * void aarch64_dcache_wbinv_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_dcache_wbinv_range)
	cache_handle_range	dcop = civac
	ret
END(aarch64_dcache_wbinv_range)

/*
 * void aarch64_dcache_inv_range(vaddr_t, vsize_t)
 *
 * Note, we must not invalidate everything.  If the range is too big we
 * must use wb-inv of the entire cache.
 */
ENTRY(aarch64_dcache_inv_range)
	cache_handle_range	dcop = ivac
	ret
END(aarch64_dcache_inv_range)

/*
 * void aarch64_idcache_wbinv_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_idcache_wbinv_range)
	cache_handle_range	dcop = civac, icop = ivau
	ret
END(aarch64_idcache_wbinv_range)

/*
 * void aarch64_icache_sync_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_icache_sync_range)
	cache_handle_range	dcop = cvau, icop = ivau
	ret
END(aarch64_icache_sync_range)

/*
 * void aarch64_icache_inv_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_icache_inv_range)
	cache_handle_range	icop = ivau
	ret
END(aarch64_icache_inv_range)

/*
 * void aarch64_icache_barrier_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_icache_barrier_range)
	dsb	ishst
	isb
	ret
END(aarch64_icache_barrier_range)

/*
 * void aarch64_icache_inv_all(void)
 */
ENTRY(aarch64_icache_inv_all)
	dsb	ish
#ifdef MULTIPROCESSOR
	ic	ialluis
#else
	ic	iallu
#endif
	dsb	ish
	isb
	ret
END(aarch64_icache_inv_all)



ENTRY(aarch64_drain_writebuf)
	dsb	sy
	ret
END(aarch64_drain_writebuf)


/*
 * TLB ops
 */

/* void aarch64_set_ttbr0(uint64_t ttbr0) */
ENTRY(aarch64_set_ttbr0)
	dsb	ish
	msr	ttbr0_el1, x0
	dsb	ish
	isb
	ret
END(aarch64_set_ttbr0)

#ifdef CPU_THUNDERX
/*
 * Cavium erratum 27456
 * void aarch64_set_ttbr0_thunderx(uint64_t ttbr0)
 */
ENTRY(aarch64_set_ttbr0_thunderx)
	dsb	ish
	msr	ttbr0_el1, x0
	isb
	ic	iallu
	dsb	nsh
	isb
	ret
END(aarch64_set_ttbr0_thunderx)
#endif /* CPU_THUNDERX */

/* void aarch64_tlbi_all(void) */
ENTRY(aarch64_tlbi_all)
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	vmalle1is
#else
	tlbi	vmalle1
#endif
	dsb	ish
	isb
	ret
END(aarch64_tlbi_all)

/* void aarch64_tlbi_by_asid(int asid) */
ENTRY(aarch64_tlbi_by_asid)
	/* x8 = bit 63[ASID]48, 47[RES0]0 */
	lsl	x8, x0, #48
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	aside1is, x8
#else
	tlbi	aside1, x8
#endif
	dsb	ish
	isb
	ret
END(aarch64_tlbi_by_asid)

/* aarch64_tlbi_by_va(vaddr_t va) */
ENTRY(aarch64_tlbi_by_va)
	/* x8 = bit 63[RES0]44, 43[VA(55:12)]0 */
	ubfx	x8, x0, #12, #44
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	vaae1is, x8
#else
	tlbi	vaae1, x8
#endif
	dsb	ish
	isb
	ret
END(aarch64_tlbi_by_va)

/* aarch64_tlbi_by_va_ll(vaddr_t va) */
ENTRY(aarch64_tlbi_by_va_ll)
	/* x8 = bit 63[RES0]44, 43[VA(55:12)]0 */
	ubfx	x8, x0, #12, #44
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	vaale1is, x8
#else
	tlbi	vaale1, x8
#endif
	dsb	ish
	isb
	ret
END(aarch64_tlbi_by_va_ll)

/* aarch64_tlbi_by_asid_va(int asid, vaddr_t va) */
ENTRY(aarch64_tlbi_by_asid_va)
	/* x8 = bit 63[ASID]48, 47[RES0]44, 43[VA(55:12)]0 */
	lsl	x8, x0, #48
	bfxil	x8, x1, #12, #44
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	vae1is, x8
#else
	tlbi	vae1, x8
#endif
	dsb	ish
	isb
	ret
END(aarch64_tlbi_by_asid_va)

/* aarch64_tlbi_by_asid_va_ll(int asid, vaddr_t va) */
ENTRY(aarch64_tlbi_by_asid_va_ll)
	/* x8 = bit 63[ASID]48, 47[RES0]44, 43[VA(55:12)]0 */
	lsl	x8, x0, #48
	bfxil	x8, x1, #12, #44
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	vale1is, x8
#else
	tlbi	vale1, x8
#endif
	dsb	ish
	isb
	ret
END(aarch64_tlbi_by_asid_va_ll)
