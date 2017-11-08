/*	$NetBSD: pmap_segtab.c,v 1.4.2.1.4.1 2017/11/08 21:22:48 snj Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: pmap_segtab.c,v 1.4.2.1.4.1 2017/11/08 21:22:48 snj Exp $");

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
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

CTASSERT(NBPG >= sizeof(union segtab));

struct pmap_segtab_info {
	union segtab *free_segtab;		/* free list kept locally */
#ifdef DEBUG
	uint32_t nget_segtab;
	uint32_t nput_segtab;
	uint32_t npage_segtab;
#define	SEGTAB_ADD(n, v)	(pmap_segtab_info.n ## _segtab += (v))
#else
#define	SEGTAB_ADD(n, v)	((void) 0)
#endif
} pmap_segtab_info;

kmutex_t pmap_segtab_lock __cacheline_aligned;

static inline struct vm_page *
pmap_pte_pagealloc(void)
{
	return mips_pmap_alloc_poolpage(UVM_PGA_ZERO|UVM_PGA_USERESERVE);
}

static inline pt_entry_t * 
pmap_segmap(struct pmap *pmap, vaddr_t va)
{
	union segtab *stp = pmap->pm_segtab;
	KASSERT(!MIPS_KSEG0_P(va));
	KASSERT(!MIPS_KSEG1_P(va));
#ifdef _LP64
	KASSERT(!MIPS_XKPHYS_P(va));
	stp = stp->seg_seg[(va >> XSEGSHIFT) & (NSEGPG - 1)];
	if (stp == NULL)
		return NULL;
#endif
	
	return stp->seg_tab[(va >> SEGSHIFT) & (PMAP_SEGTABSIZE - 1)];
}

pt_entry_t *
pmap_pte_lookup(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *pte = pmap_segmap(pmap, va);
	if (pte == NULL)
		return NULL;

	return pte + ((va >> PGSHIFT) & (NPTEPG - 1));
}

static void
pmap_segtab_free(union segtab *stp)
{
	/*
	 * Insert the the segtab into the segtab freelist.
	 */
	mutex_spin_enter(&pmap_segtab_lock);
	stp->seg_tab[0] = (void *) pmap_segtab_info.free_segtab;
	pmap_segtab_info.free_segtab = stp;
	SEGTAB_ADD(nput, 1);
	mutex_spin_exit(&pmap_segtab_lock);
}

static void
pmap_segtab_release(union segtab *stp, u_int level)
{

	for (size_t i = 0; i < PMAP_SEGTABSIZE; i++) {
		paddr_t pa;
#ifdef _LP64
		if (level > 0) {
			if (stp->seg_seg[i] != NULL) {
				pmap_segtab_release(stp->seg_seg[i], level - 1);
				stp->seg_seg[i] = NULL;
			}
			continue;
		}
#endif

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

		/* No need to flush page here as unmap poolpage does it */
#ifdef _LP64
		KASSERT(MIPS_XKPHYS_P(pte));
#endif
		pa = mips_pmap_unmap_poolpage((vaddr_t)pte);
		uvm_pagefree(PHYS_TO_VM_PAGE(pa));

		stp->seg_tab[i] = NULL;
	}

	pmap_segtab_free(stp);
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
static union segtab *
pmap_segtab_alloc(void)
{
	union segtab *stp;

 again:
	mutex_spin_enter(&pmap_segtab_lock);
	if (__predict_true((stp = pmap_segtab_info.free_segtab) != NULL)) {
		pmap_segtab_info.free_segtab =
		    (union segtab *)stp->seg_tab[0];
		stp->seg_tab[0] = NULL;
		SEGTAB_ADD(nget, 1);
	}
	mutex_spin_exit(&pmap_segtab_lock);

	if (__predict_false(stp == NULL)) {
		struct vm_page * const stp_pg = pmap_pte_pagealloc();

		if (__predict_false(stp_pg == NULL)) {
			/*
			 * XXX What else can we do?  Could we deadlock here?
			 */
			uvm_wait("pmap_create");
			goto again;
		}
		SEGTAB_ADD(npage, 1);
		const paddr_t stp_pa = VM_PAGE_TO_PHYS(stp_pg);

#ifdef _LP64
		KASSERT(mips_options.mips3_xkphys_cached);
#endif
		stp = (union segtab *)mips_pmap_map_poolpage(stp_pa);
		const size_t n = NBPG / sizeof(*stp);
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
			mutex_spin_enter(&pmap_segtab_lock);
			stp[n-1].seg_tab[0] = (void *)pmap_segtab_info.free_segtab;
			pmap_segtab_info.free_segtab = stp + 1;
			SEGTAB_ADD(nput, n - 1);
			mutex_spin_exit(&pmap_segtab_lock);
		}
	}

#ifdef PARANOIADIAG
	for (i = 0; i < PMAP_SEGTABSIZE; i++) {
		if (stp->seg_tab[i] != 0)
			panic("pmap_create: pm_segtab.seg_tab[%zu] != 0");
	}
#endif
	return stp;
}

/*
 * Allocate the top segment table for the pmap.
 */
void
pmap_segtab_init(pmap_t pmap)
{

	pmap->pm_segtab = pmap_segtab_alloc();
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_segtab_destroy(pmap_t pmap)
{
	union segtab *stp = pmap->pm_segtab;

	if (stp == NULL)
		return;

#ifdef _LP64
	pmap_segtab_release(stp, 1);
#else
	pmap_segtab_release(stp, 0);
#endif
}

/*
 *	Make a new pmap (vmspace) active for the given process.
 */
void
pmap_segtab_activate(struct pmap *pm, struct lwp *l)
{
	if (l == curlwp) {
		KASSERT(pm == l->l_proc->p_vmspace->vm_map.pmap);
#ifdef _LP64
		l->l_cpu->ci_pmap_segtab = pm->pm_segtab;
		if (pm != pmap_kernel()) {
			l->l_cpu->ci_pmap_seg0tab = pm->pm_segtab->seg_seg[0];
		} else {
			l->l_cpu->ci_pmap_seg0tab = NULL;
		}
#else
		l->l_cpu->ci_pmap_seg0tab = pm->pm_segtab;
#endif
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
	union segtab *stp = pmap->pm_segtab;
	pt_entry_t *pte;

	pte = pmap_pte_lookup(pmap, va);
	if (__predict_false(pte == NULL)) {
#ifdef _LP64
		union segtab ** const stp_p =
		    &stp->seg_seg[(va >> XSEGSHIFT) & (NSEGPG - 1)];
		if (__predict_false((stp = *stp_p) == NULL)) {
			union segtab *nstp = pmap_segtab_alloc();
#ifdef MULTIPROCESSOR
			union segtab *ostp = atomic_cas_ptr(stp_p, NULL, nstp);
			if (__predict_false(ostp != NULL)) {
				pmap_segtab_free(nstp);
				nstp = ostp;
			}
#else
			*stp_p = nstp;
#endif /* MULTIPROCESSOR */
			stp = nstp;
		}
		KASSERT(stp == pmap->pm_segtab->seg_seg[(va >> XSEGSHIFT) & (NSEGPG - 1)]);
#endif /* _LP64 */
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
#endif
		pte = (pt_entry_t *)mips_pmap_map_poolpage(pa);
		pt_entry_t ** const pte_p =
		    &stp->seg_tab[(va >> SEGSHIFT) & (PMAP_SEGTABSIZE - 1)];
#ifdef MULTIPROCESSOR
		pt_entry_t *opte = atomic_cas_ptr(pte_p, NULL, pte);
		/*
		 * If another thread allocated the segtab needed for this va
		 * free the page we just allocated.
		 */
		if (__predict_false(opte != NULL)) {
			uvm_pagefree(pg);
			pte = opte;
		}
#else
		*pte_p = pte;
#endif
		KASSERT(pte == stp->seg_tab[(va >> SEGSHIFT) & (PMAP_SEGTABSIZE - 1)]);

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
