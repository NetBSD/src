/*	$NetBSD: pmap_machdep.c,v 1.21.6.1 2018/06/25 07:25:44 pgoyette Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pmap.c	8.4 (Berkeley) 1/26/94
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pmap_machdep.c,v 1.21.6.1 2018/06/25 07:25:44 pgoyette Exp $");

/*
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

/* XXX simonb 2002/02/26
 *
 * MIPS3_PLUS is used to conditionally compile the r4k MMU support.
 * This is bogus - for example, some IDT MIPS-II CPUs have r4k style
 * MMUs (and 32-bit ones at that).
 *
 * On the other hand, it's not likely that we'll ever support the R6000
 * (is it?), so maybe that can be an "if MIPS2 or greater" check.
 *
 * Also along these lines are using totally separate functions for
 * r3k-style and r4k-style MMUs and removing all the MIPS_HAS_R4K_MMU
 * checks in the current functions.
 *
 * These warnings probably applies to other files under sys/arch/mips.
 */

#include "opt_cputype.h"
#include "opt_mips_cache.h"
#include "opt_multiprocessor.h"
#include "opt_sysv.h"

#define __MUTEX_PRIVATE
#define __PMAP_PRIVATE

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/systm.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <uvm/uvm.h>
#include <uvm/uvm_physseg.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

CTASSERT(MIPS_KSEG0_START < 0);
CTASSERT((intptr_t)MIPS_PHYS_TO_KSEG0(0x1000) < 0);
CTASSERT(MIPS_KSEG1_START < 0);
CTASSERT((intptr_t)MIPS_PHYS_TO_KSEG1(0x1000) < 0);
CTASSERT(MIPS_KSEG2_START < 0);
CTASSERT(MIPS_MAX_MEM_ADDR < 0);
CTASSERT(MIPS_RESERVED_ADDR < 0);
CTASSERT((uint32_t)MIPS_KSEG0_START == 0x80000000);
CTASSERT((uint32_t)MIPS_KSEG1_START == 0xa0000000);
CTASSERT((uint32_t)MIPS_KSEG2_START == 0xc0000000);
CTASSERT((uint32_t)MIPS_MAX_MEM_ADDR == 0xbe000000);
CTASSERT((uint32_t)MIPS_RESERVED_ADDR == 0xbfc80000);
CTASSERT(MIPS_KSEG0_P(MIPS_PHYS_TO_KSEG0(0)));
CTASSERT(MIPS_KSEG1_P(MIPS_PHYS_TO_KSEG1(0)));
#ifdef _LP64
CTASSERT(VM_MIN_KERNEL_ADDRESS % NBXSEG == 0);
#else
CTASSERT(VM_MIN_KERNEL_ADDRESS % NBSEG == 0);
#endif

//PMAP_COUNTER(idlezeroed_pages, "pages idle zeroed");
PMAP_COUNTER(zeroed_pages, "pages zeroed");
PMAP_COUNTER(copied_pages, "pages copied");
extern struct evcnt pmap_evcnt_page_cache_evictions;

u_int pmap_page_cache_alias_mask;

#define pmap_md_cache_indexof(x)	(((vaddr_t)(x)) & pmap_page_cache_alias_mask)

static register_t
pmap_md_map_ephemeral_page(struct vm_page *pg, bool locked_p, int prot,
    pt_entry_t *old_pte_p)
{
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv = &mdpg->mdpg_first;
	register_t va = 0;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pg=%p, prot=%d, ptep=%p)",
	    pg, prot, old_pte_p, 0);

	KASSERT(!locked_p || VM_PAGEMD_PVLIST_LOCKED_P(mdpg));

	if (!MIPS_CACHE_VIRTUAL_ALIAS || !mips_cache_badalias(pv->pv_va, pa)) {
#ifdef _LP64
		va = MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
		if (pa < MIPS_PHYS_MASK) {
			va = MIPS_PHYS_TO_KSEG0(pa);
		}
#endif
	}
	if (va == 0) {
		/*
		 * Make sure to use a congruent mapping to the last mapped
		 * address so we don't have to worry about virtual aliases.
		 */
		kpreempt_disable(); // paired with the one in unmap
		struct cpu_info * const ci = curcpu();
		if (MIPS_CACHE_VIRTUAL_ALIAS) {
			KASSERT(ci->ci_pmap_dstbase != 0);
			KASSERT(ci->ci_pmap_srcbase != 0);

			const u_int __diagused mask = pmap_page_cache_alias_mask;
			KASSERTMSG((ci->ci_pmap_dstbase & mask) == 0,
			    "%#"PRIxVADDR, ci->ci_pmap_dstbase);
			KASSERTMSG((ci->ci_pmap_srcbase & mask) == 0,
			    "%#"PRIxVADDR, ci->ci_pmap_srcbase);
		}
		vaddr_t nva = (prot & VM_PROT_WRITE
			? ci->ci_pmap_dstbase
			: ci->ci_pmap_srcbase)
		    + pmap_md_cache_indexof(MIPS_CACHE_VIRTUAL_ALIAS
			? pv->pv_va
			: pa);

		va = (intptr_t)nva;
		/*
		 * Now to make and write the new PTE to map the PA.
		 */
		const pt_entry_t npte = pte_make_kenter_pa(pa, mdpg, prot, 0);
		pt_entry_t * const ptep = pmap_pte_lookup(pmap_kernel(), va);
		*old_pte_p = *ptep;		// save
		bool rv __diagused;
		*ptep = npte;			// update page table

		// update the TLB directly making sure we force the new entry
		// into it.
		rv = tlb_update_addr(va, KERNEL_PID, npte, true);
		KASSERTMSG(rv == 1, "va %#"PRIxREGISTER" pte=%#"PRIxPTE" rv=%d",
		    va, pte_value(npte), rv);
	}
	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		/*
		 * If we are forced to use an incompatible alias, flush the
		 * page from the cache so we will copy the correct contents.
		 */
		if (!locked_p)
			(void)VM_PAGEMD_PVLIST_READLOCK(mdpg);
		if (VM_PAGEMD_CACHED_P(mdpg)
		    && mips_cache_badalias(pv->pv_va, va)) {
			register_t ova = (intptr_t)trunc_page(pv->pv_va);
			mips_dcache_wbinv_range_index(ova, PAGE_SIZE);
			/*
			 * If there is no active mapping, remember this new one.
			 */
			if (pv->pv_pmap == NULL)
				pv->pv_va = va;
		}
		if (!locked_p)
			VM_PAGEMD_PVLIST_UNLOCK(mdpg);
	}

	UVMHIST_LOG(pmaphist, " <-- done (va=%#lx)", va, 0, 0, 0);

	return va;
}

static void
pmap_md_unmap_ephemeral_page(struct vm_page *pg, bool locked_p, register_t va,
	pt_entry_t old_pte)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv = &mdpg->mdpg_first;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pg=%p, va=%#lx, pte=%#"PRIxPTE")",
	    pg, va, pte_value(old_pte), 0);

	KASSERT(!locked_p || VM_PAGEMD_PVLIST_LOCKED_P(mdpg));

	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		if (!locked_p)
			(void)VM_PAGEMD_PVLIST_READLOCK(mdpg);
		/*
		 * If this page was previously uncached or we had to use an
		 * incompatible alias, flush it from the cache.
		 */
		if (VM_PAGEMD_UNCACHED_P(mdpg)
		    || (pv->pv_pmap != NULL
			&& mips_cache_badalias(pv->pv_va, va))) {
			mips_dcache_wbinv_range(va, PAGE_SIZE);
		}
		if (!locked_p)
			VM_PAGEMD_PVLIST_UNLOCK(mdpg);
	}
	/*
	 * If we had to map using a page table entry, restore it now.
	 */
	if (!pmap_md_direct_mapped_vaddr_p(va)) {
		*pmap_pte_lookup(pmap_kernel(), va) = old_pte;
		if (pte_valid_p(old_pte)) {
			// Update the TLB with the old mapping.
			tlb_update_addr(va, KERNEL_PID, old_pte, 0);
		} else {
			// Invalidate TLB entry if the old pte wasn't valid.
			tlb_invalidate_addr(va, KERNEL_PID);
		}
		kpreempt_enable();	// Restore preemption
	}
	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

static void
pmap_md_vca_page_wbinv(struct vm_page *pg, bool locked_p)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	pt_entry_t pte;

	const register_t va = pmap_md_map_ephemeral_page(pg, locked_p,
	    VM_PROT_READ, &pte);

	mips_dcache_wbinv_range(va, PAGE_SIZE);

	pmap_md_unmap_ephemeral_page(pg, locked_p, va, pte);
}

bool
pmap_md_ok_to_steal_p(const uvm_physseg_t bank, size_t npgs)
{
#ifndef _LP64
	if (uvm_physseg_get_avail_start(bank) + npgs >= atop(MIPS_PHYS_MASK + 1)) {
		aprint_debug("%s: seg not enough in KSEG0 for %zu pages\n",
		    __func__, npgs);
		return false;
	}
#endif
	return true;
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void
pmap_bootstrap(void)
{
	vsize_t bufsz;
	size_t sysmap_size;
	pt_entry_t *sysmap;

	if (MIPS_CACHE_VIRTUAL_ALIAS && uvmexp.ncolors) {
		pmap_page_colormask = (uvmexp.ncolors - 1) << PAGE_SHIFT;
		pmap_page_cache_alias_mask = max(
		    mips_cache_info.mci_cache_alias_mask,
		    mips_cache_info.mci_icache_alias_mask);
	}

#ifdef MULTIPROCESSOR
	pmap_t pm = pmap_kernel();
	kcpuset_create(&pm->pm_onproc, true);
	kcpuset_create(&pm->pm_active, true);
	KASSERT(pm->pm_onproc != NULL);
	KASSERT(pm->pm_active != NULL);
	kcpuset_set(pm->pm_onproc, cpu_number());
	kcpuset_set(pm->pm_active, cpu_number());
#endif
	pmap_tlb_info_init(&pmap_tlb0_info);		/* init the lock */

	/*
	 * Compute the number of pages kmem_arena will have.
	 */
	kmeminit_nkmempages();

	/*
	 * Figure out how many PTE's are necessary to map the kernel.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */

	/* Get size of buffer cache and set an upper limit */
	buf_setvalimit((VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / 8);
	bufsz = buf_memcalc();
	buf_setvalimit(bufsz);

	sysmap_size = (VM_PHYS_SIZE + (ubc_nwins << ubc_winshift) +
	    bufsz + 16 * NCARGS + pager_map_size) / NBPG +
	    (maxproc * UPAGES) + nkmempages;
#ifdef DEBUG
	{
		extern int kmem_guard_depth;
		sysmap_size += kmem_guard_depth;
	}
#endif

#ifdef SYSVSHM
	sysmap_size += shminfo.shmall;
#endif
#ifdef KSEG2IOBUFSIZE
	sysmap_size += (KSEG2IOBUFSIZE >> PGSHIFT);
#endif
#ifdef _LP64
	/*
	 * If we are using tmpfs, then we might want to use a great deal of
	 * our memory with it.  Make sure we have enough VM to do that.
	 */
	sysmap_size += physmem;
#else
	/* XXX: else runs out of space on 256MB sbmips!! */
	sysmap_size += 20000;
#endif
	/* Rounup to a even number of pte page tables */
	sysmap_size = (sysmap_size + NPTEPG - 1) & -NPTEPG;

	/*
	 * Initialize `FYI' variables.	Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.  Must do this before uvm_pageboot_alloc()
	 * can be called.
	 */
	pmap_limits.avail_start = ptoa(uvm_physseg_get_start(uvm_physseg_get_first()));
	pmap_limits.avail_end = ptoa(uvm_physseg_get_end(uvm_physseg_get_last()));
	pmap_limits.virtual_end = pmap_limits.virtual_start + (vaddr_t)sysmap_size * NBPG;

#ifndef _LP64
	if (pmap_limits.virtual_end > VM_MAX_KERNEL_ADDRESS
	    || pmap_limits.virtual_end < VM_MIN_KERNEL_ADDRESS) {
		printf("%s: changing last kernel VA from %#"PRIxVADDR
		    " to %#"PRIxVADDR"\n", __func__,
		    pmap_limits.virtual_end, VM_MAX_KERNEL_ADDRESS);
		pmap_limits.virtual_end = VM_MAX_KERNEL_ADDRESS;
		sysmap_size =
		    (VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / NBPG;
	}
#endif
	pmap_pvlist_lock_init(mips_cache_info.mci_pdcache_line_size);

	/*
	 * Now actually allocate the kernel PTE array (must be done
	 * after pmap_limits.virtual_end is initialized).
	 */
	sysmap = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * sysmap_size);

	vaddr_t va = VM_MIN_KERNEL_ADDRESS;
#ifdef _LP64
	/*
	 * Do we need more than one XSEG's worth virtual address space?
	 * If so, we have to allocate the additional pmap_segtab_t's for them
	 * and insert them into the kernel's top level segtab.
	 */
	const size_t xsegs = (sysmap_size * NBPG + NBXSEG - 1) / NBXSEG;
	if (xsegs > 1) {
		printf("%s: %zu xsegs required for %zu pages\n",
		    __func__, xsegs, sysmap_size);
		pmap_segtab_t *stp = (pmap_segtab_t *)
		    uvm_pageboot_alloc(sizeof(pmap_segtab_t) * (xsegs - 1));
		for (size_t i = 1; i <= xsegs; i++, stp++) {
			pmap_kern_segtab.seg_seg[i] = stp;
		}
	}
	pmap_segtab_t ** const xstp = pmap_kern_segtab.seg_seg;
#else
	const size_t xsegs = 1;
	pmap_segtab_t * const stp = &pmap_kern_segtab;
#endif
	KASSERT(curcpu()->ci_pmap_kern_segtab == &pmap_kern_segtab);

	for (size_t k = 0, i = 0; k < xsegs; k++) {
#ifdef _LP64
		pmap_segtab_t * const stp =
		    xstp[(va >> XSEGSHIFT) & (NSEGPG - 1)];
#endif
		bool done = false;

		for (size_t j = (va >> SEGSHIFT) & (NSEGPG - 1);
		     !done && i < sysmap_size;
		     i += NPTEPG, j++, va += NBSEG) {
			/*
			 * Now set the page table pointer...
			 */
			stp->seg_tab[j] = &sysmap[i];
#ifdef _LP64
			/*
			 * If we are at end of this XSEG, terminate the loop
			 * so we advance to the next one.
			 */
			done = (j + 1 == NSEGPG);
#endif
		}
	}
	KASSERT(pmap_pte_lookup(pmap_kernel(), VM_MIN_KERNEL_ADDRESS) == sysmap);

	/*
	 * Initialize the pools.
	 */
	pool_init(&pmap_pmap_pool, PMAP_SIZE, 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator, IPL_NONE);

	tlb_set_asid(0);

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
	/*
	 * The R4?00 stores only one copy of the Global bit in the
	 * translation lookaside buffer for each 2 page entry.
	 * Thus invalid entries must have the Global bit set so
	 * when Entry LO and Entry HI G bits are anded together
	 * they will produce a global bit to store in the tlb.
	 */
	if (MIPS_HAS_R4K_MMU) {
		while (sysmap_size-- > 0) {
			*sysmap++ = MIPS3_PG_G;
		}
	}
#endif	/* MIPS3_PLUS */
}

void
pmap_md_alloc_ephemeral_address_space(struct cpu_info *ci)
{
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * If we have more memory than can be mapped by KSEG0, we need to
	 * allocate enough VA so we can map pages with the right color
	 * (to avoid cache alias problems).
	 */
	if (false
#ifndef _LP64
	    || pmap_limits.avail_end > MIPS_KSEG1_START - MIPS_KSEG0_START
#endif
	    || MIPS_CACHE_VIRTUAL_ALIAS
	    || MIPS_ICACHE_VIRTUAL_ALIAS) {
		vsize_t size = max(mci->mci_pdcache_way_size, mci->mci_picache_way_size);
		const u_int __diagused mask = pmap_page_cache_alias_mask;

		ci->ci_pmap_dstbase = uvm_km_alloc(kernel_map, size, size,
		    UVM_KMF_VAONLY);

		KASSERT(ci->ci_pmap_dstbase);
		KASSERT(!pmap_md_direct_mapped_vaddr_p(ci->ci_pmap_dstbase));
		KASSERTMSG((ci->ci_pmap_dstbase & mask) == 0, "%#"PRIxVADDR,
		    ci->ci_pmap_dstbase);

		ci->ci_pmap_srcbase = uvm_km_alloc(kernel_map, size, size,
		    UVM_KMF_VAONLY);
		KASSERT(ci->ci_pmap_srcbase);
		KASSERT(!pmap_md_direct_mapped_vaddr_p(ci->ci_pmap_srcbase));
		KASSERTMSG((ci->ci_pmap_srcbase & mask) == 0, "%#"PRIxVADDR,
		    ci->ci_pmap_srcbase);
	}
}

void
pmap_md_init(void)
{
	pmap_md_alloc_ephemeral_address_space(curcpu());

#if defined(MIPS3) && 0
	if (MIPS_HAS_R4K_MMU) {
		/*
		 * XXX
		 * Disable sosend_loan() in src/sys/kern/uipc_socket.c
		 * on MIPS3 CPUs to avoid possible virtual cache aliases
		 * and uncached mappings in pmap_enter_pv().
		 *
		 * Ideally, read only shared mapping won't cause aliases
		 * so pmap_enter_pv() should handle any shared read only
		 * mappings without uncached ops like ARM pmap.
		 *
		 * On the other hand, R4000 and R4400 have the virtual
		 * coherency exceptions which will happen even on read only
		 * mappings, so we always have to disable sosend_loan()
		 * on such CPUs.
		 */
		sock_loan_thresh = -1;
	}
#endif
}

/*
 * XXXJRT -- need a version for each cache type.
 */
void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	if (MIPS_HAS_R4K_MMU) {
		/*
		 * XXX
		 * shouldn't need to do this for physical d$?
		 * should need to do this for virtual i$ if prot == EXEC?
		 */
		if (p == curlwp->l_proc
		    && mips_cache_info.mci_pdcache_way_mask < PAGE_SIZE)
		    /* XXX check icache mask too? */
			mips_icache_sync_range((intptr_t)va, len);
		else
			mips_icache_sync_range_index((intptr_t)va, len);
	} else {
		pmap_t pmap = p->p_vmspace->vm_map.pmap;
		kpreempt_disable();
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		pt_entry_t entry = (ptep != NULL ? *ptep : 0);
		kpreempt_enable();
		if (!pte_valid_p(entry))
			return;

		mips_icache_sync_range(
		    MIPS_PHYS_TO_KSEG0(pte_to_paddr(entry) + (va & PGOFSET)),
		    len);
	}
}

/*
 *	pmap_zero_page zeros the specified page.
 */
void
pmap_zero_page(paddr_t dst_pa)
{
	pt_entry_t dst_pte;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pa=%#"PRIxPADDR")", dst_pa, 0, 0, 0);
	PMAP_COUNT(zeroed_pages);

	struct vm_page * const dst_pg = PHYS_TO_VM_PAGE(dst_pa);

	KASSERT(!VM_PAGEMD_EXECPAGE_P(VM_PAGE_TO_MD(dst_pg)));

	const register_t dst_va = pmap_md_map_ephemeral_page(dst_pg, false,
	    VM_PROT_READ|VM_PROT_WRITE, &dst_pte);

	mips_pagezero(dst_va);

	pmap_md_unmap_ephemeral_page(dst_pg, false, dst_va, dst_pte);

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

/*
 *	pmap_copy_page copies the specified page.
 */
void
pmap_copy_page(paddr_t src_pa, paddr_t dst_pa)
{
	pt_entry_t src_pte, dst_pte;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(src_pa=%#lx, dst_pa=%#lx)", src_pa, dst_pa, 0, 0);
	PMAP_COUNT(copied_pages);

	struct vm_page * const src_pg = PHYS_TO_VM_PAGE(src_pa);
	struct vm_page * const dst_pg = PHYS_TO_VM_PAGE(dst_pa);

	const register_t src_va = pmap_md_map_ephemeral_page(src_pg, false,
	    VM_PROT_READ, &src_pte);

	KASSERT(VM_PAGE_TO_MD(dst_pg)->mdpg_first.pv_pmap == NULL);
	KASSERT(!VM_PAGEMD_EXECPAGE_P(VM_PAGE_TO_MD(dst_pg)));
	const register_t dst_va = pmap_md_map_ephemeral_page(dst_pg, false,
	    VM_PROT_READ|VM_PROT_WRITE, &dst_pte);

	mips_pagecopy(dst_va, src_va);

	pmap_md_unmap_ephemeral_page(dst_pg, false, dst_va, dst_pte);
	pmap_md_unmap_ephemeral_page(src_pg, false, src_va, src_pte);

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

void
pmap_md_page_syncicache(struct vm_page *pg, const kcpuset_t *onproc)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	struct mips_options * const opts = &mips_options;
	if (opts->mips_cpu_flags & CPU_MIPS_I_D_CACHE_COHERENT)
		return;

	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

	/*
	 * If onproc is empty, we could do a
	 * pmap_page_protect(pg, VM_PROT_NONE) and remove all
	 * mappings of the page and clear its execness.  Then
	 * the next time page is faulted, it will get icache
	 * synched.  But this is easier. :)
	 */
	if (MIPS_HAS_R4K_MMU) {
		if (VM_PAGEMD_CACHED_P(mdpg)) {
			/* This was probably mapped cached by UBC so flush it */
			pt_entry_t pte;
			const register_t tva = pmap_md_map_ephemeral_page(pg, false,
			    VM_PROT_READ, &pte);

			UVMHIST_LOG(pmaphist, "  va %#"PRIxVADDR, tva, 0, 0, 0);
			mips_dcache_wbinv_range(tva, PAGE_SIZE);
			mips_icache_sync_range(tva, PAGE_SIZE);

			pmap_md_unmap_ephemeral_page(pg, false, tva, pte);
		}
	} else {
		mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(VM_PAGE_TO_PHYS(pg)),
		    PAGE_SIZE);
	}
#ifdef MULTIPROCESSOR
	pv_entry_t pv = &mdpg->mdpg_first;
	const register_t va = (intptr_t)trunc_page(pv->pv_va);
	pmap_tlb_syncicache(va, onproc);
#endif
}

struct vm_page *
pmap_md_alloc_poolpage(int flags)
{
	/*
	 * On 32bit kernels, we must make sure that we only allocate pages that
	 * can be mapped via KSEG0.  On 64bit kernels, try to allocated from
	 * the first 4G.  If all memory is in KSEG0/4G, then we can just
	 * use the default freelist otherwise we must use the pool page list.
	 */
	if (mips_poolpage_vmfreelist != VM_FREELIST_DEFAULT)
		return uvm_pagealloc_strat(NULL, 0, NULL, flags,
		    UVM_PGA_STRAT_ONLY, mips_poolpage_vmfreelist);

	return uvm_pagealloc(NULL, 0, NULL, flags);
}

vaddr_t
pmap_md_map_poolpage(paddr_t pa, size_t len)
{

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	vaddr_t va = pmap_md_pool_phystov(pa);
	KASSERT(cold || pg != NULL);
	if (pg != NULL) {
		struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
		pv_entry_t pv = &mdpg->mdpg_first;
		vaddr_t last_va = trunc_page(pv->pv_va);

		KASSERT(len == PAGE_SIZE || last_va == pa);
		KASSERT(pv->pv_pmap == NULL);
		KASSERT(pv->pv_next == NULL);
		KASSERT(!VM_PAGEMD_EXECPAGE_P(mdpg));

		/*
		 * If this page was last mapped with an address that
		 * might cause aliases, flush the page from the cache.
		 */
		if (MIPS_CACHE_VIRTUAL_ALIAS
		    && mips_cache_badalias(last_va, va)) {
			pmap_md_vca_page_wbinv(pg, false);
		}

		pv->pv_va = va;
	}
	return va;
}

paddr_t
pmap_md_unmap_poolpage(vaddr_t va, size_t len)
{
	KASSERT(len == PAGE_SIZE);
	KASSERT(pmap_md_direct_mapped_vaddr_p(va));

	const paddr_t pa = pmap_md_direct_mapped_vaddr_to_paddr(va);
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);

	KASSERT(pg);
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

	KASSERT(VM_PAGEMD_CACHED_P(mdpg));
	KASSERT(!VM_PAGEMD_EXECPAGE_P(mdpg));

	pv_entry_t pv = &mdpg->mdpg_first;

	/* Note last mapped address for future color check */
	pv->pv_va = va;

	KASSERT(pv->pv_pmap == NULL);
	KASSERT(pv->pv_next == NULL);

	return pa;
}

bool
pmap_md_direct_mapped_vaddr_p(register_t va)
{
#ifndef __mips_o32
	if (MIPS_XKPHYS_P(va))
		return true;
#endif
	return MIPS_KSEG0_P(va);
}

paddr_t
pmap_md_direct_mapped_vaddr_to_paddr(register_t va)
{
	if (MIPS_KSEG0_P(va)) {
		return MIPS_KSEG0_TO_PHYS(va);
	}
#ifndef __mips_o32
	if (MIPS_XKPHYS_P(va)) {
		return MIPS_XKPHYS_TO_PHYS(va);
	}
#endif
	panic("%s: va %#"PRIxREGISTER" not direct mapped!", __func__, va);
}

bool
pmap_md_io_vaddr_p(vaddr_t va)
{
#ifdef _LP64
	if (MIPS_XKPHYS_P(va)) {
		return MIPS_XKPHYS_TO_CCA(va) == CCA_UNCACHED;
	}
#endif
	return MIPS_KSEG1_P(va);
}

void
pmap_md_icache_sync_range_index(vaddr_t va, vsize_t len)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	mips_icache_sync_range_index(va, len);
}

void
pmap_md_icache_sync_all(void)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	mips_icache_sync_all();
}

#ifdef MULTIPROCESSOR
void
pmap_md_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
	if (ci->ci_index != 0)
		return;
	const u_int icache_way_pages =
	    mips_cache_info.mci_picache_way_size >> PGSHIFT;

	KASSERT(icache_way_pages <= 8*sizeof(pmap_tlb_synci_page_mask));
	pmap_tlb_synci_page_mask = icache_way_pages - 1;
	pmap_tlb_synci_map_mask = ~(~0 << icache_way_pages);
	printf("tlb0: synci page mask %#x and map mask %#x used for %u pages\n",
	    pmap_tlb_synci_page_mask, pmap_tlb_synci_map_mask, icache_way_pages);
}
#endif


bool
pmap_md_tlb_check_entry(void *ctx, vaddr_t va, tlb_asid_t asid, pt_entry_t pte)
{
	pmap_t pm = ctx;
	struct pmap_tlb_info * const ti = cpu_tlb_info(curcpu());
	struct pmap_asid_info * const pai = PMAP_PAI(pm, ti);

	if (asid != pai->pai_asid)
		return true;
	if (!pte_valid_p(pte)) {
		KASSERT(MIPS_HAS_R4K_MMU);
		KASSERTMSG(pte == MIPS3_PG_G, "va %#"PRIxVADDR" pte %#"PRIxPTE,
		    va, pte_value(pte));
		return true;
	}

	const pt_entry_t * const ptep = pmap_pte_lookup(pm, va);
	KASSERTMSG(ptep != NULL, "va %#"PRIxVADDR" asid %u pte %#"PRIxPTE,
	    va, asid, pte_value(pte));
	const pt_entry_t opte = *ptep;
	pt_entry_t xpte = opte;
	if (MIPS_HAS_R4K_MMU) {
		xpte &= ~(MIPS3_PG_WIRED|MIPS3_PG_RO);
	} else {
		xpte &= ~(MIPS1_PG_WIRED|MIPS1_PG_RO);
	}

        KASSERTMSG(pte == xpte,
            "pmap=%p va=%#"PRIxVADDR" asid=%u: TLB pte (%#"PRIxPTE
	    ") != real pte (%#"PRIxPTE"/%#"PRIxPTE") @ %p",
            pm, va, asid, pte_value(pte), pte_value(xpte), pte_value(opte),
	    ptep);

        return true;
}

void
tlb_walk(void *ctx, tlb_walkfunc_t func)
{
	kpreempt_disable();
	for (size_t i = 0; i < mips_options.mips_num_tlb_entries; i++) {
		struct tlbmask tlbmask;
		tlb_asid_t asid;
		vaddr_t va;
		tlb_read_entry(i, &tlbmask);
		if (MIPS_HAS_R4K_MMU) {
			asid = __SHIFTOUT(tlbmask.tlb_hi, MIPS3_PG_ASID);
			va = tlbmask.tlb_hi & MIPS3_PG_HVPN;
		} else {
			asid = __SHIFTOUT(tlbmask.tlb_hi, MIPS1_TLB_PID);
			va = tlbmask.tlb_hi & MIPS1_PG_FRAME;
		}
		if ((pt_entry_t)tlbmask.tlb_lo0 != 0) {
			pt_entry_t pte = tlbmask.tlb_lo0;
			tlb_asid_t asid0 = (pte_global_p(pte) ? KERNEL_PID : asid);
			if (!(*func)(ctx, va, asid0, pte))
				break;
		}
#if (PGSHIFT & 1) == 0
		if (MIPS_HAS_R4K_MMU && (pt_entry_t)tlbmask.tlb_lo1 != 0) {
			pt_entry_t pte = tlbmask.tlb_lo1;
			tlb_asid_t asid1 = (pte_global_p(pte) ? KERNEL_PID : asid);
			if (!(*func)(ctx, va + MIPS3_PG_ODDPG, asid1, pte))
				break;
		}
#endif
	}
	kpreempt_enable();
}

bool
pmap_md_vca_add(struct vm_page *pg, vaddr_t va, pt_entry_t *ptep)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	if (!MIPS_HAS_R4K_MMU || !MIPS_CACHE_VIRTUAL_ALIAS)
		return false;

	/*
	 * There is at least one other VA mapping this page.
	 * Check if they are cache index compatible.
	 */

	KASSERT(VM_PAGEMD_PVLIST_LOCKED_P(mdpg));
	pv_entry_t pv = &mdpg->mdpg_first;
#if defined(PMAP_NO_PV_UNCACHED)
	/*
	 * Instead of mapping uncached, which some platforms
	 * cannot support, remove incompatible mappings from others pmaps.
	 * When this address is touched again, the uvm will
	 * fault it in.  Because of this, each page will only
	 * be mapped with one index at any given time.
	 *
	 * We need to deal with all entries on the list - if the first is
	 * incompatible with the new mapping then they all will be.
	 */
	if (__predict_true(!mips_cache_badalias(pv->pv_va, va))) {
		return false;
	}
	KASSERT(pv->pv_pmap != NULL);
	bool ret = false;
	for (pv_entry_t npv = pv; npv && npv->pv_pmap;) {
		if (npv->pv_va & PV_KENTER) {
			npv = npv->pv_next;
			continue;
		}
		ret = true;
		vaddr_t nva = trunc_page(npv->pv_va);
		pmap_t npm = npv->pv_pmap;
		VM_PAGEMD_PVLIST_UNLOCK(mdpg);
		pmap_remove(npm, nva, nva + PAGE_SIZE);

		/*
		 * pmap_update is not required here as we're the pmap
		 * and we know that the invalidation happened or the
		 * asid has been released (and activation is deferred)
		 *
		 * A deferred activation should NOT occur here.
		 */
		(void)VM_PAGEMD_PVLIST_LOCK(mdpg);

		npv = pv;
	}
	KASSERT(ret == true);

	return ret;
#else	/* !PMAP_NO_PV_UNCACHED */
	if (VM_PAGEMD_CACHED_P(mdpg)) {
		/*
		 * If this page is cached, then all mappings
		 * have the same cache alias so we only need
		 * to check the first page to see if it's
		 * incompatible with the new mapping.
		 *
		 * If the mappings are incompatible, map this
		 * page as uncached and re-map all the current
		 * mapping as uncached until all pages can
		 * share the same cache index again.
		 */
		if (mips_cache_badalias(pv->pv_va, va)) {
			pmap_page_cache(pg, false);
			pmap_md_vca_page_wbinv(pg, true);
			*ptep = pte_cached_change(*ptep, false);
			PMAP_COUNT(page_cache_evictions);
		}
	} else {
		*ptep = pte_cached_change(*ptep, false);
		PMAP_COUNT(page_cache_evictions);
	}
	return false;
#endif	/* !PMAP_NO_PV_UNCACHED */
}

void
pmap_md_vca_clean(struct vm_page *pg, int op)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	if (!MIPS_HAS_R4K_MMU || !MIPS_CACHE_VIRTUAL_ALIAS)
		return;

	UVMHIST_LOG(pmaphist, "(pg=%p, op=%d)", pg, op, 0, 0);
	KASSERT(VM_PAGEMD_PVLIST_LOCKED_P(VM_PAGE_TO_MD(pg)));

	if (op == PMAP_WB || op == PMAP_WBINV) {
		pmap_md_vca_page_wbinv(pg, true);
	} else if (op == PMAP_INV) {
		KASSERT(op == PMAP_INV && false);
		//mips_dcache_inv_range_index(va, PAGE_SIZE);
	}
}

/*
 * In the PMAP_NO_PV_CACHED case, all conflicts are resolved at mapping
 * so nothing needs to be done in removal.
 */
void
pmap_md_vca_remove(struct vm_page *pg, vaddr_t va, bool dirty, bool last)
{
#if !defined(PMAP_NO_PV_UNCACHED)
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	if (!MIPS_HAS_R4K_MMU
	    || !MIPS_CACHE_VIRTUAL_ALIAS
	    || !VM_PAGEMD_UNCACHED_P(mdpg))
		return;

	KASSERT(kpreempt_disabled());
	KASSERT(!VM_PAGEMD_PVLIST_LOCKED_P(mdpg));
	KASSERT((va & PAGE_MASK) == 0);

	/*
	 * Page is currently uncached, check if alias mapping has been
	 * removed.  If it was, then reenable caching.
	 */
	(void)VM_PAGEMD_PVLIST_READLOCK(mdpg);
	pv_entry_t pv = &mdpg->mdpg_first;
	pv_entry_t pv0 = pv->pv_next;

	for (; pv0; pv0 = pv0->pv_next) {
		if (mips_cache_badalias(pv->pv_va, pv0->pv_va))
			break;
	}
	if (pv0 == NULL)
		pmap_page_cache(pg, true);
	VM_PAGEMD_PVLIST_UNLOCK(mdpg);
#endif
}

paddr_t
pmap_md_pool_vtophys(vaddr_t va)
{
#ifdef _LP64
	if (MIPS_XKPHYS_P(va))
		return MIPS_XKPHYS_TO_PHYS(va);
#endif
	KASSERT(MIPS_KSEG0_P(va));
	return MIPS_KSEG0_TO_PHYS(va);
}

vaddr_t
pmap_md_pool_phystov(paddr_t pa)
{
#ifdef _LP64
	KASSERT(mips_options.mips3_xkphys_cached);
	return MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
	KASSERT((pa & ~MIPS_PHYS_MASK) == 0);
	return MIPS_PHYS_TO_KSEG0(pa);
#endif
}
