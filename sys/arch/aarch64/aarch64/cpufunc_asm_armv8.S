/*	$NetBSD: cpufunc_asm_armv8.S,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $	*/

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

#include "opt_multiprocessor.h"
#include <aarch64/asm.h>

	.text
	.align	2

/*
 * Macro to handle the cache. This takes the start address in x0, length
 * in x1. It will corrupt x0, x1, x2, and x3.
 */
.macro cache_handle_range dcop = 0, ic = 0, icop = 0
.if \ic == 0
	mrs	x3, ctr_el0
	ubfx	x3, x3, #16, #4		/* x3 = D cache shift */
	mov	x2, #4			/* size of word */
	lsl	x3, x2, x3		/* x3 = D cache line size */
.else
	mrs	x3, ctr_el0
	ubfx	x2, x3, #16, #4		/* x2 = D cache shift */
	and	x3, x3, #15		/* x3 = I cache shift */
	cmp	x3, x2
	bcs	1f
	mov	x3, x2
1:					/* x3 = MAX(IcacheShift,DcacheShift) */
	mov	x2, #4			/* size of word */
	lsl	x3, x2, x3		/* x3 = cache line size */
.endif
	sub	x4, x3, #1		/* Get the address mask */
	and	x2, x0, x4		/* Get the low bits of the address */
	add	x1, x1, x2		/* Add these to the size */
	bic	x0, x0, x4		/* Clear the low bit of the address */
1:
	dc	\dcop, x0
	dsb	ish
.if \ic != 0
	ic	\icop, x0
	dsb	ish
.endif
	add	x0, x0, x3		/* Move to the next line */
	subs	x1, x1, x3		/* Reduce the size */
	b.hi	1b			/* Check if we are done */
.if \ic != 0
	isb
.endif
	ret
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
END(aarch64_dcache_wb_range)

/*
 * void aarch64_dcache_wbinv_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_dcache_wbinv_range)
	cache_handle_range	dcop = civac
END(aarch64_dcache_wbinv_range)

/*
 * void aarch64_dcache_inv_range(vaddr_t, vsize_t)
 *
 * Note, we must not invalidate everything.  If the range is too big we
 * must use wb-inv of the entire cache.
 */
ENTRY(aarch64_dcache_inv_range)
	cache_handle_range	dcop = ivac
END(aarch64_dcache_inv_range)

/*
 * void aarch64_idcache_wbinv_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_idcache_wbinv_range)
	cache_handle_range	dcop = civac, ic = 1, icop = ivau
END(aarch64_idcache_wbinv_range)

/*
 * void aarch64_icache_sync_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_icache_sync_range)
	cache_handle_range	dcop = cvau, ic = 1, icop = ivau
END(aarch64_icache_sync_range)

/*
 * void aarch64_icache_inv_all(void)
 */
ENTRY(aarch64_icache_inv_all)
	dsb	ish
#ifdef MULTIPROCESSOR
	ic	iallu
#else
	ic	ialluis
#endif
	dsb	ish
	isb
	ret
END(aarch64_icache_sync_range)



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
#ifdef MULTIPROCESSOR
	/* need dsb and isb for inner shareable? */
	dsb	ishst
	tlbi	vae1is, x8
	dsb	ish
	isb
#else
	/* no need dsb and isb for single entry */
	tlbi	vae1, x8
#endif
	ret
END(aarch64_tlbi_by_asid_va)

/* aarch64_tlbi_by_asid_va_ll(int asid, vaddr_t va) */
ENTRY(aarch64_tlbi_by_asid_va_ll)
	/* x8 = bit 63[ASID]48, 47[RES0]44, 43[VA(55:12)]0 */
	lsl	x8, x0, #48
	bfxil	x8, x1, #12, #44
#ifdef MULTIPROCESSOR
	/* need dsb and isb for inner shareable? */
	dsb	ishst
	tlbi	vale1is, x8
	dsb	ish
	isb
#else
	/* no need dsb and isb for single entry */
	tlbi	vale1, x8
#endif
	ret
END(aarch64_tlbi_by_asid_va_ll)
