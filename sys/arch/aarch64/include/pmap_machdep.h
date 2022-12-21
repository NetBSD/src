/*	$NetBSD: pmap_machdep.h,v 1.2 2022/12/21 11:39:46 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef	_AARCH64_PMAP_MACHDEP_H_
#define	_AARCH64_PMAP_MACHDEP_H_

#include <arm/cpufunc.h>

#define	PMAP_HWPAGEWALKER		1

#define	PMAP_PDETABSIZE	(PAGE_SIZE / sizeof(pd_entry_t))
#define	PMAP_SEGTABSIZE	NSEGPG

#define	PMAP_INVALID_PDETAB_ADDRESS	((pmap_pdetab_t *)(VM_MIN_KERNEL_ADDRESS - PAGE_SIZE))
#define	PMAP_INVALID_SEGTAB_ADDRESS	((pmap_segtab_t *)(VM_MIN_KERNEL_ADDRESS - PAGE_SIZE))

#define	NPTEPG		(PAGE_SIZE / sizeof(pt_entry_t))
#define	NPDEPG		(PAGE_SIZE / sizeof(pd_entry_t))

#define	PTPSHIFT	3
#define	PTPLENGTH	(PGSHIFT - PTPSHIFT)
#define	SEGSHIFT	(PGSHIFT + PTPLENGTH)	/* LOG2(NBSEG) */

#define	NBSEG		(1 << SEGSHIFT)		/* bytes/segment */
#define	SEGOFSET	(NBSEG - 1)		/* byte offset into segment */

#define	SEGLENGTH	(PGSHIFT - 3)

#define	XSEGSHIFT	(SEGSHIFT + SEGLENGTH + SEGLENGTH)
						/* LOG2(NBXSEG) */

#define	NBXSEG		(1UL << XSEGSHIFT)	/* bytes/xsegment */
#define	XSEGOFSET	(NBXSEG - 1)		/* byte offset into xsegment */
#define	XSEGLENGTH	(PGSHIFT - 3)
#define	NXSEGPG		(1 << XSEGLENGTH)
#define	NSEGPG		(1 << SEGLENGTH)


#ifndef __BSD_PTENTRY_T__
#define	__BSD_PTENTRY_T__
#define	PRIxPTE         PRIx64
#endif /* __BSD_PTENTRY_T__ */

#define	KERNEL_PID	0

#define	__HAVE_PMAP_PV_TRACK
#define	__HAVE_PMAP_MD

/* XXX temporary */
#define	__HAVE_UNLOCKED_PMAP

#define	PMAP_PAGE_INIT(pp)				\
do {							\
	(pp)->pp_md.mdpg_first.pv_next = NULL;		\
	(pp)->pp_md.mdpg_first.pv_pmap = NULL;		\
	(pp)->pp_md.mdpg_first.pv_va = 0;		\
	(pp)->pp_md.mdpg_attrs = 0;			\
	VM_PAGEMD_PVLIST_LOCK_INIT(&(pp)->pp_md);	\
} while (/* CONSTCOND */ 0)

struct pmap_md {
	paddr_t			pmd_l0_pa;
};

#define	pm_l0_pa	pm_md.pmd_l0_pa

void pmap_md_pdetab_init(struct pmap *);
void pmap_md_pdetab_fini(struct pmap *);

vaddr_t pmap_md_map_poolpage(paddr_t, size_t);
paddr_t pmap_md_unmap_poolpage(vaddr_t, size_t);
struct vm_page *pmap_md_alloc_poolpage(int);

bool	pmap_md_kernel_vaddr_p(vaddr_t);
paddr_t	pmap_md_kernel_vaddr_to_paddr(vaddr_t);
bool	pmap_md_direct_mapped_vaddr_p(vaddr_t);
paddr_t	pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t);
vaddr_t	pmap_md_direct_map_paddr(paddr_t);
bool	pmap_md_io_vaddr_p(vaddr_t);

void	pmap_md_activate_efirt(void);
void	pmap_md_deactivate_efirt(void);

void	pmap_icache_sync_range(pmap_t, vaddr_t, vaddr_t);

#include <uvm/pmap/vmpagemd.h>
#include <uvm/pmap/pmap.h>
#include <uvm/pmap/pmap_pvt.h>
#include <uvm/pmap/pmap_tlb.h>
#include <uvm/pmap/pmap_synci.h>
#include <uvm/pmap/tlb.h>

#include <uvm/uvm_page.h>

#define	POOL_VTOPHYS(va)	vtophys((vaddr_t)(va))

struct pmap_page {
	struct vm_page_md	pp_md;
};

#define	PMAP_PAGE_TO_MD(ppage)	(&((ppage)->pp_md))

#define	PVLIST_EMPTY_P(pg)	VM_PAGEMD_PVLIST_EMPTY_P(VM_PAGE_TO_MD(pg))

#define	LX_BLKPAG_OS_MODIFIED	LX_BLKPAG_OS_0

#define	PMAP_PTE_OS0	"modified"
#define	PMAP_PTE_OS1	"(unk)"

static inline paddr_t
pmap_l0pa(struct pmap *pm)
{
        return pm->pm_l0_pa;
}

#if defined(__PMAP_PRIVATE)

#include <uvm/uvm_physseg.h>
struct vm_page_md;

void	pmap_md_icache_sync_all(void);
void	pmap_md_icache_sync_range_index(vaddr_t, vsize_t);
void	pmap_md_page_syncicache(struct vm_page_md *, const kcpuset_t *);
bool	pmap_md_vca_add(struct vm_page_md *, vaddr_t, pt_entry_t *);
void	pmap_md_vca_clean(struct vm_page_md *, int);
void	pmap_md_vca_remove(struct vm_page_md *, vaddr_t, bool, bool);
bool	pmap_md_ok_to_steal_p(const uvm_physseg_t, size_t);

void	pmap_md_xtab_activate(pmap_t, struct lwp *);
void	pmap_md_xtab_deactivate(pmap_t);

vaddr_t pmap_md_direct_map_paddr(paddr_t);


#ifdef MULTIPROCESSOR
#define	PMAP_NO_PV_UNCACHED
#endif

static inline void
pmap_md_init(void)
{
	// nothing
}


static inline bool
pmap_md_tlb_check_entry(void *ctx, vaddr_t va, tlb_asid_t asid, pt_entry_t pte)
{
	// TLB not walked and so not called.
	return false;
}


static inline bool
pmap_md_virtual_cache_aliasing_p(void)
{
	return false;
}


static inline vsize_t
pmap_md_cache_prefer_mask(void)
{
	return 0;
}


static inline pt_entry_t *
pmap_md_nptep(pt_entry_t *ptep)
{

	return ptep + 1;
}

#endif	/* __PMAP_PRIVATE */

#ifdef __PMAP_PRIVATE
static __inline paddr_t
pte_to_paddr(pt_entry_t pte)
{

	return l3pte_pa(pte);
}


static inline bool
pte_valid_p(pt_entry_t pte)
{

	return l3pte_valid(pte);
}


static inline void
pmap_md_clean_page(struct vm_page_md *md, bool is_src)
{
}


static inline bool
pte_modified_p(pt_entry_t pte)
{

	return (pte & LX_BLKPAG_OS_MODIFIED) != 0;
}


static inline bool
pte_wired_p(pt_entry_t pte)
{

        return (pte & LX_BLKPAG_OS_WIRED) != 0;
}


static inline pt_entry_t
pte_wire_entry(pt_entry_t pte)
{

        return pte | LX_BLKPAG_OS_WIRED;
}


static inline pt_entry_t
pte_unwire_entry(pt_entry_t pte)
{

        return pte & ~LX_BLKPAG_OS_WIRED;
}


static inline uint64_t
pte_value(pt_entry_t pte)
{

	return pte;
}

static inline bool
pte_cached_p(pt_entry_t pte)
{

	return ((pte & LX_BLKPAG_ATTR_MASK) == LX_BLKPAG_ATTR_NORMAL_WB);
}

static inline bool
pte_deferred_exec_p(pt_entry_t pte)
{

	return false;
}

static inline pt_entry_t
pte_nv_entry(bool kernel_p)
{

	/* Not valid entry */
	return kernel_p ? 0 : 0;
}

static inline pt_entry_t
pte_prot_downgrade(pt_entry_t pte, vm_prot_t prot)
{

	return (pte & ~LX_BLKPAG_AP)
	    | (((prot) & (VM_PROT_READ | VM_PROT_WRITE)) == VM_PROT_READ ? LX_BLKPAG_AP_RO : LX_BLKPAG_AP_RW);
}

static inline pt_entry_t
pte_prot_nowrite(pt_entry_t pte)
{

	return pte & ~LX_BLKPAG_AF;
}

static inline pt_entry_t
pte_cached_change(pt_entry_t pte, bool cached)
{
	pte &= ~LX_BLKPAG_ATTR_MASK;
	pte |= (cached ? LX_BLKPAG_ATTR_NORMAL_WB : LX_BLKPAG_ATTR_NORMAL_NC);

	return pte;
}

static inline void
pte_set(pt_entry_t *ptep, pt_entry_t pte)
{

	*ptep = pte;
	dsb(ishst);
	/*
	 * if this mapping is going to be used by userland then the eret *can* act
	 * as the isb, but might not (apple m1).
	 *
	 * if this mapping is kernel then the isb is always needed (for some micro-
	 * architectures)
	 */

	isb();
}

static inline pd_entry_t
pte_invalid_pde(void)
{

	return 0;
}


static inline pd_entry_t
pte_pde_pdetab(paddr_t pa, bool kernel_p)
{

	return LX_VALID | LX_TYPE_TBL | (kernel_p ? 0 : LX_BLKPAG_NG) | pa;
}


static inline pd_entry_t
pte_pde_ptpage(paddr_t pa, bool kernel_p)
{

	return LX_VALID | LX_TYPE_TBL | (kernel_p ? 0 : LX_BLKPAG_NG) | pa;
}


static inline bool
pte_pde_valid_p(pd_entry_t pde)
{

	return lxpde_valid(pde);
}


static inline paddr_t
pte_pde_to_paddr(pd_entry_t pde)
{

	return lxpde_pa(pde);
}


static inline pd_entry_t
pte_pde_cas(pd_entry_t *pdep, pd_entry_t opde, pt_entry_t npde)
{
#ifdef MULTIPROCESSOR
	opde = atomic_cas_64(pdep, opde, npde);
	dsb(ishst);
#else
	*pdep = npde;
#endif
	return opde;
}


static inline void
pte_pde_set(pd_entry_t *pdep, pd_entry_t npde)
{

	*pdep = npde;
}



static inline pt_entry_t
pte_memattr(u_int flags)
{

	switch (flags & (PMAP_DEV_MASK | PMAP_CACHE_MASK)) {
	case PMAP_DEV_NP ... PMAP_DEV_NP | PMAP_CACHE_MASK:
		/* Device-nGnRnE */
		return LX_BLKPAG_ATTR_DEVICE_MEM_NP;
	case PMAP_DEV ... PMAP_DEV | PMAP_CACHE_MASK:
		/* Device-nGnRE */
		return LX_BLKPAG_ATTR_DEVICE_MEM;
	case PMAP_NOCACHE:
	case PMAP_NOCACHE_OVR:
	case PMAP_WRITE_COMBINE:
		/* only no-cache */
		return LX_BLKPAG_ATTR_NORMAL_NC;
	case PMAP_WRITE_BACK:
	case 0:
	default:
		return LX_BLKPAG_ATTR_NORMAL_WB;
	}
}


static inline pt_entry_t
pte_make_kenter_pa(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
    u_int flags)
{
	KASSERTMSG((pa & ~L3_PAG_OA) == 0, "pa %" PRIxPADDR, pa);

	pt_entry_t pte = pa
	    | LX_VALID
#ifdef MULTIPROCESSOR
	    | LX_BLKPAG_SH_IS
#endif
	    | L3_TYPE_PAG
	    | LX_BLKPAG_AF
	    | LX_BLKPAG_UXN | LX_BLKPAG_PXN
	    | (((prot) & (VM_PROT_READ | VM_PROT_WRITE)) == VM_PROT_READ ? LX_BLKPAG_AP_RO : LX_BLKPAG_AP_RW)
	    | LX_BLKPAG_OS_WIRED;

	if (prot & VM_PROT_EXECUTE)
		pte &= ~LX_BLKPAG_PXN;

	pte &= ~LX_BLKPAG_ATTR_MASK;
	pte |= pte_memattr(flags);

	return pte;
}

static inline pt_entry_t
pte_make_enter_efirt(paddr_t pa, vm_prot_t prot, u_int flags)
{
	KASSERTMSG((pa & ~L3_PAG_OA) == 0, "pa %" PRIxPADDR, pa);

	pt_entry_t npte = pa
	    | LX_VALID
#ifdef MULTIPROCESSOR
	    | LX_BLKPAG_SH_IS
#endif
	    | L3_TYPE_PAG
	    | LX_BLKPAG_AF
	    | LX_BLKPAG_NG /* | LX_BLKPAG_APUSER */
	    | LX_BLKPAG_UXN | LX_BLKPAG_PXN
	    | (((prot) & (VM_PROT_READ | VM_PROT_WRITE)) == VM_PROT_READ ? LX_BLKPAG_AP_RO : LX_BLKPAG_AP_RW);

	if (prot & VM_PROT_EXECUTE)
		npte &= ~LX_BLKPAG_PXN;

	npte &= ~LX_BLKPAG_ATTR_MASK;
	npte |= pte_memattr(flags);

	return npte;
}

static inline pt_entry_t
pte_make_enter(paddr_t pa, const struct vm_page_md *mdpg, vm_prot_t prot,
    u_int flags, bool is_kernel_pmap_p)
{
	KASSERTMSG((pa & ~L3_PAG_OA) == 0, "pa %" PRIxPADDR, pa);

	pt_entry_t npte = pa
	    | LX_VALID
#ifdef MULTIPROCESSOR
	    | LX_BLKPAG_SH_IS
#endif
	    | L3_TYPE_PAG
	    | LX_BLKPAG_UXN | LX_BLKPAG_PXN
	    | (((prot) & (VM_PROT_READ | VM_PROT_WRITE)) == VM_PROT_READ ? LX_BLKPAG_AP_RO : LX_BLKPAG_AP_RW);

	if ((prot & VM_PROT_WRITE) != 0 &&
	    ((flags & VM_PROT_WRITE) != 0 || VM_PAGEMD_MODIFIED_P(mdpg))) {
		/*
		 * This is a writable mapping, and the page's mod state
		 * indicates it has already been modified.  No need for
		 * modified emulation.
		 */
		npte |= LX_BLKPAG_AF;
	} else if ((flags & VM_PROT_ALL) || VM_PAGEMD_REFERENCED_P(mdpg)) {
		/*
		 * - The access type indicates that we don't need to do
		 *   referenced emulation.
		 * OR
		 * - The physical page has already been referenced so no need
		 *   to re-do referenced emulation here.
		 */
		npte |= LX_BLKPAG_AF;
	}

	if (prot & VM_PROT_EXECUTE)
		npte &= (is_kernel_pmap_p ? ~LX_BLKPAG_PXN : ~LX_BLKPAG_UXN);

	npte &= ~LX_BLKPAG_ATTR_MASK;
	npte |= pte_memattr(flags);

	/*
	 * Make sure userland mappings get the right permissions
	 */
	if (!is_kernel_pmap_p) {
		npte |= LX_BLKPAG_NG | LX_BLKPAG_APUSER;
	}

	return npte;
}
#endif /* __PMAP_PRIVATE */

#endif	/* _AARCH64_PMAP_MACHDEP_H_ */

