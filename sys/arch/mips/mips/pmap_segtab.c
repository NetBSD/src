/*	$NetBSD: pmap_segtab.c,v 1.1.2.3 2010/01/20 06:58:37 matt Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: pmap_segtab.c,v 1.1.2.3 2010/01/20 06:58:37 matt Exp $");

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

#include "opt_sysv.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/pool.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#include <sys/socketvar.h>	/* XXX: for sock_loan_thresh */

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

CTASSERT(NBPG >= sizeof(struct segtab));

struct segtab	*free_segtab;		/* free list kept locally */

static inline struct vm_page *
pmap_pte_pagealloc(void)
{
	return mips_pmap_alloc_poolpage(UVM_PGA_ZERO|UVM_PGA_USERESERVE);
}

static inline pt_entry_t * 
pmap_segmap(struct pmap *pmap, vaddr_t va)
{
	struct segtab *stp = pmap->pm_segtab;
	return stp->seg_tab[va >> SEGSHIFT];
}

pt_entry_t *
pmap_pte_lookup(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *pte = pmap_segmap(pmap, va);
	if (pte == NULL)
		return NULL;

	return pte + ((va >> PGSHIFT) & (NPTEPG - 1));
}

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
void
pmap_segtab_alloc(pmap_t pmap)
{
	struct segtab *stp;

	stp = NULL;
	while (__predict_true(free_segtab != NULL)) {
		struct segtab *next_stp;
		stp = free_segtab;
		next_stp = (struct segtab *)stp->seg_tab[0];
		if (stp == atomic_cas_ptr(&free_segtab, stp, next_stp))
			break;
	}
	
	if (__predict_true(stp != NULL)) {
		stp->seg_tab[0] = NULL;
	} else {
		paddr_t stp_pa;

		for (;;) {
			struct vm_page * const stp_pg = pmap_pte_pagealloc();
			if (stp_pg != NULL) {
				stp_pa = VM_PAGE_TO_PHYS(stp_pg);
				break;
			}
			/*
			 * XXX What else can we do?  Could we
			 * XXX deadlock here?
			 */
			uvm_wait("pmap_create");
		}

#ifdef _LP64
		KASSERT(mips_options.mips3_xkphys_cached);
		stp = (struct segtab *)MIPS_PHYS_TO_XKPHYS_CACHED(stp_pa);
#else
		stp = (struct segtab *)MIPS_PHYS_TO_KSEG0(stp_pa);
#endif
		const size_t n = NBPG / sizeof(struct segtab);
		if (n > 1) {
			/*
			 * link all the segtabs in this page together
			 */
			for (size_t i = 1; i < n - 1; i++) {
				stp[i].seg_tab[0] = (void *)&stp[i+1];
			}
			/*
			 * Now link the new segtabs into the free segtab list.
			 */
			for (;;) {
				void *tmp = free_segtab;
				stp[n-1].seg_tab[0] = tmp;
				if (tmp == atomic_cas_ptr(&free_segtab, tmp, stp+1))
					break;
			}
		}
	}

#ifdef PARANOIADIAG
	for (i = 0; i < PMAP_SEGTABSIZE; i++) {
		if (stp->seg_tab[i] != 0)
			panic("pmap_create: pm_segtab.seg_tab[%zu] != 0");
	}
#endif

	pmap->pm_segtab = stp;
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_segtab_free(pmap_t pmap)
{
	struct segtab *stp = pmap->pm_segtab;

	if (stp == NULL)
		return;

	for (size_t i = 0; i < PMAP_SEGTABSIZE; i++) {
		paddr_t pa;
		/* get pointer to segment map */
		pt_entry_t *pte = stp->seg_tab[i];
		if (pte == NULL)
			continue;
#ifdef PARANOIADIAG
		for (size_t j = 0; j < NPTEPG; j++) {
			if ((pte + j)->pt_entry)
				panic("pmap_destroy: segmap not empty");
		}
#endif

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
		/*
		 * The pica pmap.c flushed the segmap pages here.  I'm
		 * not sure why, but I suspect it's because the page(s)
		 * were being accessed by KSEG0 (cached) addresses and
		 * may cause cache coherency problems when the page
		 * is reused with KSEG2 (mapped) addresses.  This may
		 * cause problems on machines without VCED/VCEI.
		 */
		if (mips_cache_info.mci_cache_virtual_alias)
			mips_dcache_inv_range((vaddr_t)pte, PAGE_SIZE);
#endif	/* MIPS3_PLUS */
#ifdef _LP64
		KASSERT(MIPS_XKPHYS_P(pte));
		pa = MIPS_XKPHYS_TO_PHYS(pte);
#else
		pa = MIPS_KSEG0_TO_PHYS(pte);
#endif
		uvm_pagefree(PHYS_TO_VM_PAGE(pa));

		stp->seg_tab[i] = NULL;
	}

	/*
	 * Insert the the segtab into the segtab freelist.
	 */
	for (;;) {
		void *tmp = free_segtab;
		stp->seg_tab[0] = tmp;
		if (tmp == atomic_cas_ptr(&free_segtab, tmp, stp))
			break;
	}
}

/*
 *	Make a new pmap (vmspace) active for the given process.
 */
void
pmap_segtab_activate(struct lwp *l)
{
	if (l == curlwp) {
		pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;
		l->l_cpu->ci_pmap_segbase = pmap->pm_segtab;
	}
}

/*
 *	Act on the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly rounded to
 *	the page size.
 */
void
pmap_pte_process(pmap_t pmap, vaddr_t sva, vaddr_t eva,
	pte_callback_t callback, uintptr_t flags)
{
#if 0
	printf("%s: %p, %"PRIxVADDR", %"PRIxVADDR", %p, %"PRIxPTR"\n",
	    __func__, pmap, sva, eva, callback, flags);
#endif
	while (sva < eva) {
		vaddr_t lastseg_va = mips_trunc_seg(sva) + NBSEG;
		KASSERT(lastseg_va != 0);
		if (lastseg_va > eva)
			lastseg_va = eva;

		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		pt_entry_t * const pte = pmap_pte_lookup(pmap, sva);
		if (pte != NULL) {
			/*
			 * Callback to deal with the ptes for this segment.
			 */
			(*callback)(pmap, sva, lastseg_va, pte, flags);
		}
		/*
		 * In theory we could release pages with no entries,
		 * but that takes more effort than we want here.
		 */
		sva = lastseg_va;
	}
}

/*
 *	Return a pointer for the pte that corresponds to the specified virtual
 *	address (va) in the target physical map, allocating if needed.
 */
pt_entry_t *
pmap_pte_reserve(pmap_t pmap, vaddr_t va, int flags)
{
	struct segtab *stp = pmap->pm_segtab;
	pt_entry_t *pte;

	pte = pmap_pte_lookup(pmap, va);
	if (__predict_false(pte == NULL)) {
		struct vm_page * const pg = pmap_pte_pagealloc();
		if (pg == NULL) {
			if (flags & PMAP_CANFAIL)
				return NULL;
			panic("%s: cannot allocate page table page "
			    "for va %" PRIxVADDR, __func__, va);
		}

		const paddr_t pa = VM_PAGE_TO_PHYS(pg);
#ifdef _LP64
		KASSERT(mips_options.mips3_xkphys_cached);
		pte = (pt_entry_t *)MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
		pte = (pt_entry_t *)MIPS_PHYS_TO_KSEG0(pa);
#endif
		stp->seg_tab[va >> SEGSHIFT] = pte;

		pte += (va >> PGSHIFT) & (NPTEPG - 1);
#ifdef PARANOIADIAG
		for (size_t i = 0; i < NPTEPG; i++) {
			if ((pte+i)->pt_entry)
				panic("pmap_enter: new segmap not empty");
		}
#endif
	}

	return pte;
}
