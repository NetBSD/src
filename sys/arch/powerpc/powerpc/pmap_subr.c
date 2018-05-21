/*	$NetBSD: pmap_subr.c,v 1.27.46.1 2018/05/21 04:36:01 pgoyette Exp $	*/
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com> of Allegro Networks, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_subr.c,v 1.27.46.1 2018/05/21 04:36:01 pgoyette Exp $");

#include "opt_multiprocessor.h"
#include "opt_altivec.h"
#include "opt_pmap.h"
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#if defined (PPC_OEA) || defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/vmparam.h>
#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif
#endif
#include <powerpc/psl.h>

#define	MFMSR()		mfmsr()
#define	MTMSR(psl)	__asm volatile("sync; mtmsr %0; isync" :: "r"(psl))

#ifdef PMAPCOUNTERS
#define	PMAPCOUNT(ev)	((pmap_evcnt_ ## ev).ev_count++)
#define	PMAPCOUNT2(ev)	((ev).ev_count++)

struct evcnt pmap_evcnt_zeroed_pages =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	"pages zeroed");
struct evcnt pmap_evcnt_copied_pages =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	"pages copied");
struct evcnt pmap_evcnt_idlezeroed_pages =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap",
	"pages idle zeroed");

EVCNT_ATTACH_STATIC(pmap_evcnt_zeroed_pages);
EVCNT_ATTACH_STATIC(pmap_evcnt_copied_pages);
EVCNT_ATTACH_STATIC(pmap_evcnt_idlezeroed_pages);

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)

struct evcnt pmap_evcnt_mappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "pages mapped");
struct evcnt pmap_evcnt_unmappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_mappings,
	    "pmap", "pages unmapped");

struct evcnt pmap_evcnt_kernel_mappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "kernel pages mapped");
struct evcnt pmap_evcnt_kernel_unmappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_kernel_mappings,
	    "pmap", "kernel pages unmapped");

struct evcnt pmap_evcnt_mappings_replaced =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "page mappings replaced");

struct evcnt pmap_evcnt_exec_mappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_mappings,
	    "pmap", "exec pages mapped");
struct evcnt pmap_evcnt_exec_cached =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_mappings,
	    "pmap", "exec pages cached");

struct evcnt pmap_evcnt_exec_synced =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages synced");
struct evcnt pmap_evcnt_exec_synced_clear_modify =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages synced (CM)");
struct evcnt pmap_evcnt_exec_synced_pvo_remove =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages synced (PR)");

struct evcnt pmap_evcnt_exec_uncached_page_protect =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (PP)");
struct evcnt pmap_evcnt_exec_uncached_clear_modify =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (CM)");
struct evcnt pmap_evcnt_exec_uncached_zero_page =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (ZP)");
struct evcnt pmap_evcnt_exec_uncached_copy_page =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (CP)");
struct evcnt pmap_evcnt_exec_uncached_pvo_remove =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (PR)");

struct evcnt pmap_evcnt_updates =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "updates");
struct evcnt pmap_evcnt_collects =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "collects");
struct evcnt pmap_evcnt_copies =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "copies");

struct evcnt pmap_evcnt_ptes_spilled =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes spilled from overflow");
struct evcnt pmap_evcnt_ptes_unspilled =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes not spilled");
struct evcnt pmap_evcnt_ptes_evicted =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes evicted");

struct evcnt pmap_evcnt_ptes_primary[8] = {
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[0]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[1]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[2]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[3]"),

    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[4]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[5]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[6]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[7]"),
};
struct evcnt pmap_evcnt_ptes_secondary[8] = {
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[0]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[1]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[2]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[3]"),

    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[4]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[5]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[6]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[7]"),
};
struct evcnt pmap_evcnt_ptes_removed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes removed");
struct evcnt pmap_evcnt_ptes_changed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes changed");
struct evcnt pmap_evcnt_pvos_reclaimed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "pvos reclaimed");
struct evcnt pmap_evcnt_pvos_failed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "pvo allocation failures");

EVCNT_ATTACH_STATIC(pmap_evcnt_mappings);
EVCNT_ATTACH_STATIC(pmap_evcnt_mappings_replaced);
EVCNT_ATTACH_STATIC(pmap_evcnt_unmappings);

EVCNT_ATTACH_STATIC(pmap_evcnt_kernel_mappings);
EVCNT_ATTACH_STATIC(pmap_evcnt_kernel_unmappings);

EVCNT_ATTACH_STATIC(pmap_evcnt_exec_mappings);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_cached);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_synced);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_synced_clear_modify);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_synced_pvo_remove);

EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_page_protect);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_clear_modify);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_zero_page);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_copy_page);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_pvo_remove);

EVCNT_ATTACH_STATIC(pmap_evcnt_updates);
EVCNT_ATTACH_STATIC(pmap_evcnt_collects);
EVCNT_ATTACH_STATIC(pmap_evcnt_copies);

EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_spilled);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_unspilled);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_evicted);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_removed);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_changed);

EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 0);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 1);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 2);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 3);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 4);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 5);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 6);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 7);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 0);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 1);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 2);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 3);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 4);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 5);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 6);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 7);

EVCNT_ATTACH_STATIC(pmap_evcnt_pvos_reclaimed);
EVCNT_ATTACH_STATIC(pmap_evcnt_pvos_failed);
#endif /* PPC_OEA || PPC_OEA64_BRIDGE */
#else
#define	PMAPCOUNT(ev)	((void) 0)
#define	PMAPCOUNT2(ev)	((void) 0)
#endif /* PMAPCOUNTERS */

/*
 * This file uses a sick & twisted method to deal with the common pmap
 * operations of zero'ing, copying, and syncing the page with the
 * instruction cache.
 *
 * When a PowerPC CPU takes an exception (interrupt or trap), that 
 * exception is handled with the MMU off.  The handler has to explicitly
 * renable the MMU before continuing.  The state of the MMU will be restored
 * when the exception is returned from.
 *
 * Therefore if we disable the MMU we know that doesn't affect any exception.
 * So it's safe for us to disable the MMU so we can deal with physical
 * addresses without having to map any pages via a BAT or into a page table.
 *
 * It's also safe to do regardless of IPL.
 *
 * However while relocation is off, we MUST not access the kernel stack in
 * any manner since it will probably no longer be mapped.  This mean no
 * calls while relocation is off.  The AltiVEC routines need to handle the
 * MSR fiddling themselves so they can save things on the stack.
 */

/*
 * Fill the given physical page with zeroes.
 */
void
pmap_zero_page(paddr_t pa)
{
	size_t linewidth;
	register_t msr = 0; /* XXX: gcc */

#if defined(PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	{
		/*
		 * If we are zeroing this page, we must clear the EXEC-ness
		 * of this page since the page contents will have changed.
		 */
		struct vm_page *pg = PHYS_TO_VM_PAGE(pa);
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		KDASSERT(pg != NULL);
		KDASSERT(LIST_EMPTY(&md->mdpg_pvoh));
#ifdef PMAPCOUNTERS
		if (md->mdpg_attrs & PTE_EXEC) {
			PMAPCOUNT(exec_uncached_zero_page);
		}
#endif
		md->mdpg_attrs &= ~PTE_EXEC;
	}
#endif

	PMAPCOUNT(zeroed_pages);

#ifdef ALTIVEC
	if (pmap_use_altivec) {
		vzeropage(pa);
		return;
	}
#endif

	/*
	 * Turn off data relocation (DMMU off).
	 */
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	if (pa >= SEGMENT_LENGTH) {
#endif
		msr = MFMSR();
		MTMSR(msr & ~PSL_DR);
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	}
#endif

	/*
	 * Zero the page.  Since DR is off, the address is assumed to
	 * valid but we know that UVM will never pass a uncacheable page.
	 * Don't use dcbz if we don't know the cache width.
	 */
	if ((linewidth = curcpu()->ci_ci.dcache_line_size) == 0) {
		long *dp = (long *)pa;
		long * const ep = dp + PAGE_SIZE/sizeof(dp[0]);
		do {
			dp[0] = 0; dp[1] = 0; dp[2] = 0; dp[3] = 0;
			dp[4] = 0; dp[5] = 0; dp[6] = 0; dp[7] = 0;
		} while ((dp += 8) < ep);
	} else {
		size_t i = 0;
		do {
			__asm ("dcbz %0,%1" :: "b"(pa), "r"(i)); i += linewidth;
			__asm ("dcbz %0,%1" :: "b"(pa), "r"(i)); i += linewidth;
		} while (i < PAGE_SIZE);
	}

	/*
	 * Restore data relocation (DMMU on).
	 */
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	if (pa >= SEGMENT_LENGTH)
#endif
		MTMSR(msr);
}

/*
 * Copy the given physical source page to its destination.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	const register_t *sp;
	register_t *dp;
	register_t msr;
	size_t i;

#if defined(PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	{
		/*
		 * If we are copying to the destination page, we must clear
		 * the EXEC-ness of this page since the page contents have
		 * changed.
		 */
		struct vm_page *pg = PHYS_TO_VM_PAGE(dst);
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		KDASSERT(pg != NULL);
		KDASSERT(LIST_EMPTY(&md->mdpg_pvoh));
#ifdef PMAPCOUNTERS
		if (md->mdpg_attrs & PTE_EXEC) {
			PMAPCOUNT(exec_uncached_copy_page);
		}
#endif
		md->mdpg_attrs &= ~PTE_EXEC;
	}
#endif

	PMAPCOUNT(copied_pages);

#ifdef ALTIVEC
	if (pmap_use_altivec) {
		vcopypage(dst, src);
		return;
	}
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	if (src < SEGMENT_LENGTH && dst < SEGMENT_LENGTH) {
		/*
		 * Copy the page (memcpy is optimized, right? :)
		 */
		memcpy((void *) dst, (void *) src, PAGE_SIZE);
		return;
	}
#endif

	/*
	 * Turn off data relocation (DMMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~PSL_DR);

	/*
	 * Copy the page.  Don't use memcpy as we can't refer to the
	 * kernel stack at this point.
	 */
	sp = (const register_t *) src;
	dp = (register_t *) dst;
	for (i = 0; i < PAGE_SIZE/sizeof(dp[0]); i += 8, dp += 8, sp += 8) {
		dp[0] = sp[0]; dp[1] = sp[1]; dp[2] = sp[2]; dp[3] = sp[3];
		dp[4] = sp[4]; dp[5] = sp[5]; dp[6] = sp[6]; dp[7] = sp[7];
	}

	/*
	 * Restore data relocation (DMMU on).
	 */
	MTMSR(msr);
}

void
pmap_syncicache(paddr_t pa, psize_t len)
{

/* 
 * XXX
 * disabling the MULTIPROCESSOR case because:
 * - _syncicache() takes a virtual addresses
 * - this causes crashes on G5
 */
#ifdef MULTIPROCESSOR__
	__syncicache((void *)pa, len);
#else
	const size_t linewidth = curcpu()->ci_ci.icache_line_size;
	register_t msr;
	size_t i;

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	if (pa + len <= SEGMENT_LENGTH) {
		__syncicache((void *)pa, len);
		return;
	}
#endif

	/*
	 * Turn off instruction and data relocation (MMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~(PSL_IR|PSL_DR));

	/*
	 * Make sure to start on a cache boundary.
	 */
	len += pa - (pa & ~linewidth);
	pa &= ~linewidth;

	/*
	 * Write out the data cache
	 */
	i = 0;
	do {
		__asm ("dcbst %0,%1" :: "b"(pa), "r"(i)); i += linewidth;
	} while (i < len);

	/*
	 * Wait for it to finish
	 */
	__asm volatile("sync");

	/*
	 * Now invalidate the instruction cache.
	 */
	i = 0;
	do {
		__asm ("icbi %0,%1" :: "b"(pa), "r"(i)); i += linewidth;
	} while (i < len);

	/*
	 * Restore relocation (MMU on).  (this will do the required
	 * sync and isync).
	 */
	MTMSR(msr);
#endif	/* !MULTIPROCESSOR */
}

bool
pmap_pageidlezero(paddr_t pa)
{
	register_t msr;
	register_t *dp = (register_t *) pa;
	struct cpu_info * const ci = curcpu();
	bool rv = true;
	int i;

#if defined(PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	if (pa < SEGMENT_LENGTH) {
		for (i = 0; i < PAGE_SIZE / sizeof(dp[0]); i++) {
			if (ci->ci_want_resched)
				return false;
			*dp++ = 0;
		}
		PMAPCOUNT(idlezeroed_pages);
		return true;
	}
#endif

	/*
	 * Turn off instruction and data relocation (MMU off).
	 */
	msr = MFMSR();
	MTMSR(msr & ~(PSL_IR|PSL_DR));

	/*
	 * Zero the page until a process becomes runnable.
	 */
	for (i = 0; i < PAGE_SIZE / sizeof(dp[0]); i++) {
		if (ci->ci_want_resched) {
			rv = false;
			break;
		}
		*dp++ = 0;
	}

	/*
	 * Restore relocation (MMU on).
	 */
	MTMSR(msr);
#ifdef PMAPCOUNTERS
	if (rv)
		PMAPCOUNT(idlezeroed_pages);
#endif
	return rv;
}
