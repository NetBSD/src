/*	$NetBSD: pmap.c,v 1.117 2022/05/26 05:34:04 skrll Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*	$OpenBSD: pmap.c,v 1.132 2008/04/18 06:42:21 djm Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * References:
 * 1. PA7100LC ERS, Hewlett-Packard, March 30 1999, Public version 1.0
 * 2. PA7300LC ERS, Hewlett-Packard, March 18 1996, Version 1.0
 * 3. PA-RISC 1.1 Architecture and Instruction Set Reference Manual,
 *    Hewlett-Packard, February 1994, Third Edition
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.117 2022/05/26 05:34:04 skrll Exp $");

#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/rwlock.h>
#include <sys/systm.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page_array.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/iomod.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>

#include <hppa/hppa/hpt.h>
#include <hppa/hppa/machdep.h>

#if defined(DDB)
#include <ddb/db_output.h>
#endif

int		pmap_hptsize = 16 * PAGE_SIZE;	/* patchable */
vaddr_t		pmap_hpt;

static struct pmap	kernel_pmap_store;
struct pmap		*const kernel_pmap_ptr = &kernel_pmap_store;

int		hppa_sid_max = HPPA_SID_MAX;
struct pool	pmap_pool;
struct pool	pmap_pv_pool;
int		pmap_pvlowat = 252;
bool		pmap_initialized = false;

static kmutex_t	pmaps_lock;

static union pmap_pv_locks {
	kmutex_t	lock;
	char		padding[COHERENCY_UNIT];
} pmap_pv_locks[64] __aligned(COHERENCY_UNIT);

#define	PMAP_PV_LOCK(md) \
    ((uintptr_t)(md) >> 7 & (__arraycount(pmap_pv_locks) - 1))

u_int	hppa_prot[8];
u_int	sid_counter;

static const struct uvm_pagerops pmap_pager = {
	/* nothing */
};

/*
 * Page 3-6 of the "PA-RISC 1.1 Architecture and Instruction Set
 * Reference Manual" (HP part number 09740-90039) defines equivalent
 * and non-equivalent virtual addresses in the cache.
 *
 * This macro evaluates to true iff the two space/virtual address
 * combinations are non-equivalent aliases, and therefore will find
 * two different locations in the cache.
 *
 * NB: currently, the CPU-specific desidhash() functions disable the
 * use of the space in all cache hashing functions.  This means that
 * this macro definition is stricter than it has to be (because it
 * takes space into account), but one day cache space hashing should
 * be re-enabled.  Cache space hashing should yield better performance
 * through better utilization of the cache, assuming that most aliasing
 * is the read-only kind, which we do allow in the cache.
 */
#define NON_EQUIVALENT_ALIAS(sp1, va1, sp2, va2) \
  (((((va1) ^ (va2)) & ~HPPA_PGAMASK) != 0) || \
   ((((sp1) ^ (sp2)) & ~HPPA_SPAMASK) != 0))

/* Prototypes. */
struct vm_page *pmap_pagealloc(struct uvm_object *, voff_t);
void pmap_pagefree(struct vm_page *);

static inline void pmap_lock(struct pmap *);
static inline void pmap_unlock(struct pmap *);
static inline bool pmap_trylock(struct pmap *);

static inline void pmap_sdir_set(pa_space_t, volatile uint32_t *);
static inline uint32_t *pmap_sdir_get(pa_space_t);

static inline volatile pt_entry_t *pmap_pde_get(volatile uint32_t *, vaddr_t);
static inline void pmap_pde_set(pmap_t, vaddr_t, paddr_t);
static inline pt_entry_t *pmap_pde_alloc(pmap_t, vaddr_t, struct vm_page **);
static inline struct vm_page *pmap_pde_ptp(pmap_t, volatile pt_entry_t *);
static inline void pmap_pde_release(pmap_t, vaddr_t, struct vm_page *);

static inline volatile pt_entry_t *pmap_pde_get(volatile uint32_t *, vaddr_t);
static inline void pmap_pde_set(pmap_t, vaddr_t, paddr_t);

void pmap_pte_flush(pmap_t, vaddr_t, pt_entry_t);

static inline pt_entry_t pmap_pte_get(volatile pt_entry_t *, vaddr_t);
static inline void pmap_pte_set(volatile pt_entry_t *, vaddr_t, pt_entry_t);

static inline pt_entry_t pmap_vp_find(pmap_t, vaddr_t);

static inline struct pv_entry *pmap_pv_alloc(void);
static inline void pmap_pv_free(struct pv_entry *);
static inline void pmap_pv_enter(struct vm_page *, struct pv_entry *, pmap_t,
    vaddr_t , struct vm_page *, u_int);
static inline struct pv_entry *pmap_pv_remove(struct vm_page *, pmap_t,
    vaddr_t);
static inline void pmap_pv_lock(const struct vm_page_md *md);
static inline void pmap_pv_unlock(const struct vm_page_md *md);
static inline bool pmap_pv_locked(const struct vm_page_md *md);

static inline void pmap_flush_page(struct vm_page *, bool);
static void pmap_resolve_alias(struct vm_page *, struct pmap *, vaddr_t,
    pt_entry_t);
static void pmap_syncicache_page(struct vm_page *, pmap_t, vaddr_t);

static void pmap_page_physload(paddr_t, paddr_t);

void pmap_copy_page(paddr_t, paddr_t);

#ifdef USE_HPT
static inline struct hpt_entry *pmap_hash(pmap_t, vaddr_t);
static inline uint32_t pmap_vtag(pmap_t, vaddr_t);

#ifdef DDB
void pmap_hptdump(void);
#endif
#endif

#ifdef DDB
void pmap_dump_table(pa_space_t, vaddr_t);
void pmap_dump_pv(paddr_t);
#endif

#define	IS_IOPAGE_P(pa)		((pa) >= HPPA_IOBEGIN)
#define	IS_PVFEXEC_P(f)		(((f) & PVF_EXEC) != 0)

/* un-invert PVF_REF */
#define pmap_pvh_attrs(a) \
	(((a) & (PVF_MOD|PVF_REF)) ^ PVF_REF)

static inline void
pmap_lock(struct pmap *pm)
{

	rw_enter(pm->pm_lock, RW_WRITER);
}

static inline void
pmap_unlock(struct pmap *pm)
{

	rw_exit(pm->pm_lock);
}

static inline bool
pmap_trylock(struct pmap *pm)
{

	return rw_tryenter(pm->pm_lock, RW_WRITER);
}

static inline void
pmap_pv_lock(const struct vm_page_md *md)
{

	mutex_enter(&pmap_pv_locks[PMAP_PV_LOCK(md)].lock);
}

static inline void
pmap_pv_unlock(const struct vm_page_md *md)
{

	mutex_exit(&pmap_pv_locks[PMAP_PV_LOCK(md)].lock);
}

static inline bool
pmap_pv_locked(const struct vm_page_md *md)
{

	return mutex_owned(&pmap_pv_locks[PMAP_PV_LOCK(md)].lock);
}

struct vm_page *
pmap_pagealloc(struct uvm_object *obj, voff_t off)
{
	struct vm_page *pg;

	if ((pg = uvm_pagealloc(obj, off, NULL,
	    UVM_PGA_USERESERVE | UVM_PGA_ZERO)) == NULL)
		printf("pmap_pagealloc fail\n");

	return (pg);
}

void
pmap_pagefree(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	pdcache(HPPA_SID_KERNEL, pa, PAGE_SIZE);

#if defined(HP8000_CPU) || defined(HP8200_CPU) || \
    defined(HP8500_CPU) || defined(HP8600_CPU)
	pdtlb(HPPA_SID_KERNEL, pa);
	pitlb(HPPA_SID_KERNEL, pa);
#endif
	uvm_pagefree(pg);
}

#ifdef USE_HPT
/*
 * This hash function is the one used by the hardware TLB walker on the 7100LC.
 */
static inline struct hpt_entry *
pmap_hash(pmap_t pmap, vaddr_t va)
{

	return (struct hpt_entry *)(pmap_hpt +
	    (((va >> 8) ^ (pmap->pm_space << 9)) & (pmap_hptsize - 1)));
}

static inline uint32_t
pmap_vtag(pmap_t pmap, vaddr_t va)
{

	return (0x80000000 | (pmap->pm_space & 0xffff) |
	    ((va >> 1) & 0x7fff0000));
}
#endif

static inline void
pmap_sdir_set(pa_space_t space, volatile uint32_t *pd)
{
	volatile uint32_t *vtop;

	mfctl(CR_VTOP, vtop);

	KASSERT(vtop != NULL);

	vtop[space] = (uint32_t)pd;
}

static inline uint32_t *
pmap_sdir_get(pa_space_t space)
{
	uint32_t *vtop;

	mfctl(CR_VTOP, vtop);
	return ((uint32_t *)vtop[space]);
}

static inline volatile pt_entry_t *
pmap_pde_get(volatile uint32_t *pd, vaddr_t va)
{

	return ((pt_entry_t *)pd[va >> 22]);
}

static inline void
pmap_pde_set(pmap_t pm, vaddr_t va, paddr_t ptp)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx ptp %#jx", (uintptr_t)pm,
	    va, ptp, 0);

	KASSERT((ptp & PGOFSET) == 0);

	pm->pm_pdir[va >> 22] = ptp;
}

static inline pt_entry_t *
pmap_pde_alloc(pmap_t pm, vaddr_t va, struct vm_page **pdep)
{
	struct vm_page *pg;
	paddr_t pa;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx pdep %#jx", (uintptr_t)pm,
	    va, (uintptr_t)pdep, 0);


	KASSERT(pm != pmap_kernel());
	KASSERT(rw_write_held(pm->pm_lock));

	pg = pmap_pagealloc(&pm->pm_obj, va);

	if (pg == NULL)
		return NULL;

	pa = VM_PAGE_TO_PHYS(pg);

	UVMHIST_LOG(maphist, "pde %#jx", pa, 0, 0, 0);

	pg->flags &= ~PG_BUSY;		/* never busy */
	pg->wire_count = 1;		/* no mappings yet */
	pmap_pde_set(pm, va, pa);
	pm->pm_stats.resident_count++;	/* count PTP as resident */
	pm->pm_ptphint = pg;
	if (pdep)
		*pdep = pg;
	return ((pt_entry_t *)pa);
}

static inline struct vm_page *
pmap_pde_ptp(pmap_t pm, volatile pt_entry_t *pde)
{
	paddr_t pa = (paddr_t)pde;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx pdep %#jx", (uintptr_t)pm,
	    (uintptr_t)pde, 0, 0);

	if (pm->pm_ptphint && VM_PAGE_TO_PHYS(pm->pm_ptphint) == pa)
		return (pm->pm_ptphint);

	UVMHIST_LOG(maphist, "<--- done (%#jx)",
	    (uintptr_t)PHYS_TO_VM_PAGE(pa), 0, 0, 0);

	return (PHYS_TO_VM_PAGE(pa));
}

static inline void
pmap_pde_release(pmap_t pmap, vaddr_t va, struct vm_page *ptp)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx ptp %#jx", (uintptr_t)pmap,
	    va, (uintptr_t)ptp, 0);

	KASSERT(pmap != pmap_kernel());
	if (--ptp->wire_count <= 1) {
		UVMHIST_LOG(maphist, "disposing ptp %#jx", (uintptr_t)ptp, 0,
		    0, 0);
		pmap_pde_set(pmap, va, 0);
		pmap->pm_stats.resident_count--;
		if (pmap->pm_ptphint == ptp)
			pmap->pm_ptphint = NULL;
		ptp->wire_count = 0;

		KASSERT((ptp->flags & PG_BUSY) == 0);

		pmap_pagefree(ptp);
	}
}

static inline pt_entry_t
pmap_pte_get(volatile pt_entry_t *pde, vaddr_t va)
{

	return (pde[(va >> 12) & 0x3ff]);
}

static inline void
pmap_pte_set(volatile pt_entry_t *pde, vaddr_t va, pt_entry_t pte)
{

	/* too verbose due to hppa_pagezero_{,un}map */
#if 0
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pdep %#jx va %#jx pte %#jx", (uintptr_t)pde,
	    va, pte, 0);
#endif

	KASSERT(pde != NULL);
	KASSERT(((paddr_t)pde & PGOFSET) == 0);

	pde[(va >> 12) & 0x3ff] = pte;
}

void
pmap_pte_flush(pmap_t pmap, vaddr_t va, pt_entry_t pte)
{

	UVMHIST_FUNC(__func__);
	if (pmap != pmap_kernel() && va != 0) {
		UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx pte %#jx",
		    (uintptr_t)pmap, va, (uintptr_t)pte, 0);
	}

	fdcache(pmap->pm_space, va, PAGE_SIZE);
	if (pte & PTE_PROT(TLB_EXECUTE)) {
		ficache(pmap->pm_space, va, PAGE_SIZE);
		pitlb(pmap->pm_space, va);
	}
	pdtlb(pmap->pm_space, va);
#ifdef USE_HPT
	if (pmap_hpt) {
		struct hpt_entry *hpt;
		hpt = pmap_hash(pmap, va);
		if (hpt->hpt_valid &&
		    hpt->hpt_space == pmap->pm_space &&
		    hpt->hpt_vpn == ((va >> 1) & 0x7fff0000))
			hpt->hpt_space = 0xffff;
	}
#endif
}

static inline pt_entry_t
pmap_vp_find(pmap_t pm, vaddr_t va)
{
	volatile pt_entry_t *pde;

	if (!(pde = pmap_pde_get(pm->pm_pdir, va)))
		return (0);

	return (pmap_pte_get(pde, va));
}

#ifdef DDB
void
pmap_dump_table(pa_space_t space, vaddr_t sva)
{
	char buf[64];
	volatile pt_entry_t *pde = NULL;
	vaddr_t va = sva;
	vaddr_t pdemask = 1;
	pt_entry_t pte;
	uint32_t *pd;

	if (space > hppa_sid_max)
		return;

	pd = pmap_sdir_get(space);
	if (!pd)
		return;

	do {
		if (pdemask != (va & PDE_MASK)) {
			pdemask = va & PDE_MASK;
			pde = pmap_pde_get(pd, va);
			if (!pde) {
				va = pdemask + PDE_SIZE;
				continue;
			}
			db_printf("%x:%8p:\n", space, pde);
		}

		pte = pmap_pte_get(pde, va);
		if (pte) {
			snprintb(buf, sizeof(buf), TLB_BITS,
			   TLB_PROT(pte & PAGE_MASK));
			db_printf("0x%08lx-0x%08x:%s\n", va, pte & ~PAGE_MASK,
			    buf);
		}
		va += PAGE_SIZE;
	} while (va != 0);
}

void
pmap_dump_pv(paddr_t pa)
{
	struct vm_page *pg;
	struct vm_page_md *md;
	struct pv_entry *pve;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg == NULL)
		return;

	md = VM_PAGE_TO_MD(pg);
	db_printf("pg %p attr 0x%08x\n", pg, md->pvh_attrs);
	for (pve = md->pvh_list; pve; pve = pve->pv_next)
		db_printf("%x:%lx\n", pve->pv_pmap->pm_space,
		    pve->pv_va & PV_VAMASK);
}
#endif

static void
pmap_resolve_alias(struct vm_page *pg, struct pmap *pm, vaddr_t va,
    pt_entry_t pte)
{

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pg %#jx pm %#jx va %#jx pte %#jx",
	    (uintptr_t)pg, (uintptr_t)pm, va, pte);

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry *pve, *npve, **pvp;

 restart:
	pmap_pv_lock(md);
	pvp = &md->pvh_list;
	for (pve = md->pvh_list; pve; pve = npve) {
		const pmap_t ppm = pve->pv_pmap;
		const vaddr_t pva = pve->pv_va & PV_VAMASK;

		UVMHIST_LOG(maphist, "... pm %#jx va %#jx", (uintptr_t)ppm,
		    pva, 0, 0);

		npve = pve->pv_next;

		volatile pt_entry_t *pde;
		pt_entry_t ppte;
		if (pve->pv_va & PV_KENTER) {
			/* Get the pte for this mapping */
			pde = pmap_pde_get(ppm->pm_pdir, pva);
			ppte = pmap_pte_get(pde, pva);
		} else {
			/*
			 * We have to seamlessly get a hold on the pmap's lock
			 * while holding the PV head lock, to know that the
			 * mapping is still in place and we can operate on it.
			 * If that can't be had, drop the PV head lock, wait
			 * for the pmap's lock to become available, and then
			 * try again.
			 */
			UVMHIST_LOG(maphist, "... pm %#jx va %#jx... checking",
			    (uintptr_t)ppm, pva, 0, 0);

			bool locked = true;
			if (pm != ppm) {
				pmap_reference(ppm);
				locked = pmap_trylock(ppm);
			}

			if (!locked) {
				pmap_pv_unlock(md);
				pmap_lock(ppm);
				/* nothing */
				pmap_unlock(ppm);
				pmap_destroy(ppm);

				UVMHIST_LOG(maphist, "... failed lock", 0, 0, 0,
				    0);
				goto restart;
			}
			pde = pmap_pde_get(ppm->pm_pdir, pva);
			ppte = pmap_pte_get(pde, pva);

			md->pvh_attrs |= pmap_pvh_attrs(ppte);
		}

		const bool writeable =
		    ((pte | ppte) & PTE_PROT(TLB_WRITE)) != 0;

		if ((va & HPPA_PGAOFF) != (pva & HPPA_PGAOFF) && writeable) {
			UVMHIST_LOG(maphist,
			    "aliased writeable mapping %#jx:%#jx",
			    ppm->pm_space, pva, 0, 0);

			pmap_pte_flush(ppm, pva, ppte);
			if (ppte & PTE_PROT(TLB_WIRED))
				ppm->pm_stats.wired_count--;
			ppm->pm_stats.resident_count--;

			if (pve->pv_va & PV_KENTER) {
				/*
				 * This is an unmanaged mapping, it must be
				 * preserved.  Move it back on the list and
				 * advance the end-of-list pointer.
				 */
				*pvp = pve;
				pvp = &pve->pv_next;
			} else {
				pmap_pte_set(pde, pva, 0);

				/* Remove pve from list */
				*pvp = npve;

				pmap_pv_unlock(md);
				pmap_pv_free(pve);
				if (pm != ppm) {
					pmap_unlock(ppm);
					pmap_destroy(ppm);

				}
				UVMHIST_LOG(maphist, "... removed", 0,
				    0, 0, 0);
				goto restart;
			}
		} else {
			UVMHIST_LOG(maphist, "not aliased writeable mapping",
			    0,0,0,0);

			if (!(pve->pv_va & PV_KENTER) && pm != ppm) {
				pmap_unlock(ppm);
				pmap_destroy(ppm);
			}
			*pvp = pve;
			pvp = &pve->pv_next;
		}
	}
	md->pvh_attrs &= ~PVF_EXEC;
	*pvp = NULL;

#ifdef DEBUG
	int ret = 0;
	/* check for non-equ aliased mappings */
	for (pve = md->pvh_list; pve; pve = pve->pv_next) {
		vaddr_t pva = pve->pv_va & PV_VAMASK;

		UVMHIST_LOG(maphist, "... pm %#jx va %#jx",
		    (uintptr_t)pve->pv_pmap, pva, 0, 0);

		pte |= pmap_vp_find(pve->pv_pmap, pva);
		if ((va & HPPA_PGAOFF) != (pva & HPPA_PGAOFF) &&
		    (pte & PTE_PROT(TLB_WRITE))) {
			UVMHIST_LOG(maphist,
			    "aliased writable mapping %#jx:%#jx",
			    pve->pv_pmap->pm_space, pve->pv_va, 0, 0);

			ret++;
		}
	}
	UVMHIST_LOG(maphist, "check returned %jd", ret, 0, 0, 0);
#endif

	pmap_pv_unlock(md);

	UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);

	return;
}

/*
 * This allocates and returns a new struct pv_entry.
 */
static inline struct pv_entry *
pmap_pv_alloc(void)
{
	struct pv_entry *pv;

	pv = pool_get(&pmap_pv_pool, PR_NOWAIT);

	return (pv);
}

static inline void
pmap_pv_free(struct pv_entry *pv)
{

	if (pv->pv_ptp)
		pmap_pde_release(pv->pv_pmap, pv->pv_va & PV_VAMASK,
		    pv->pv_ptp);

	pool_put(&pmap_pv_pool, pv);
}

static inline void
pmap_pv_enter(struct vm_page *pg, struct pv_entry *pve, pmap_t pm,
    vaddr_t va, struct vm_page *pdep, u_int flags)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pg %#jx pve %#jx pm %#jx va %#jx",
	    (uintptr_t)pg, (uintptr_t)pve, (uintptr_t)pm, va);
	UVMHIST_LOG(maphist, "...pdep %#jx flags %#jx",
	    (uintptr_t)pdep, flags, 0, 0);

	KASSERT(pmap_pv_locked(md));

	pve->pv_pmap = pm;
	pve->pv_va = va | flags;
	pve->pv_ptp = pdep;
	pve->pv_next = md->pvh_list;
	md->pvh_list = pve;
}

static inline struct pv_entry *
pmap_pv_remove(struct vm_page *pg, pmap_t pmap, vaddr_t va)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pg %#jx pm %#jx va %#jx",
	    (uintptr_t)pg, (uintptr_t)pmap, va, 0);

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry **pve, *pv;

	KASSERT(pmap_pv_locked(md));

	for (pv = *(pve = &md->pvh_list);
	    pv; pv = *(pve = &(*pve)->pv_next)) {
		if (pv->pv_pmap == pmap && (pv->pv_va & PV_VAMASK) == va) {
			*pve = pv->pv_next;
			break;
		}
	}

	if (IS_PVFEXEC_P(md->pvh_attrs)) {
		if (md->pvh_list == NULL) {
			md->pvh_attrs &= ~PVF_EXEC;
		} else {
			pmap_syncicache_page(pg, pmap, va);
		}
	}

	return (pv);
}

#define	FIRST_16M atop(16 * 1024 * 1024)

static void
pmap_page_physload(paddr_t spa, paddr_t epa)
{

	if (spa < FIRST_16M && epa <= FIRST_16M) {
		uvm_page_physload(spa, epa, spa, epa, VM_FREELIST_ISADMA);
	} else if (spa < FIRST_16M && epa > FIRST_16M) {
		uvm_page_physload(spa, FIRST_16M, spa, FIRST_16M,
		    VM_FREELIST_ISADMA);
		uvm_page_physload(FIRST_16M, epa, FIRST_16M, epa,
		    VM_FREELIST_DEFAULT);
	} else {
		uvm_page_physload(spa, epa, spa, epa, VM_FREELIST_DEFAULT);
	}

	availphysmem += epa - spa;
}

/*
 * Bootstrap the system enough to run with virtual memory.
 * Map the kernel's code, data and bss, and allocate the system page table.
 * Called with mapping OFF.
 *
 * Parameters:
 * vstart	PA of first available physical page
 */
void
pmap_bootstrap(vaddr_t vstart)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLED(maphist);

	vaddr_t va, addr;
	vsize_t size;
	extern paddr_t hppa_vtop;
	pmap_t kpm;
	int npdes, nkpdes;
	extern int resvphysmem;
	vsize_t btlb_entry_min, btlb_entry_max, btlb_entry_got;
	paddr_t ksrx, kerx, ksro, kero, ksrw, kerw;
	extern int usebtlb;

	/* Provided by the linker script */
	extern int kernel_text, etext;
	extern int __rodata_start, __rodata_end;
	extern int __data_start;

	uvm_md_init();

	hppa_prot[UVM_PROT_NONE]  = TLB_AR_NA;
	hppa_prot[UVM_PROT_READ]  = TLB_AR_R;
	hppa_prot[UVM_PROT_WRITE] = TLB_AR_RW;
	hppa_prot[UVM_PROT_RW]    = TLB_AR_RW;
	hppa_prot[UVM_PROT_EXEC]  = TLB_AR_RX;
	hppa_prot[UVM_PROT_RX]    = TLB_AR_RX;
	hppa_prot[UVM_PROT_WX]    = TLB_AR_RWX;
	hppa_prot[UVM_PROT_RWX]   = TLB_AR_RWX;

	/*
	 * Initialize kernel pmap
	 */
	addr = round_page(vstart);
	kpm = pmap_kernel();
	memset(kpm, 0, sizeof(*kpm));

	rw_init(&kpm->pm_obj_lock);
	uvm_obj_init(&kpm->pm_obj, &pmap_pager, false, 1);
	uvm_obj_setlock(&kpm->pm_obj, &kpm->pm_obj_lock);

	kpm->pm_space = HPPA_SID_KERNEL;
	kpm->pm_pid = HPPA_PID_KERNEL;
	kpm->pm_pdir_pg = NULL;
	kpm->pm_pdir = (uint32_t *)addr;

	memset((void *)addr, 0, PAGE_SIZE);
	fdcache(HPPA_SID_KERNEL, addr, PAGE_SIZE);
	addr += PAGE_SIZE;

	/*
	 * Allocate various tables and structures.
	 */
	mtctl(addr, CR_VTOP);
	hppa_vtop = addr;
	size = round_page((hppa_sid_max + 1) * 4);
	memset((void *)addr, 0, size);
	fdcache(HPPA_SID_KERNEL, addr, size);

	addr += size;
	pmap_sdir_set(HPPA_SID_KERNEL, kpm->pm_pdir);

	/*
	 * cpuid() found out how big the HPT should be, so align addr to
	 * what will be its beginning.  We don't waste the pages skipped
	 * for the alignment.
	 */
#ifdef USE_HPT
	if (pmap_hptsize) {
		struct hpt_entry *hptp;
		int i, error;

		if (addr & (pmap_hptsize - 1))
			addr += pmap_hptsize;
		addr &= ~(pmap_hptsize - 1);

		memset((void *)addr, 0, pmap_hptsize);
		hptp = (struct hpt_entry *)addr;
		for (i = pmap_hptsize / sizeof(struct hpt_entry); i--; ) {
			hptp[i].hpt_valid = 0;
			hptp[i].hpt_space = 0xffff;
			hptp[i].hpt_vpn = 0;
		}
		pmap_hpt = addr;
		addr += pmap_hptsize;

		UVMHIST_LOG(maphist, "hpt_table %#jx @ %#jx",
		    pmap_hptsize, addr, 0, 0);

		if ((error = (cpu_hpt_init)(pmap_hpt, pmap_hptsize)) < 0) {
			printf("WARNING: HPT init error %d -- DISABLED\n",
			    error);
			pmap_hpt = 0;
		} else {
			UVMHIST_LOG(maphist,
			    "HPT installed for %jd entries @ %#jx",
			    pmap_hptsize / sizeof(struct hpt_entry), addr, 0,
			    0);
		}
	}
#endif

	/* Setup vtop in lwp0 trapframe. */
	lwp0.l_md.md_regs->tf_vtop = hppa_vtop;

	/* Pre-allocate PDEs for kernel virtual */
	nkpdes = (VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / PDE_SIZE;
	/* ... and io space too */
	nkpdes += HPPA_IOLEN / PDE_SIZE;
	/* ... and all physmem (VA == PA) */
	npdes = nkpdes + (physmem + atop(PDE_SIZE) - 1) / atop(PDE_SIZE);

	UVMHIST_LOG(maphist, "npdes %jd", npdes, 0, 0, 0);

	/* map the pdes */
	for (va = 0; npdes--; va += PDE_SIZE, addr += PAGE_SIZE) {
		/* last nkpdes are for the kernel virtual */
		if (npdes == nkpdes - 1)
			va = SYSCALLGATE;
		if (npdes == HPPA_IOLEN / PDE_SIZE - 1)
			va = HPPA_IOBEGIN;
		/* now map the pde for the physmem */
		memset((void *)addr, 0, PAGE_SIZE);

		UVMHIST_LOG(maphist, "pde premap 0x%08jx 0x%08jx", va,
		    addr, 0, 0);
		pmap_pde_set(kpm, va, addr);
		kpm->pm_stats.resident_count++; /* count PTP as resident */
	}

	/*
	 * At this point we've finished reserving memory for the kernel.
	 */
	/* XXXNH */
	resvphysmem = atop(addr);

	ksrx = (paddr_t) &kernel_text;
	kerx = (paddr_t) &etext;
	ksro = (paddr_t) &__rodata_start;
	kero = (paddr_t) &__rodata_end;
	ksrw = (paddr_t) &__data_start;
	kerw = addr;

	/*
	 * The kernel text, data, and bss must be direct-mapped,
	 * because the kernel often runs in physical mode, and
	 * anyways the loader loaded the kernel into physical
	 * memory exactly where it was linked.
	 *
	 * All memory already allocated after bss, either by
	 * our caller or by this function itself, must also be
	 * direct-mapped, because it's completely unmanaged
	 * and was allocated in physical mode.
	 *
	 * BTLB entries are used to do this direct mapping.
	 * BTLB entries have a minimum and maximum possible size,
	 * and MD code gives us these sizes in units of pages.
	 */

	btlb_entry_min = (vsize_t) hppa_btlb_size_min * PAGE_SIZE;
	btlb_entry_max = (vsize_t) hppa_btlb_size_max * PAGE_SIZE;

	/*
	 * To try to conserve BTLB entries, take a hint from how
	 * the kernel was linked: take the kernel text start as
	 * our effective minimum BTLB entry size, assuming that
	 * the data segment was also aligned to that size.
	 *
	 * In practice, linking the kernel at 2MB, and aligning
	 * the data segment to a 2MB boundary, should control well
	 * how much of the BTLB the pmap uses.  However, this code
	 * should not rely on this 2MB magic number, nor should
	 * it rely on the data segment being aligned at all.  This
	 * is to allow (smaller) kernels (linked lower) to work fine.
	 */
	btlb_entry_min = (vaddr_t) &kernel_text;

	if (usebtlb) {
#define BTLB_SET_SIZE 16
		vaddr_t btlb_entry_start[BTLB_SET_SIZE];
		vsize_t btlb_entry_size[BTLB_SET_SIZE];
		int btlb_entry_vm_prot[BTLB_SET_SIZE];
		int btlb_i;
		int btlb_j;

		/*
		 * Now make BTLB entries to direct-map the kernel text
		 * read- and execute-only as much as possible.  Note that
		 * if the data segment isn't nicely aligned, the last
		 * BTLB entry for the kernel text may also cover some of
		 * the data segment, meaning it will have to allow writing.
		 */
		addr = ksrx;

		UVMHIST_LOG(maphist,
		    "BTLB mapping text and rodata @ %#jx - %#jx", addr, kero,
		    0, 0);

		btlb_j = 0;
		while (addr < (vaddr_t) kero) {

			/* Set up the next BTLB entry. */
			KASSERT(btlb_j < BTLB_SET_SIZE);
			btlb_entry_start[btlb_j] = addr;
			btlb_entry_size[btlb_j] = btlb_entry_min;
			btlb_entry_vm_prot[btlb_j] =
			    VM_PROT_READ | VM_PROT_EXECUTE;
			if (addr + btlb_entry_min > kero)
				btlb_entry_vm_prot[btlb_j] |= VM_PROT_WRITE;

			/* Coalesce BTLB entries whenever possible. */
			while (btlb_j > 0 &&
			    btlb_entry_vm_prot[btlb_j] ==
				btlb_entry_vm_prot[btlb_j - 1] &&
			    btlb_entry_size[btlb_j] ==
				btlb_entry_size[btlb_j - 1] &&
			    !(btlb_entry_start[btlb_j - 1] &
				((btlb_entry_size[btlb_j - 1] << 1) - 1)) &&
			    (btlb_entry_size[btlb_j - 1] << 1) <=
				btlb_entry_max)
				btlb_entry_size[--btlb_j] <<= 1;

			/* Move on. */
			addr =
			    btlb_entry_start[btlb_j] + btlb_entry_size[btlb_j];
			btlb_j++;
		}

		/*
		 * Now make BTLB entries to direct-map the kernel data,
		 * bss, and all of the preallocated space read-write.
		 *
		 * Note that, unlike above, we're not concerned with
		 * making these BTLB entries such that they finish as
		 * close as possible to the end of the space we need
		 * them to map.  Instead, to minimize the number of BTLB
		 * entries we need, we make them as large as possible.
		 * The only thing this wastes is kernel virtual space,
		 * which is plentiful.
		 */

		UVMHIST_LOG(maphist, "mapping data, bss, etc @ %#jx - %#jx",
		    addr, kerw, 0, 0);

		while (addr < kerw) {

			/* Make the next BTLB entry. */
			KASSERT(btlb_j < BTLB_SET_SIZE);
			size = btlb_entry_min;
			while ((addr + size) < kerw &&
				(size << 1) < btlb_entry_max &&
			    !(addr & ((size << 1) - 1)))
				size <<= 1;
			btlb_entry_start[btlb_j] = addr;
			btlb_entry_size[btlb_j] = size;
			btlb_entry_vm_prot[btlb_j] =
			    VM_PROT_READ | VM_PROT_WRITE;

			/* Move on. */
			addr =
			    btlb_entry_start[btlb_j] + btlb_entry_size[btlb_j];
			btlb_j++;
		}

		/* Now insert all of the BTLB entries. */
		for (btlb_i = 0; btlb_i < btlb_j; btlb_i++) {
			int error;
			int prot;

			btlb_entry_got = btlb_entry_size[btlb_i];
			prot = btlb_entry_vm_prot[btlb_i];

			error = hppa_btlb_insert(kpm->pm_space,
			    btlb_entry_start[btlb_i], btlb_entry_start[btlb_i],
			    &btlb_entry_got,
			    kpm->pm_pid | pmap_prot(kpm, prot));

			if (error)
				panic("%s: cannot insert BTLB entry",
				    __func__);
			if (btlb_entry_got != btlb_entry_size[btlb_i])
				panic("%s: BTLB entry mapped wrong amount",
				    __func__);
		}

		kerw =
		    btlb_entry_start[btlb_j - 1] + btlb_entry_size[btlb_j - 1];
	}

	/*
	 * We now know the exact beginning of managed kernel virtual space.
	 *
	 * Finally, load physical pages into UVM.  There are three segments of
	 * pages.
	 */

	availphysmem = 0;

	pmap_page_physload(resvmem, atop(ksrx));
	pmap_page_physload(atop(kero), atop(ksrw));
	pmap_page_physload(atop(kerw), physmem);

	mutex_init(&pmaps_lock, MUTEX_DEFAULT, IPL_NONE);

	/* TODO optimize/inline the kenter */
	for (va = PAGE_SIZE; va < ptoa(physmem); va += PAGE_SIZE) {
		vm_prot_t prot = UVM_PROT_RW;

		if (va < resvmem)
			prot = UVM_PROT_RX;
		else if (va >= ksrx && va < kerx)
			prot = UVM_PROT_RX;
		else if (va >= ksro && va < kero)
			prot = UVM_PROT_R;
#ifdef DIAGNOSTIC
		else if (va == uvm_lwp_getuarea(&lwp0) + USPACE - PAGE_SIZE)
			prot = UVM_PROT_NONE;
#endif
		pmap_kenter_pa(va, va, prot, PMAP_DIRECTMAP);
	}

	/* XXXNH update */
	UVMHIST_LOG(maphist, "mapped %#jx - %#jx", ksro, kero, 0, 0);
	UVMHIST_LOG(maphist, "mapped %#jx - %#jx", ksrw, kerw, 0, 0);

}

/*
 * Finishes the initialization of the pmap module.
 * This procedure is called from uvm_init() in uvm/uvm_init.c
 * to initialize any remaining data structures that the pmap module
 * needs to map virtual memory (VM is already ON).
 */
void
pmap_init(void)
{
	extern void gateway_page(void);
	volatile pt_entry_t *pde;
	int i;

	UVMHIST_FUNC(__func__)
	UVMHIST_CALLED(maphist);

	sid_counter = HPPA_SID_KERNEL;

	pool_init(&pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pmappv",
	    &pool_allocator_nointr, IPL_NONE);

	pool_setlowat(&pmap_pv_pool, pmap_pvlowat);
	pool_sethiwat(&pmap_pv_pool, pmap_pvlowat * 32);

	/*
	 * map SysCall gateway page once for everybody
	 * NB: we'll have to remap the phys memory
	 *     if we have any at SYSCALLGATE address (;
	 *
	 * no spls since no interrupts
	 */
	if (!(pde = pmap_pde_get(pmap_kernel()->pm_pdir, SYSCALLGATE)) &&
	    !(pde = pmap_pde_alloc(pmap_kernel(), SYSCALLGATE, NULL)))
		panic("pmap_init: cannot allocate pde");

	pmap_pte_set(pde, SYSCALLGATE, (paddr_t)&gateway_page |
	    PTE_PROT(TLB_GATE_PROT));

	for (i = 0; i < __arraycount(pmap_pv_locks); i++)
		mutex_init(&pmap_pv_locks[i].lock, MUTEX_DEFAULT, IPL_VM);

	pmap_initialized = true;

	UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);
}

/*
 * How much virtual space does this kernel have?
 */
void
pmap_virtual_space(vaddr_t *startp, vaddr_t *endp)
{

	*startp = SYSCALLGATE + PAGE_SIZE;
	*endp = VM_MAX_KERNEL_ADDRESS;
}

/*
 * pmap_create()
 *
 * Create and return a physical map.
 * The map is an actual physical map, and may be referenced by the hardware.
 */
pmap_t
pmap_create(void)
{
	pmap_t pmap;
	pa_space_t space;

	UVMHIST_FUNC(__func__)
	UVMHIST_CALLED(maphist);

	pmap = pool_get(&pmap_pool, PR_WAITOK);

	UVMHIST_LOG(maphist, "pm %#jx", (uintptr_t)pmap, 0, 0, 0);

	rw_init(&pmap->pm_obj_lock);
	uvm_obj_init(&pmap->pm_obj, &pmap_pager, false, 1);
	uvm_obj_setlock(&pmap->pm_obj, &pmap->pm_obj_lock);

	mutex_enter(&pmaps_lock);

	/*
	 * Allocate space IDs for the pmap; we get the protection ID from this.
	 * If all are allocated, there is nothing we can do.
	 */
	/* XXXNH can't this loop forever??? */
	for (space = sid_counter; pmap_sdir_get(space);
	    space = (space + 1) % hppa_sid_max)
		;

	if ((pmap->pm_pdir_pg = pmap_pagealloc(NULL, 0)) == NULL)
		panic("pmap_create: no pages");
	pmap->pm_ptphint = NULL;
	pmap->pm_pdir = (uint32_t *)VM_PAGE_TO_PHYS(pmap->pm_pdir_pg);
	pmap_sdir_set(space, pmap->pm_pdir);

	pmap->pm_space = space;
	pmap->pm_pid = (space + 1) << 1;

	pmap->pm_stats.resident_count = 1;
	pmap->pm_stats.wired_count = 0;

	mutex_exit(&pmaps_lock);

	UVMHIST_LOG(maphist, "pm %#jx, space %jd, pid %jd",
	    (uintptr_t)pmap, space, pmap->pm_pid, 0);

	return (pmap);
}

/*
 * pmap_destroy(pmap)
 *	Gives up a reference to the specified pmap.  When the reference count
 *	reaches zero the pmap structure is added to the pmap free list.
 *	Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pmap)
{
	UVMHIST_FUNC(__func__)
	UVMHIST_CALLARGS(maphist, "pm %#jx", (uintptr_t)pmap, 0, 0, 0);

#ifdef DIAGNOSTIC
	struct uvm_page_array a;
	struct vm_page *pg;
	off_t off;
#endif

	membar_release();
	if (atomic_dec_uint_nv(&pmap->pm_obj.uo_refs) > 0)
		return;
	membar_acquire();

#ifdef DIAGNOSTIC
	uvm_page_array_init(&a, &pmap->pm_obj, 0);
	off = 0;
	rw_enter(pmap->pm_lock, RW_WRITER);
	while ((pg = uvm_page_array_fill_and_peek(&a, off, 0)) != NULL) {
		pt_entry_t *pde, *epde;
		struct vm_page *spg;
		struct pv_entry *pv, *npv;
		paddr_t pa;
		vaddr_t va;

		off = pg->offset + PAGE_SIZE;
		uvm_page_array_advance(&a);
		KASSERT(pg != pmap->pm_pdir_pg);
		pa = VM_PAGE_TO_PHYS(pg);

		UVMHIST_LOG(maphist, "pm %#jx: stray ptp %#jx w/ %jd entries:",
		    (uintptr_t)pmap, pa, pg->wire_count - 1, 0);

		pde = (pt_entry_t *)pa;
		epde = (pt_entry_t *)(pa + PAGE_SIZE);
		for (; pde < epde; pde++) {
			if (*pde == 0)
				continue;

			spg = PHYS_TO_VM_PAGE(PTE_PAGE(*pde));
			if (spg == NULL)
				continue;

			struct vm_page_md * const md = VM_PAGE_TO_MD(spg);
			pmap_pv_lock(md);
			for (pv = md->pvh_list; pv != NULL; pv = npv) {
				npv = pv->pv_next;
				if (pv->pv_pmap != pmap)
					continue;

				UVMHIST_LOG(maphist, " %#jx", pv->pv_va, 0, 0,
				    0);

				va = pv->pv_va & PV_VAMASK;
				pmap_pv_unlock(md);
				pmap_remove(pmap, va, va + PAGE_SIZE);
				pmap_pv_lock(md);
				/* List may have changed: restart. */
				npv = md->pvh_list;
			}
			pmap_pv_unlock(md);
		}
	}
	rw_exit(pmap->pm_lock);
	uvm_page_array_fini(&a);
#endif
	pmap_sdir_set(pmap->pm_space, 0);
	rw_enter(pmap->pm_lock, RW_WRITER);
	pmap_pagefree(pmap->pm_pdir_pg);
	rw_exit(pmap->pm_lock);

	uvm_obj_destroy(&pmap->pm_obj, false);
	rw_destroy(&pmap->pm_obj_lock);
	pool_put(&pmap_pool, pmap);
}

/*
 * Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	UVMHIST_FUNC(__func__)
	UVMHIST_CALLARGS(maphist, "pm %#jx", (uintptr_t)pmap, 0, 0, 0);

	atomic_inc_uint(&pmap->pm_obj.uo_refs);
}


void
pmap_syncicache_page(struct vm_page *pg, pmap_t pm, vaddr_t va)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry *pve = md->pvh_list;

	for (; pve; pve = pve->pv_next) {
		pmap_t fpm = pve->pv_pmap;
		vaddr_t fva = pve->pv_va & PV_VAMASK;
		pt_entry_t pte = pmap_vp_find(fpm, fva);

		if ((pte & PTE_PROT(TLB_DIRTY)) == 0)
			continue;

		/* Don't attempt to use the mapping we're adding */
		if (pm == fpm && va == fva)
			continue;

		fdcache(fpm->pm_space, fva, PAGE_SIZE);
		ficache(fpm->pm_space, fva, PAGE_SIZE);
		break;
	}
}

/*
 * pmap_enter(pmap, va, pa, prot, flags)
 *	Create a translation for the virtual address (va) to the physical
 *	address (pa) in the pmap with the protection requested. If the
 *	translation is wired then we can not allow a page fault to occur
 *	for this mapping.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	volatile pt_entry_t *pde;
	pt_entry_t pte;
	struct vm_page *pg = NULL, *ptp = NULL;
	struct pv_entry *pve = NULL;
	bool wired = (flags & PMAP_WIRED) != 0;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx pa %#jx prot %#jx",
	    (uintptr_t)pmap, va, pa, prot);
	UVMHIST_LOG(maphist, "...flags %#jx", flags, 0, 0, 0);

	pmap_lock(pmap);

	if (!(pde = pmap_pde_get(pmap->pm_pdir, va)) &&
	    !(pde = pmap_pde_alloc(pmap, va, &ptp))) {
		if (flags & PMAP_CANFAIL) {
			pmap_unlock(pmap);
			return (ENOMEM);
		}

		panic("pmap_enter: cannot allocate pde");
	}

	if (!ptp)
		ptp = pmap_pde_ptp(pmap, pde);

	if ((pte = pmap_pte_get(pde, va))) {
		UVMHIST_LOG(maphist, "remapping %#jx -> %#jx", pte, pa, 0, 0);

		pmap_pte_flush(pmap, va, pte);
		if (wired && !(pte & PTE_PROT(TLB_WIRED)))
			pmap->pm_stats.wired_count++;
		else if (!wired && (pte & PTE_PROT(TLB_WIRED)))
			pmap->pm_stats.wired_count--;

		pg = PHYS_TO_VM_PAGE(PTE_PAGE(pte));
		if (PTE_PAGE(pte) == pa) {
			UVMHIST_LOG(maphist, "same page", 0, 0, 0, 0);
			goto enter;
		}

		if (pg != NULL) {
			struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

			pmap_pv_lock(md);
			pve = pmap_pv_remove(pg, pmap, va);
			md->pvh_attrs |= pmap_pvh_attrs(pte);
			pmap_pv_unlock(md);
		}
	} else {
		UVMHIST_LOG(maphist, "new mapping %#jx -> %#jx",
		    va, pa, 0, 0);
		pte = PTE_PROT(TLB_REFTRAP);
		pmap->pm_stats.resident_count++;
		if (wired)
			pmap->pm_stats.wired_count++;
		if (ptp)
			ptp->wire_count++;
	}

	if (pmap_initialized && (pg = PHYS_TO_VM_PAGE(pa))) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

		if (!pve && !(pve = pmap_pv_alloc())) {
			if (flags & PMAP_CANFAIL) {
 				pmap_unlock(pmap);
				return (ENOMEM);
			}
			panic("%s: no pv entries available", __func__);
		}
		pte |= PTE_PROT(pmap_prot(pmap, prot));
		pmap_resolve_alias(pg, pmap, va, pte);

		pmap_pv_lock(md);
		pmap_pv_enter(pg, pve, pmap, va, ptp, 0);
		pmap_pv_unlock(md);
	} else if (pve) {
		pmap_pv_free(pve);
	}

enter:
	/* preserve old ref & mod */
	pte = pa | PTE_PROT(pmap_prot(pmap, prot)) |
	    (pte & PTE_PROT(TLB_UNCACHEABLE|TLB_DIRTY|TLB_REFTRAP));

	if (pg != NULL) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

		if ((pte & PTE_PROT(TLB_EXECUTE)) != 0 &&
		    !IS_PVFEXEC_P(md->pvh_attrs)) {
			pmap_syncicache_page(pg, pmap, va);
			md->pvh_attrs |= PVF_EXEC;
		}
	}

	if (IS_IOPAGE_P(pa))
		pte |= PTE_PROT(TLB_UNCACHEABLE);
	if (wired)
		pte |= PTE_PROT(TLB_WIRED);
	pmap_pte_set(pde, va, pte);

	pmap_unlock(pmap);

	UVMHIST_LOG(maphist, "<--- done (0)", 0, 0, 0, 0);

	return (0);
}

/*
 * pmap_remove(pmap, sva, eva)
 *	unmaps all virtual addresses in the virtual address
 *	range determined by [sva, eva) and pmap.
 *	sva and eva must be on machine independent page boundaries and
 *	sva must be less than or equal to eva.
 */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "sva %#jx eva %#jx", sva, eva, 0, 0);

	struct pv_entry *pve;
	volatile pt_entry_t *pde = NULL;
	pt_entry_t pte;
	struct vm_page *pg, *ptp;
	vaddr_t pdemask;
	int batch;

	pmap_lock(pmap);

	for (batch = 0; sva < eva; sva += PAGE_SIZE) {
		pdemask = sva & PDE_MASK;
		if (!(pde = pmap_pde_get(pmap->pm_pdir, sva))) {
			sva = pdemask + PDE_SIZE - PAGE_SIZE;
			continue;
		}
		batch = pdemask == sva && sva + PDE_SIZE <= eva;

		if ((pte = pmap_pte_get(pde, sva))) {

			/* TODO measure here the speed tradeoff
			 * for flushing whole 4M vs per-page
			 * in case of non-complete pde fill
			 */
			pmap_pte_flush(pmap, sva, pte);
			if (pte & PTE_PROT(TLB_WIRED))
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;

			/* iff properly accounted pde will be dropped anyway */
			if (!batch)
				pmap_pte_set(pde, sva, 0);

			if (pmap_initialized &&
			    (pg = PHYS_TO_VM_PAGE(PTE_PAGE(pte)))) {
				struct vm_page_md * const md =
				    VM_PAGE_TO_MD(pg);

				pmap_pv_lock(md);
				pve = pmap_pv_remove(pg, pmap, sva);
				md->pvh_attrs |= pmap_pvh_attrs(pte);
				pmap_pv_unlock(md);

				if (pve != NULL)
					pmap_pv_free(pve);
			} else {
				if (IS_IOPAGE_P(PTE_PAGE(pte))) {
					ptp = pmap_pde_ptp(pmap, pde);
					if (ptp != NULL)
						pmap_pde_release(pmap, sva,
						    ptp);
				}
			}
		}
	}

	pmap_unlock(pmap);

	UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);
}

void
pmap_write_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx sva %#jx eva %#jx prot %#jx",
	    (uintptr_t)pmap, sva, eva, prot);

	struct vm_page *pg;
	volatile pt_entry_t *pde = NULL;
	pt_entry_t pte;
	u_int pteprot, pdemask;

	sva = trunc_page(sva);
	pteprot = PTE_PROT(pmap_prot(pmap, prot));

	pmap_lock(pmap);

	for (pdemask = 1; sva < eva; sva += PAGE_SIZE) {
		if (pdemask != (sva & PDE_MASK)) {
			pdemask = sva & PDE_MASK;
			if (!(pde = pmap_pde_get(pmap->pm_pdir, sva))) {
				sva = pdemask + PDE_SIZE - PAGE_SIZE;
				continue;
			}
		}
		if ((pte = pmap_pte_get(pde, sva))) {
			UVMHIST_LOG(maphist, "va% #jx pte %#jx", sva, pte,
			    0, 0);
			/*
			 * Determine if mapping is changing.
			 * If not, nothing to do.
			 */
			if ((pte & PTE_PROT(TLB_AR_MASK)) == pteprot)
				continue;

			pg = PHYS_TO_VM_PAGE(PTE_PAGE(pte));
			if (pg != NULL) {
				struct vm_page_md * const md =
				    VM_PAGE_TO_MD(pg);

				pmap_pv_lock(md);
				md->pvh_attrs |= pmap_pvh_attrs(pte);
				pmap_pv_unlock(md);
			}

			/* Add TLB_EXECUTE if PVF_EXEC ??? */
			pmap_pte_flush(pmap, sva, pte);
			pte &= ~PTE_PROT(TLB_AR_MASK);
			pte |= pteprot;
			pmap_pte_set(pde, sva, pte);
		}
	}

	pmap_unlock(pmap);
}

void
pmap_page_remove(struct vm_page *pg)
{
	UVMHIST_FUNC(__func__)
	UVMHIST_CALLARGS(maphist, "pg %#jx", (uintptr_t)pg, 0, 0, 0);

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry *pve, *npve, **pvp;

	if (md->pvh_list == NULL) {
		KASSERT((md->pvh_attrs & PVF_EXEC) == 0);
		return;
	}

 restart:
	pmap_pv_lock(md);
	pvp = &md->pvh_list;
	for (pve = md->pvh_list; pve; pve = npve) {
		pmap_t pmap = pve->pv_pmap;
		vaddr_t va = pve->pv_va & PV_VAMASK;
		volatile pt_entry_t *pde;
		pt_entry_t pte;
		bool locked;

		UVMHIST_LOG(maphist, "... pm %#jx va %#jx", (uintptr_t)pmap,
		    va, 0, 0);

		npve = pve->pv_next;
		if (pve->pv_va & PV_KENTER) {
			/*
			 * This is an unmanaged mapping, it must be preserved.
			 * Move it back on the list and advance the end-of-list
			 * pointer.
			 */
			*pvp = pve;
			pvp = &pve->pv_next;

			/* Get the pte for this mapping */
			pde = pmap_pde_get(pmap->pm_pdir, va);
			pte = pmap_pte_get(pde, va);
		} else {
			/*
			 * We have to seamlessly get a hold on the pmap's lock
			 * while holding the PV head lock, to know that the
			 * mapping is still in place and we can operate on it.
			 * If that can't be had, drop the PV head lock, wait
			 * for the pmap's lock to become available, and then
			 * try again.
			 */
			UVMHIST_LOG(maphist, "... pm %#jx va %#jx... removing",
			    (uintptr_t)pmap, va, 0, 0);

			pmap_reference(pmap);
			locked = pmap_trylock(pmap);
			if (!locked) {
				pmap_pv_unlock(md);
				pmap_lock(pmap);
				/* nothing */
				pmap_unlock(pmap);
				pmap_destroy(pmap);

				UVMHIST_LOG(maphist, "... failed lock", 0, 0, 0,
				    0);
				goto restart;
			}
			pde = pmap_pde_get(pmap->pm_pdir, va);
			pte = pmap_pte_get(pde, va);

			md->pvh_attrs |= pmap_pvh_attrs(pte);
		}

		pmap_pte_flush(pmap, va, pte);
		if (pte & PTE_PROT(TLB_WIRED))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;

		if (!(pve->pv_va & PV_KENTER)) {
			pmap_pte_set(pde, va, 0);

			pmap_pv_unlock(md);
			pmap_pv_free(pve);
			pmap_unlock(pmap);
			pmap_destroy(pmap);
			UVMHIST_LOG(maphist, "... removed", 0, 0, 0, 0);
			*pvp = npve;
			goto restart;
		}
	}
	md->pvh_attrs &= ~PVF_EXEC;
	*pvp = NULL;

	pmap_pv_unlock(md);

	UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);
}

/*
 *	Routine:	pmap_unwire
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 *
 * Change the wiring for a given virtual page. This routine currently is
 * only used to unwire pages and hence the mapping entry will exist.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx", (uintptr_t)pmap, va, 0, 0);

	volatile pt_entry_t *pde;
	pt_entry_t pte = 0;

	pmap_lock(pmap);
	if ((pde = pmap_pde_get(pmap->pm_pdir, va))) {
		pte = pmap_pte_get(pde, va);

		KASSERT(pte);

		if (pte & PTE_PROT(TLB_WIRED)) {
			pte &= ~PTE_PROT(TLB_WIRED);
			pmap->pm_stats.wired_count--;
			pmap_pte_set(pde, va, pte);
		}
	}
	pmap_unlock(pmap);

	UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);
}

bool
pmap_changebit(struct vm_page *pg, u_int set, u_int clear)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pg %#jx (md %#jx) set %#jx clear %#jx",
	    (uintptr_t)pg, (uintptr_t)VM_PAGE_TO_MD(pg), set, clear);

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry *pve;
	int res;

	KASSERT((set & clear) == 0);
	KASSERT((set & ~(PVF_REF|PVF_UNCACHEABLE)) == 0);
	KASSERT((clear & ~(PVF_MOD|PVF_WRITE|PVF_UNCACHEABLE)) == 0);

	/* preserve other bits */
	pmap_pv_lock(md);
	res = md->pvh_attrs & (set | clear);
	md->pvh_attrs ^= res;

	for (pve = md->pvh_list; pve; pve = pve->pv_next) {
		pmap_t pmap = pve->pv_pmap;
		vaddr_t va = pve->pv_va & PV_VAMASK;
		volatile pt_entry_t *pde;
		pt_entry_t opte, pte;

		if ((pde = pmap_pde_get(pmap->pm_pdir, va))) {
			opte = pte = pmap_pte_get(pde, va);
#ifdef DEBUG
			if (!pte) {
				UVMHIST_LOG(maphist, "zero pte for %#jx",
				    va, 0, 0, 0);
				continue;
			}
#endif
			pte &= ~clear;
			pte |= set;

			if (!(pve->pv_va & PV_KENTER)) {
				md->pvh_attrs |= pmap_pvh_attrs(pte);
				res |= pmap_pvh_attrs(opte);
			}

			if (opte != pte) {
				pmap_pte_flush(pmap, va, opte);
				pmap_pte_set(pde, va, pte);
			}
		}
	}
	pmap_pv_unlock(md);

	return ((res & (clear | set)) != 0);
}

bool
pmap_testbit(struct vm_page *pg, u_int bit)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pg %#jx (md %#jx) bit %#jx",
	    (uintptr_t)pg, (uintptr_t)VM_PAGE_TO_MD(pg), bit, 0);

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry *pve;
	pt_entry_t pte;
	int ret;

	pmap_pv_lock(md);
	for (pve = md->pvh_list; !(md->pvh_attrs & bit) && pve;
	    pve = pve->pv_next) {
		pmap_t pm = pve->pv_pmap;

		pte = pmap_vp_find(pm, pve->pv_va & PV_VAMASK);
		if (pve->pv_va & PV_KENTER)
			continue;

		md->pvh_attrs |= pmap_pvh_attrs(pte);
	}
	ret = ((md->pvh_attrs & bit) != 0);
	pmap_pv_unlock(md);

	return ret;
}

/*
 * pmap_extract(pmap, va, pap)
 *	fills in the physical address corresponding to the
 *	virtual address specified by pmap and va into the
 *	storage pointed to by pap and returns true if the
 *	virtual address is mapped. returns false in not mapped.
 */
bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pm %#jx va %#jx", (uintptr_t)pmap, va, 0, 0);

	pt_entry_t pte;


	if (pmap != pmap_kernel()) {
		pmap_lock(pmap);
		pte = pmap_vp_find(pmap, va);
		pmap_unlock(pmap);
	} else {
		pte = pmap_vp_find(pmap, va);
	}

	if (pte) {
		if (pap)
			*pap = (pte & ~PGOFSET) | (va & PGOFSET);
		return true;
	}

	return false;
}

/*
 * pmap_activate(lwp)
 *
 *	Activates the vmspace for the given LWP.
 *	This is not necessarily the current LWP.
 */
void
pmap_activate(struct lwp *l)
{
	struct proc *p = l->l_proc;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	pa_space_t space = pmap->pm_space;
	struct pcb *pcb = lwp_getpcb(l);

	/* space is cached for the copy{in,out}'s pleasure */
	pcb->pcb_space = space;
	fdcache(HPPA_SID_KERNEL, (vaddr_t)pcb, sizeof(struct pcb));

	if (p == curproc)
		mtctl(pmap->pm_pid, CR_PIDR2);
}

void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	pmap_t pmap = p->p_vmspace->vm_map.pmap;

	fdcache(pmap->pm_space, va, len);
	ficache(pmap->pm_space, va, len);
}

static inline void
pmap_flush_page(struct vm_page *pg, bool purge)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pg %#jx (md %#jx) purge %jd",
	    (uintptr_t)pg, (uintptr_t)VM_PAGE_TO_MD(pg), purge, 0);

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	struct pv_entry *pve;

	/* purge cache for all possible mappings for the pa */
	for (pve = md->pvh_list; pve; pve = pve->pv_next) {
		vaddr_t va = pve->pv_va & PV_VAMASK;
		pa_space_t sp = pve->pv_pmap->pm_space;

		if (purge)
			pdcache(sp, va, PAGE_SIZE);
		else
			fdcache(sp, va, PAGE_SIZE);
#if defined(HP8000_CPU) || defined(HP8200_CPU) || \
    defined(HP8500_CPU) || defined(HP8600_CPU)
		ficache(sp, va, PAGE_SIZE);
		pdtlb(sp, va);
		pitlb(sp, va);
#endif
	}
}

/*
 * pmap_zero_page(pa)
 *
 * Zeros the specified page.
 */
void
pmap_zero_page(paddr_t pa)
{

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "pa %#jx (pg %#jx)", pa,
	    (uintptr_t)PHYS_TO_VM_PAGE(pa), 0, 0);

	KASSERT(VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(pa))->pvh_list == NULL);
	KASSERT((VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(pa))->pvh_attrs & PVF_EXEC) == 0);

	memset((void *)pa, 0, PAGE_SIZE);
	fdcache(HPPA_SID_KERNEL, pa, PAGE_SIZE);

#if defined(HP8000_CPU) || defined(HP8200_CPU) || \
    defined(HP8500_CPU) || defined(HP8600_CPU)
	ficache(HPPA_SID_KERNEL, pa, PAGE_SIZE);
	pdtlb(HPPA_SID_KERNEL, pa);
	pitlb(HPPA_SID_KERNEL, pa);
#endif
}

/*
 * pmap_copy_page(src, dst)
 *
 * pmap_copy_page copies the source page to the destination page.
 */
void
pmap_copy_page(paddr_t spa, paddr_t dpa)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist, "spa %#jx (pg %#jx) dpa %#jx (pg %#jx)",
	    spa, (uintptr_t)PHYS_TO_VM_PAGE(spa),
	    dpa, (uintptr_t)PHYS_TO_VM_PAGE(dpa));

	struct vm_page *srcpg = PHYS_TO_VM_PAGE(spa);

	KASSERT(VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(dpa))->pvh_list == NULL);
	KASSERT((VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(dpa))->pvh_attrs & PVF_EXEC) == 0);

	pmap_flush_page(srcpg, false);

	memcpy((void *)dpa, (void *)spa, PAGE_SIZE);

	pdcache(HPPA_SID_KERNEL, spa, PAGE_SIZE);
	fdcache(HPPA_SID_KERNEL, dpa, PAGE_SIZE);
#if defined(HP8000_CPU) || defined(HP8200_CPU) || \
    defined(HP8500_CPU) || defined(HP8600_CPU)
	ficache(HPPA_SID_KERNEL, spa, PAGE_SIZE);
	ficache(HPPA_SID_KERNEL, dpa, PAGE_SIZE);
	pdtlb(HPPA_SID_KERNEL, spa);
	pdtlb(HPPA_SID_KERNEL, dpa);
	pitlb(HPPA_SID_KERNEL, spa);
	pitlb(HPPA_SID_KERNEL, dpa);
#endif
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	UVMHIST_FUNC(__func__);
	if (va != 0) {
		UVMHIST_CALLARGS(maphist, "va %#jx pa %#jx prot %#jx flags %#jx",
		    va, pa, prot, flags);
	}

	volatile pt_entry_t *pde;
	pt_entry_t pte, opte;
	struct vm_page *pg;

	if (!(pde = pmap_pde_get(pmap_kernel()->pm_pdir, va)) &&
	    !(pde = pmap_pde_alloc(pmap_kernel(), va, NULL)))
		panic("pmap_kenter_pa: cannot allocate pde for va=0x%lx", va);
	opte = pmap_pte_get(pde, va);
	pte = pa | PTE_PROT(TLB_WIRED | TLB_REFTRAP |
	    pmap_prot(pmap_kernel(), prot & VM_PROT_ALL));
	if (IS_IOPAGE_P(pa) || (flags & PMAP_NOCACHE))
		pte |= PTE_PROT(TLB_UNCACHEABLE);

	if ((flags & PMAP_DIRECTMAP) == 0) {
		pmap_kernel()->pm_stats.wired_count++;
		pmap_kernel()->pm_stats.resident_count++;
	}
	if (opte)
		pmap_pte_flush(pmap_kernel(), va, opte);

	pg = pmap_initialized ? PHYS_TO_VM_PAGE(PTE_PAGE(pte)) : NULL;
	if (pg != NULL) {
		KASSERT(pa < HPPA_IOBEGIN);

		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		struct pv_entry *pve;

		pve = pmap_pv_alloc();
		if (!pve)
			panic("%s: no pv entries available", __func__);
		UVMHIST_LOG(maphist, "va %#jx pa %#jx pte %#jx TLB_KENTER",
		    va, pa, pte, 0);

		pmap_resolve_alias(pg, pmap_kernel(), va, pte);

		pmap_pv_lock(md);
		pmap_pv_enter(pg, pve, pmap_kernel(), va, NULL, PV_KENTER);
		pmap_pv_unlock(md);
	}
	pmap_pte_set(pde, va, pte);

	if (va != 0) {
		UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);
	}

}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
	UVMHIST_FUNC(__func__);
	bool pzero = false;
	if (va != 0) {
		UVMHIST_CALLARGS(maphist, "va %#jx...%#jx", va, va + size, 0,
		    0);
		pzero = true;
	}

	struct pv_entry *pve;
	vaddr_t eva, pdemask;
	volatile pt_entry_t *pde = NULL;
	pt_entry_t pte;
	struct vm_page *pg;
	pmap_t pmap = pmap_kernel();

#ifdef DEBUG
	/*
	 * Don't allow the VA == PA mappings, apart from page zero, to be
	 * removed. Page zero is given special treatment so that we get TLB
	 * faults when the kernel tries to de-reference NULL or anything else
	 * in the first page when it shouldn't.
	 */
	if (va != 0 && va < ptoa(physmem)) {
		UVMHIST_LOG(maphist, "va %#jx size %#jx: unmapping physmem", va,
		    size, 0, 0);
		return;
	}
#endif

	for (pdemask = 1, eva = va + size; va < eva; va += PAGE_SIZE) {
		if (pdemask != (va & PDE_MASK)) {
			pdemask = va & PDE_MASK;
			if (!(pde = pmap_pde_get(pmap->pm_pdir, va))) {
				va = pdemask + PDE_SIZE - PAGE_SIZE;
				continue;
			}
		}
		if (!(pte = pmap_pte_get(pde, va))) {
			UVMHIST_LOG(maphist, "unmapping unmapped %#jx",
			    va, 0, 0, 0);
			continue;
		}

		pmap_pte_flush(pmap, va, pte);
		pmap_pte_set(pde, va, 0);

		pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;

		pg = pmap_initialized ? PHYS_TO_VM_PAGE(PTE_PAGE(pte)) : NULL;
		if (pg != NULL) {
			struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

			pmap_pv_lock(md);
			pve = pmap_pv_remove(pg, pmap, va);
			pmap_pv_unlock(md);

			if (pve != NULL)
				pmap_pv_free(pve);
		}
	}
	if (pzero) {
		UVMHIST_LOG(maphist, "<--- done", 0, 0, 0, 0);
	}
}

#if defined(USE_HPT)
#if defined(DDB)
/*
 * prints whole va->pa (aka HPT or HVT)
 */
void
pmap_hptdump(void)
{
	struct hpt_entry *hpt, *ehpt;

	hpt = (struct hpt_entry *)pmap_hpt;
	ehpt = (struct hpt_entry *)((int)hpt + pmap_hptsize);
	db_printf("HPT dump %p-%p:\n", hpt, ehpt);
	for (; hpt < ehpt; hpt++)
		if (hpt->hpt_valid) {
			char buf[128];

			snprintb(buf, sizeof(buf), TLB_BITS, hpt->hpt_tlbprot);

			db_printf("hpt@%p: %x{%sv=%x:%x},%s,%x\n",
			    hpt, *(int *)hpt, (hpt->hpt_valid?"ok,":""),
			    hpt->hpt_space, hpt->hpt_vpn << 9,
			    buf, tlbptob(hpt->hpt_tlbpage));
		}
}
#endif
#endif
