/*	$NetBSD: pmap.c,v 1.1.4.1 2011/06/06 09:07:14 jruoho Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.1.4.1 2011/06/06 09:07:14 jruoho Exp $");

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

#include "opt_sysv.h"
#include "opt_multiprocessor.h"

#define __PMAP_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/pool.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#include <sys/socketvar.h>	/* XXX: for sock_loan_thresh */

#include <uvm/uvm.h>

#define	PMAP_COUNT(name)	(pmap_evcnt_##name.ev_count++ + 0)
#define PMAP_COUNTER(name, desc) \
static struct evcnt pmap_evcnt_##name = \
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", desc); \
EVCNT_ATTACH_STATIC(pmap_evcnt_##name)

PMAP_COUNTER(remove_kernel_calls, "remove kernel calls");
PMAP_COUNTER(remove_kernel_pages, "kernel pages unmapped");
PMAP_COUNTER(remove_user_calls, "remove user calls");
PMAP_COUNTER(remove_user_pages, "user pages unmapped");
PMAP_COUNTER(remove_flushes, "remove cache flushes");
PMAP_COUNTER(remove_tlb_ops, "remove tlb ops");
PMAP_COUNTER(remove_pvfirst, "remove pv first");
PMAP_COUNTER(remove_pvsearch, "remove pv search");

PMAP_COUNTER(prefer_requests, "prefer requests");
PMAP_COUNTER(prefer_adjustments, "prefer adjustments");

PMAP_COUNTER(idlezeroed_pages, "pages idle zeroed");
PMAP_COUNTER(zeroed_pages, "pages zeroed");
PMAP_COUNTER(copied_pages, "pages copied");

PMAP_COUNTER(kenter_pa, "kernel fast mapped pages");
PMAP_COUNTER(kenter_pa_bad, "kernel fast mapped pages (bad color)");
PMAP_COUNTER(kenter_pa_unmanaged, "kernel fast mapped unmanaged pages");
PMAP_COUNTER(kremove_pages, "kernel fast unmapped pages");

PMAP_COUNTER(page_cache_evictions, "pages changed to uncacheable");
PMAP_COUNTER(page_cache_restorations, "pages changed to cacheable");

PMAP_COUNTER(kernel_mappings_bad, "kernel pages mapped (bad color)");
PMAP_COUNTER(user_mappings_bad, "user pages mapped (bad color)");
PMAP_COUNTER(kernel_mappings, "kernel pages mapped");
PMAP_COUNTER(user_mappings, "user pages mapped");
PMAP_COUNTER(user_mappings_changed, "user mapping changed");
PMAP_COUNTER(kernel_mappings_changed, "kernel mapping changed");
PMAP_COUNTER(uncached_mappings, "uncached pages mapped");
PMAP_COUNTER(unmanaged_mappings, "unmanaged pages mapped");
PMAP_COUNTER(managed_mappings, "managed pages mapped");
PMAP_COUNTER(mappings, "pages mapped");
PMAP_COUNTER(remappings, "pages remapped");
PMAP_COUNTER(unmappings, "pages unmapped");
PMAP_COUNTER(primary_mappings, "page initial mappings");
PMAP_COUNTER(primary_unmappings, "page final unmappings");
PMAP_COUNTER(tlb_hit, "page mapping");

PMAP_COUNTER(exec_mappings, "exec pages mapped");
PMAP_COUNTER(exec_synced_mappings, "exec pages synced");
PMAP_COUNTER(exec_synced_remove, "exec pages synced (PR)");
PMAP_COUNTER(exec_synced_clear_modify, "exec pages synced (CM)");
PMAP_COUNTER(exec_synced_page_protect, "exec pages synced (PP)");
PMAP_COUNTER(exec_synced_protect, "exec pages synced (P)");
PMAP_COUNTER(exec_uncached_page_protect, "exec pages uncached (PP)");
PMAP_COUNTER(exec_uncached_clear_modify, "exec pages uncached (CM)");
PMAP_COUNTER(exec_uncached_zero_page, "exec pages uncached (ZP)");
PMAP_COUNTER(exec_uncached_copy_page, "exec pages uncached (CP)");
PMAP_COUNTER(exec_uncached_remove, "exec pages uncached (PR)");

PMAP_COUNTER(create, "creates");
PMAP_COUNTER(reference, "references");
PMAP_COUNTER(dereference, "dereferences");
PMAP_COUNTER(destroy, "destroyed");
PMAP_COUNTER(activate, "activations");
PMAP_COUNTER(deactivate, "deactivations");
PMAP_COUNTER(update, "updates");
#ifdef MULTIPROCESSOR
PMAP_COUNTER(shootdown_ipis, "shootdown IPIs");
#endif
PMAP_COUNTER(unwire, "unwires");
PMAP_COUNTER(copy, "copies");
PMAP_COUNTER(clear_modify, "clear_modifies");
PMAP_COUNTER(protect, "protects");
PMAP_COUNTER(page_protect, "page_protects");

#define PDB_FOLLOW	0x0001
#define PDB_INIT	0x0002
#define PDB_ENTER	0x0004
#define PDB_REMOVE	0x0008
#define PDB_CREATE	0x0010
#define PDB_PTPAGE	0x0020
#define PDB_PVENTRY	0x0040
#define PDB_BITS	0x0080
#define PDB_PROTECT	0x0200
#define PDB_TLBPID	0x0400
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000
int pmapdebug = 0;

#define PMAP_ASID_RESERVED 0
CTASSERT(PMAP_ASID_RESERVED == 0);

/*
 * Initialize the kernel pmap.
 */
#ifdef MULTIPROCESSOR
#define	PMAP_SIZE	offsetof(struct pmap, pm_pai[MAXCPUS])
#else
#define	PMAP_SIZE	sizeof(struct pmap)
kmutex_t pmap_pvlist_mutex __aligned(COHERENCY_UNIT);
#endif

struct pmap_kernel kernel_pmap_store = {
	.kernel_pmap = {
		.pm_count = 1,
		.pm_segtab = PMAP_INVALID_SEGTAB_ADDRESS,
		.pm_minaddr = VM_MIN_KERNEL_ADDRESS,
		.pm_maxaddr = VM_MAX_KERNEL_ADDRESS,
#ifdef MULTIPROCESSOR
		.pm_active = 1,
		.pm_onproc = 1,
#endif
	},
};

struct pmap * const kernel_pmap_ptr = &kernel_pmap_store.kernel_pmap;

struct pmap_limits pmap_limits;

#ifdef PMAP_POOLPAGE_DEBUG
struct poolpage_info {
	vaddr_t base;
	vaddr_t size;
	vaddr_t hint;
	pt_entry_t *sysmap;
} poolpage;
#endif

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
struct pool pmap_pmap_pool;
struct pool pmap_pv_pool;

#ifndef PMAP_PV_LOWAT
#define	PMAP_PV_LOWAT	16
#endif
int		pmap_pv_lowat = PMAP_PV_LOWAT;

bool		pmap_initialized = false;
#define	PMAP_PAGE_COLOROK_P(a, b) \
		((((int)(a) ^ (int)(b)) & pmap_page_colormask) == 0)
u_int		pmap_page_colormask;

#define PAGE_IS_MANAGED(pa)	\
	(pmap_initialized == true && vm_physseg_find(atop(pa), NULL) != -1)

#define PMAP_IS_ACTIVE(pm)						\
	((pm) == pmap_kernel() || 					\
	 (pm) == curlwp->l_proc->p_vmspace->vm_map.pmap)

/* Forward function declarations */
void pmap_remove_pv(pmap_t, vaddr_t, struct vm_page *, bool);
void pmap_enter_pv(pmap_t, vaddr_t, struct vm_page *, u_int *);

/*
 * PV table management functions.
 */
void	*pmap_pv_page_alloc(struct pool *, int);
void	pmap_pv_page_free(struct pool *, void *);

struct pool_allocator pmap_pv_page_allocator = {
	pmap_pv_page_alloc, pmap_pv_page_free, 0,
};

#define	pmap_pv_alloc()		pool_get(&pmap_pv_pool, PR_NOWAIT)
#define	pmap_pv_free(pv)	pool_put(&pmap_pv_pool, (pv))

/*
 * Misc. functions.
 */

bool
pmap_page_clear_attributes(struct vm_page *pg, u_int clear_attributes)
{
	volatile u_int * const attrp = &VM_PAGE_TO_MD(pg)->mdpg_attrs;
#ifdef MULTIPROCESSOR
	for (;;) {
		u_int old_attr = *attrp;
		if ((old_attr & clear_attributes) == 0)
			return false;
		u_int new_attr = old_attr & ~clear_attributes;
		if (old_attr == atomic_cas_uint(attrp, old_attr, new_attr))
			return true;
	}
#else
	u_int old_attr = *attrp;
	if ((old_attr & clear_attributes) == 0)
		return false;
	*attrp &= ~clear_attributes;
	return true;
#endif
}

void
pmap_page_set_attributes(struct vm_page *pg, u_int set_attributes)
{
#ifdef MULTIPROCESSOR
	atomic_or_uint(&VM_PAGE_TO_MD(pg)->mdpg_attrs, set_attributes);
#else
	VM_PAGE_TO_MD(pg)->mdpg_attrs |= set_attributes;
#endif
}

static void
pmap_page_syncicache(struct vm_page *pg)
{
#ifndef MULTIPROCESSOR
	struct pmap * const curpmap = curcpu()->ci_curpm;
#endif
	pv_entry_t pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
	__cpuset_t onproc = CPUSET_NULLSET;
	(void)VM_PAGE_PVLIST_LOCK(pg, false);
	if (pv->pv_pmap != NULL) {
		for (; pv != NULL; pv = pv->pv_next) {
#ifdef MULTIPROCESSOR
			CPUSET_MERGE(onproc, pv->pv_pmap->pm_onproc);
			if (CPUSET_EQUAL_P(onproc, cpus_running)) {
				break;
			}
#else
			if (pv->pv_pmap == curpmap) {
				onproc = CPUSET_SINGLE(0);
				break;
			}
#endif
		}
	}
	VM_PAGE_PVLIST_UNLOCK(pg);
	kpreempt_disable();
	pmap_md_page_syncicache(pg, onproc);
	kpreempt_enable();
}

/*
 * Define the initial bounds of the kernel virtual address space.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{

	*vstartp = VM_MIN_KERNEL_ADDRESS;	/* kernel is in K0SEG */
	*vendp = trunc_page(pmap_limits.virtual_end);	/* XXX need pmap_growkernel() */
}

/*
 * Bootstrap memory allocator (alternative to vm_bootstrap_steal_memory()).
 * This function allows for early dynamic memory allocation until the virtual
 * memory system has been bootstrapped.  After that point, either kmem_alloc
 * or malloc should be used.  This function works by stealing pages from the
 * (to be) managed page pool, then implicitly mapping the pages (by using
 * their k0seg addresses) and zeroing them.
 *
 * It may be used once the physical memory segments have been pre-loaded
 * into the vm_physmem[] array.  Early memory allocation MUST use this
 * interface!  This cannot be used after vm_page_startup(), and will
 * generate a panic if tried.
 *
 * Note that this memory will never be freed, and in essence it is wired
 * down.
 *
 * We must adjust *vstartp and/or *vendp iff we use address space
 * from the kernel virtual address range defined by pmap_virtual_space().
 */
vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	u_int npgs;
	paddr_t pa;
	vaddr_t va;

	size = round_page(size);
	npgs = atop(size);

	for (u_int bank = 0; bank < vm_nphysseg; bank++) {
		struct vm_physseg * const seg = VM_PHYSMEM_PTR(bank);
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

		if (seg->avail_start != seg->start ||
		    seg->avail_start >= seg->avail_end)
			continue;

		if ((seg->avail_end - seg->avail_start) < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(seg->avail_start);
		seg->avail_start += npgs;
		seg->start += npgs;

		/*
		 * Have we used up this segment?
		 */
		if (seg->avail_start == seg->end) {
			if (vm_nphysseg == 1)
				panic("pmap_steal_memory: out of memory!");

			/* Remove this segment from the list. */
			vm_nphysseg--;
			if (bank < vm_nphysseg)
				memmove(seg, seg+1,
				    sizeof(*seg) * (vm_nphysseg - bank));
		}

		va = pmap_md_direct_map_paddr(pa);
		memset((void *)va, 0, size);
		return va;
	}

	/*
	 * If we got here, there was no memory left.
	 */
	panic("pmap_steal_memory: no memory to steal");
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(void)
{
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_INIT))
		printf("pmap_init()\n");
#endif

	/*
	 * Set a low water mark on the pv_entry pool, so that we are
	 * more likely to have these around even in extreme memory
	 * starvation.
	 */
	pool_setlowat(&pmap_pv_pool, pmap_pv_lowat);

	pmap_md_init();

	/*
	 * Now it is safe to enable pv entry recording.
	 */
	pmap_initialized = true;
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
pmap_t
pmap_create(void)
{
	pmap_t pmap;

	PMAP_COUNT(create);
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, PMAP_SIZE);

	KASSERT(pmap->pm_pai[0].pai_link.le_prev == NULL);

	pmap->pm_count = 1;
	pmap->pm_minaddr = VM_MIN_ADDRESS;
	pmap->pm_maxaddr = VM_MAXUSER_ADDRESS;

	pmap_segtab_alloc(pmap);

	return pmap;
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_destroy(pmap_t pmap)
{
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_destroy(%p)\n", pmap);
#endif
	if (atomic_dec_uint_nv(&pmap->pm_count) > 0) {
		PMAP_COUNT(dereference);
		return;
	}

	KASSERT(pmap->pm_count == 0);
	PMAP_COUNT(destroy);
	kpreempt_disable();
	pmap_tlb_asid_release_all(pmap);
	pmap_segtab_free(pmap);

	pool_put(&pmap_pmap_pool, pmap);
	kpreempt_enable();
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_reference(%p)\n", pmap);
#endif
	if (pmap != NULL) {
		atomic_inc_uint(&pmap->pm_count);
	}
	PMAP_COUNT(reference);
}

/*
 *	Make a new pmap (vmspace) active for the given process.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	PMAP_COUNT(activate);

	kpreempt_disable();
	pmap_tlb_asid_acquire(pmap, l);
	if (l == curlwp) {
		pmap_segtab_activate(pmap, l);
	}
	kpreempt_enable();
}

/*
 *	Make a previously active pmap (vmspace) inactive.
 */
void
pmap_deactivate(struct lwp *l)
{
	PMAP_COUNT(deactivate);

	kpreempt_disable();
	curcpu()->ci_pmap_user_segtab = PMAP_INVALID_SEGTAB_ADDRESS;
	pmap_tlb_asid_deactivate(l->l_proc->p_vmspace->vm_map.pmap);
	kpreempt_enable();
}

void
pmap_update(struct pmap *pm)
{
	PMAP_COUNT(update);

	kpreempt_disable();
#ifdef MULTIPROCESSOR
	u_int pending = atomic_swap_uint(&pm->pm_shootdown_pending, 0);
	if (pending && pmap_tlb_shootdown_bystanders(pm))
		PMAP_COUNT(shootdown_ipis);
#endif
#ifdef DEBUG
	pmap_tlb_check(pm);
#endif /* DEBUG */

	/*
	 * If pmap_remove_all was called, we deactivated ourselves and nuked
	 * our ASID.  Now we have to reactivate ourselves.
	 */
	if (__predict_false(pm->pm_flags & PMAP_DEFERRED_ACTIVATE)) {
		pm->pm_flags ^= PMAP_DEFERRED_ACTIVATE;
		pmap_tlb_asid_acquire(pm, curlwp);
		pmap_segtab_activate(pm, curlwp);
	}
	kpreempt_enable();
}

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */

static bool
pmap_pte_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *ptep,
	uintptr_t flags)
{
	const pt_entry_t npte = flags;
	const bool is_kernel_pmap_p = (pmap == pmap_kernel());

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT)) {
		printf("%s: %p, %"PRIxVADDR", %"PRIxVADDR", %p, %"PRIxPTR"\n",
		   __func__, pmap, sva, eva, ptep, flags);
	}
#endif
	KASSERT(kpreempt_disabled());

	for (; sva < eva; sva += NBPG, ptep++) {
		pt_entry_t pt_entry = *ptep;
		if (!pte_valid_p(pt_entry))
			continue;
		if (is_kernel_pmap_p)
			PMAP_COUNT(remove_kernel_calls);
		else
			PMAP_COUNT(remove_user_pages);
		if (pte_wired_p(pt_entry))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;
		struct vm_page *pg = PHYS_TO_VM_PAGE(pte_to_paddr(pt_entry));
		if (__predict_true(pg != NULL)) {
			pmap_remove_pv(pmap, sva, pg,
			   pte_modified_p(pt_entry));
		}
		*ptep = npte;
		/*
		 * Flush the TLB for the given address.
		 */
		pmap_tlb_invalidate_addr(pmap, sva);
	}
	return false;
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	const bool is_kernel_pmap_p = (pmap == pmap_kernel());
	const pt_entry_t npte = pte_nv_entry(is_kernel_pmap_p);

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %#"PRIxVADDR", %#"PRIxVADDR")\n", pmap, sva, eva);
#endif

	if (is_kernel_pmap_p)
		PMAP_COUNT(remove_kernel_calls);
	else
		PMAP_COUNT(remove_user_calls);
#ifdef PARANOIADIAG
	if (sva < pm->pm_minaddr || eva > pm->pm_maxaddr)
		panic("%s: va range %#"PRIxVADDR"-%#"PRIxVADDR" not in range",
		    __func__, sva, eva - 1);
	if (PMAP_IS_ACTIVE(pmap)) {
		struct pmap_asid_info * const pai = PMAP_PAI(pmap, curcpu());
		uint32_t asid = tlb_get_asid();
		if (asid != pai->pai_asid) {
			panic("%s: inconsistency for active TLB flush"
			    ": %d <-> %d", __func__, asid, pai->pai_asid);
		}
	}
#endif
	kpreempt_disable();
	pmap_pte_process(pmap, sva, eva, pmap_pte_remove, npte);
	kpreempt_enable();
}

/*
 *	pmap_page_protect:
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	pv_entry_t pv;
	vaddr_t va;

	PMAP_COUNT(page_protect);
#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%#"PRIxPADDR", %x)\n",
		    VM_PAGE_TO_PHYS(pg), prot);
#endif
	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
	case VM_PROT_ALL:
		break;

	/* copy_on_write */
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		(void)VM_PAGE_PVLIST_LOCK(pg, false);
		pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
		/*
		 * Loop over all current mappings setting/clearing as apropos.
		 */
		if (pv->pv_pmap != NULL) {
			while (pv != NULL) {
				const pmap_t pmap = pv->pv_pmap;
				const uint16_t gen = VM_PAGE_PVLIST_GEN(pg);
				va = pv->pv_va;
				VM_PAGE_PVLIST_UNLOCK(pg);
				pmap_protect(pmap, va, va + PAGE_SIZE, prot);
				KASSERT(pv->pv_pmap == pmap);
				pmap_update(pmap);
				if (gen != VM_PAGE_PVLIST_LOCK(pg, false)) {
					pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
				} else {
					pv = pv->pv_next;
				}
			}
		}
		VM_PAGE_PVLIST_UNLOCK(pg);
		break;

	/* remove_all */
	default:
		/*
		 * Do this first so that for each unmapping, pmap_remove_pv
		 * won't try to sync the icache.
		 */
		if (pmap_page_clear_attributes(pg, VM_PAGE_MD_EXECPAGE)) {
			PMAP_COUNT(exec_uncached_page_protect);
		}
		(void)VM_PAGE_PVLIST_LOCK(pg, false);
		pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
		while (pv->pv_pmap != NULL) {
			const pmap_t pmap = pv->pv_pmap;
			va = pv->pv_va;
			VM_PAGE_PVLIST_UNLOCK(pg);
			pmap_remove(pmap, va, va + PAGE_SIZE);
			pmap_update(pmap);
			(void)VM_PAGE_PVLIST_LOCK(pg, false);
		}
		VM_PAGE_PVLIST_UNLOCK(pg);
	}
}

static bool
pmap_pte_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *ptep,
	uintptr_t flags)
{
	const vm_prot_t prot = (flags & VM_PROT_ALL);

	KASSERT(kpreempt_disabled());
	/*
	 * Change protection on every valid mapping within this segment.
	 */
	for (; sva < eva; sva += NBPG, ptep++) {
		pt_entry_t pt_entry = *ptep;
		if (!pte_valid_p(pt_entry))
			continue;
		struct vm_page * const pg =
		    PHYS_TO_VM_PAGE(pte_to_paddr(pt_entry));
		if (pg != NULL && pte_modified_p(pt_entry)) {
			pmap_md_vca_clean(pg, sva, PMAP_WBINV);
			if (VM_PAGE_MD_EXECPAGE_P(pg)) {
				KASSERT(VM_PAGE_TO_MD(pg)->mdpg_first.pv_pmap != NULL);
				if (pte_cached_p(pt_entry)) {
					pmap_page_syncicache(pg);
					PMAP_COUNT(exec_synced_protect);
				}
			}
		}
		pt_entry = pte_prot_downgrade(pt_entry, prot);
		if (*ptep != pt_entry) {
			*ptep = pt_entry;
			/*
			 * Update the TLB if needed.
			 */
			pmap_tlb_update_addr(pmap, sva, pt_entry,
			    PMAP_TLB_NEED_IPI);
		}
	}
	return false;
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{

	PMAP_COUNT(protect);
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %#"PRIxVADDR", %#"PRIxVADDR", %x)\n",
		    pmap, sva, eva, prot);
#endif
	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

#ifdef PARANOIADIAG
	if (sva < pm->pm_minaddr || eva > pm->pm_maxaddr)
		panic("%s: va range %#"PRIxVADDR"-%#"PRIxVADDR" not in range",
		    __func__, sva, eva - 1);
	if (PMAP_IS_ACTIVE(pmap)) {
		struct pmap_asid_info * const pai = PMAP_PAI(pmap, curcpu());
		uint32_t asid = tlb_get_asid();
		if (asid != pai->pai_asid) {
			panic("%s: inconsistency for active TLB update"
			    ": %d <-> %d", __func__, asid, pai->pai_asid);
		}
	}
#endif

	/*
	 * Change protection on every valid mapping within this segment.
	 */
	kpreempt_disable();
	pmap_pte_process(pmap, sva, eva, pmap_pte_protect, prot);
	kpreempt_enable();
}

#if defined(VM_PAGE_MD_CACHED)
/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a managed page to cached/uncached.
 */
static void
pmap_page_cache(struct vm_page *pg, bool cached)
{
	KASSERT(kpreempt_disabled());
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_page_uncache(%#"PRIxPADDR")\n", VM_PAGE_TO_PHYS(pg));
#endif
	
	if (cached) {
		pmap_page_clear_attributes(pg, VM_PAGE_MD_UNCACHED);
		PMAP_COUNT(page_cache_restorations);
	} else {
		pmap_page_set_attributes(pg, VM_PAGE_MD_UNCACHED);
		PMAP_COUNT(page_cache_evictions);
	}

	KASSERT(VM_PAGE_PVLIST_LOCKED_P(pg));
	KASSERT(kpreempt_disabled());
	for (pv_entry_t pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
	     pv != NULL;
	     pv = pv->pv_next) {
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = pv->pv_va;

		KASSERT(pmap != NULL);
		KASSERT(pmap != pmap_kernel() || !pmap_md_direct_mapped_vaddr_p(va));
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		if (ptep == NULL)
			continue;
		pt_entry_t pt_entry = *ptep;
		if (pte_valid_p(pt_entry)) {
			pt_entry = pte_cached_change(pt_entry, cached);
			*ptep = pt_entry;
			pmap_tlb_update_addr(pmap, va, pt_entry,
			    PMAP_TLB_NEED_IPI);
		}
	}
}
#endif	/* VM_PAGE_MD_CACHED */

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pt_entry_t npte;
	const bool wired = (flags & PMAP_WIRED) != 0;
	const bool is_kernel_pmap_p = (pmap == pmap_kernel());

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %#"PRIxVADDR", %#"PRIxPADDR", %x, %x)\n",
		    pmap, va, pa, prot, wired);
#endif
	const bool good_color = PMAP_PAGE_COLOROK_P(pa, va);
	if (is_kernel_pmap_p) {
		PMAP_COUNT(kernel_mappings);
		if (!good_color)
			PMAP_COUNT(kernel_mappings_bad);
	} else {
		PMAP_COUNT(user_mappings);
		if (!good_color)
			PMAP_COUNT(user_mappings_bad);
	}
#if defined(DEBUG) || defined(DIAGNOSTIC) || defined(PARANOIADIAG)
	if (va < pmap->pm_minaddr || va >= pmap->pm_maxaddr)
		panic("%s: %s %#"PRIxVADDR" too big",
		    __func__, is_kernel_pmap_p ? "kva" : "uva", va);
#endif

	if (!(prot & VM_PROT_READ))
		panic("%s: no READ in prot %u ", __func__, prot);

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);

	if (pg) {
		/* Set page referenced/modified status based on flags */
		if (flags & VM_PROT_WRITE)
			pmap_page_set_attributes(pg, VM_PAGE_MD_MODIFIED|VM_PAGE_MD_REFERENCED);
		else if (flags & VM_PROT_ALL)
			pmap_page_set_attributes(pg, VM_PAGE_MD_REFERENCED);

#ifdef VM_PAGE_MD_CACHED
		if (!VM_PAGE_MD_CACHED(pg))
			flags |= PMAP_NOCACHE;
#endif

		PMAP_COUNT(managed_mappings);
	} else {
		/*
		 * Assumption: if it is not part of our managed memory
		 * then it must be device memory which may be volatile.
		 */
		flags |= PMAP_NOCACHE;
		PMAP_COUNT(unmanaged_mappings);
	}

	npte = pte_make_enter(pa, pg, prot, flags, is_kernel_pmap_p);

	kpreempt_disable();
	pt_entry_t * const ptep = pmap_pte_reserve(pmap, va, flags);
	if (__predict_false(ptep == NULL)) {
		kpreempt_enable();
		return ENOMEM;
	}
	pt_entry_t opte = *ptep;

	/* Done after case that may sleep/return. */
	if (pg)
		pmap_enter_pv(pmap, va, pg, &npte);

	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * MIPS pages in a MACH page.
	 */
	if (wired) {
		pmap->pm_stats.wired_count++;
		npte |= pte_wired_entry();
	}
#if defined(DEBUG)
	if (pmapdebug & PDB_ENTER) {
		printf("pmap_enter: %p: %#"PRIxVADDR": new pte %#x (pa %#"PRIxPADDR")", pmap, va, npte, pa);
		printf("\n");
	}
#endif

	if (pte_valid_p(opte) && pte_to_paddr(opte) != pa) {
		pmap_remove(pmap, va, va + NBPG);
		PMAP_COUNT(user_mappings_changed);
	}

	KASSERT(pte_valid_p(npte));
	bool resident = pte_valid_p(opte);
	if (!resident)
		pmap->pm_stats.resident_count++;
	*ptep = npte;

	pmap_tlb_update_addr(pmap, va, npte,
	    ((flags & VM_PROT_ALL) ? PMAP_TLB_INSERT : 0)
	    | (resident ? PMAP_TLB_NEED_IPI : 0));
	kpreempt_enable();

	if (pg != NULL && (prot == (VM_PROT_READ | VM_PROT_EXECUTE))) {
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: flush I cache va %#"PRIxVADDR" (%#"PRIxPADDR")\n",
			    va, pa);
#endif
		PMAP_COUNT(exec_mappings);
		if ((flags & VM_PROT_EXECUTE)
		    && !pte_deferred_exec_p(npte)
		    && !VM_PAGE_MD_EXECPAGE_P(pg)
		    && pte_cached_p(npte)) {
			pmap_page_syncicache(pg);
			pmap_page_set_attributes(pg, VM_PAGE_MD_EXECPAGE);
			PMAP_COUNT(exec_synced_mappings);
		}
	}

	return 0;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pa(%#"PRIxVADDR", %#"PRIxPADDR", %x, %x)\n", va, pa, prot, flags);
#endif
	PMAP_COUNT(kenter_pa);
	if (!PMAP_PAGE_COLOROK_P(pa, va) && pg != NULL)
		PMAP_COUNT(kenter_pa_bad);

	if (pg == NULL) {
		PMAP_COUNT(kenter_pa_unmanaged);
		flags |= PMAP_NOCACHE;
	}

	const pt_entry_t npte = pte_make_kenter_pa(pa, pg, prot, flags)
	    | pte_wired_entry();
	kpreempt_disable();
	pt_entry_t * const ptep = pmap_pte_reserve(pmap_kernel(), va, 0);
	KASSERT(ptep != NULL);
	KASSERT(!pte_valid_p(*ptep));
	*ptep = npte;
	/*
	 * We have the option to force this mapping into the TLB but we
	 * don't.  Instead let the next reference to the page do it.
	 */
	pmap_tlb_update_addr(pmap_kernel(), va, npte, 0);
	kpreempt_enable();
#if DEBUG > 1
	for (u_int i = 0; i < PAGE_SIZE / sizeof(long); i++) {
		if (((long *)va)[i] != ((long *)pa)[i])
			panic("%s: contents (%lx) of va %#"PRIxVADDR
			    " != contents (%lx) of pa %#"PRIxPADDR, __func__,
			    ((long *)va)[i], va, ((long *)pa)[i], pa);
	}
#endif
}

static bool
pmap_pte_kremove(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *ptep,
	uintptr_t flags)
{
	const pt_entry_t new_pt_entry = pte_nv_entry(true);

	KASSERT(kpreempt_disabled());

	/*
	 * Set every pt on every valid mapping within this segment.
	 */
	for (; sva < eva; sva += NBPG, ptep++) {
		pt_entry_t pt_entry = *ptep;
		if (!pte_valid_p(pt_entry)) {
			continue;
		}

		PMAP_COUNT(kremove_pages);
		struct vm_page * const pg =
		    PHYS_TO_VM_PAGE(pte_to_paddr(pt_entry));
		if (pg != NULL)
			pmap_md_vca_clean(pg, sva, PMAP_WBINV);

		*ptep = new_pt_entry;
		pmap_tlb_invalidate_addr(pmap_kernel(), sva);
	}

	return false;
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	const vaddr_t sva = trunc_page(va);
	const vaddr_t eva = round_page(va + len);

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE))
		printf("pmap_kremove(%#"PRIxVADDR", %#"PRIxVSIZE")\n", va, len);
#endif

	kpreempt_disable();
	pmap_pte_process(pmap_kernel(), sva, eva, pmap_pte_kremove, 0);
	kpreempt_enable();
}

void
pmap_remove_all(struct pmap *pmap)
{
	KASSERT(pmap != pmap_kernel());

	kpreempt_disable();
	/*
	 * Free all of our ASIDs which means we can skip doing all the
	 * tlb_invalidate_addrs().
	 */
	pmap_tlb_asid_deactivate(pmap);
	pmap_tlb_asid_release_all(pmap);
	pmap->pm_flags |= PMAP_DEFERRED_ACTIVATE;

	kpreempt_enable();
}

/*
 *	Routine:	pmap_unwire
 *	Function:	Clear the wired attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{

	PMAP_COUNT(unwire);
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_WIRING))
		printf("pmap_unwire(%p, %#"PRIxVADDR")\n", pmap, va);
#endif
	/*
	 * Don't need to flush the TLB since PG_WIRED is only in software.
	 */
#ifdef PARANOIADIAG
		if (va < pmap->pm_minaddr || pmap->pm_maxaddr <= va)
			panic("pmap_unwire");
#endif
	kpreempt_disable();
	pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
	pt_entry_t pt_entry = *ptep;
#ifdef DIAGNOSTIC
	if (ptep == NULL)
		panic("%s: pmap %p va %#"PRIxVADDR" invalid STE",
		    __func__, pmap, va);
#endif

#ifdef DIAGNOSTIC
	if (!pte_valid_p(pt_entry))
		panic("pmap_unwire: pmap %p va %#"PRIxVADDR" invalid PTE",
		    pmap, va);
#endif

	if (pte_wired_p(pt_entry)) {
		*ptep &= ~pte_wired_entry();
		pmap->pm_stats.wired_count--;
	}
#ifdef DIAGNOSTIC
	else {
		printf("%s: wiring for pmap %p va %#"PRIxVADDR" unchanged!\n",
		    __func__, pmap, va);
	}
#endif
	kpreempt_enable();
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */
bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	paddr_t pa;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, %#"PRIxVADDR") -> ", pmap, va);
#endif
	if (pmap == pmap_kernel()) {
		if (pmap_md_direct_mapped_vaddr_p(va)) {
			pa = pmap_md_direct_mapped_vaddr_to_paddr(va);
			goto done;
		}
		if (pmap_md_io_vaddr_p(va))
			panic("pmap_extract: io address %#"PRIxVADDR"", va);
	}
	kpreempt_disable();
	pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
	if (ptep == NULL) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("not in segmap\n");
#endif
		kpreempt_enable();
		return false;
	}
	if (!pte_valid_p(*ptep)) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("PTE not valid\n");
#endif
		kpreempt_enable();
		return false;
	}
	pa = pte_to_paddr(*ptep) | (va & PGOFSET);
	kpreempt_enable();
done:
	if (pap != NULL) {
		*pap = pa;
	}
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pa %#"PRIxPADDR"\n", pa);
#endif
	return true;
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_addr, vsize_t len,
    vaddr_t src_addr)
{

	PMAP_COUNT(copy);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%p, %p, %#"PRIxVADDR", %#"PRIxVSIZE", %#"PRIxVADDR")\n",
		    dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%#"PRIxPADDR")\n",
		    VM_PAGE_TO_PHYS(pg));
#endif
	return pmap_page_clear_attributes(pg, VM_PAGE_MD_REFERENCED);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */
bool
pmap_is_referenced(struct vm_page *pg)
{

	return VM_PAGE_MD_REFERENCED_P(pg);
}

/*
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	pv_entry_t pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
	pv_entry_t pv_next;
	uint16_t gen;

	PMAP_COUNT(clear_modify);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%#"PRIxPADDR")\n", VM_PAGE_TO_PHYS(pg));
#endif
	if (VM_PAGE_MD_EXECPAGE_P(pg)) {
		if (pv->pv_pmap == NULL) {
			pmap_page_clear_attributes(pg, VM_PAGE_MD_EXECPAGE);
			PMAP_COUNT(exec_uncached_clear_modify);
		} else {
			pmap_page_syncicache(pg);
			PMAP_COUNT(exec_synced_clear_modify);
		}
	}
	if (!pmap_page_clear_attributes(pg, VM_PAGE_MD_MODIFIED))
		return false;
	if (pv->pv_pmap == NULL) {
		return true;
	}

	/*
	 * remove write access from any pages that are dirty
	 * so we can tell if they are written to again later.
	 * flush the VAC first if there is one.
	 */
	kpreempt_disable();
	gen = VM_PAGE_PVLIST_LOCK(pg, false);
	for (; pv != NULL; pv = pv_next) {
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = pv->pv_va;
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		KASSERT(ptep);
		pv_next = pv->pv_next;
		pt_entry_t pt_entry = pte_prot_nowrite(*ptep);
		if (*ptep == pt_entry) {
			continue;
		}
		pmap_md_vca_clean(pg, va, PMAP_WBINV);
		*ptep = pt_entry;
		VM_PAGE_PVLIST_UNLOCK(pg);
		pmap_tlb_invalidate_addr(pmap, va);
		pmap_update(pmap);
		if (__predict_false(gen != VM_PAGE_PVLIST_LOCK(pg, false))) {
			/*
			 * The list changed!  So restart from the beginning.
			 */
			pv_next = &VM_PAGE_TO_MD(pg)->mdpg_first;
		}
	}
	VM_PAGE_PVLIST_UNLOCK(pg);
	kpreempt_enable();
	return true;
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */
bool
pmap_is_modified(struct vm_page *pg)
{

	return VM_PAGE_MD_MODIFIED_P(pg);
}

/*
 *	pmap_set_modified:
 *
 *	Sets the page modified reference bit for the specified page.
 */
void
pmap_set_modified(paddr_t pa)
{
	struct vm_page *pg = PHYS_TO_VM_PAGE(pa);
	pmap_page_set_attributes(pg, VM_PAGE_MD_MODIFIED | VM_PAGE_MD_REFERENCED);
}

/******************** pv_entry management ********************/

static void
pmap_check_pvlist(struct vm_page *pg)
{
#ifdef PARANOIADIAG
	pt_entry_t pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
	if (pv->pv_pmap != NULL) {
		for (; pv != NULL; pv = pv->pv_next) {
			KASSERT(!pmap_md_direct_mapped_vaddr_p(pv->pv_va));
		}
		pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
	}
#endif /* PARANOIADIAG */
}

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
pmap_enter_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, u_int *npte)
{
	pv_entry_t pv, npv, apv;
	int16_t gen;

        KASSERT(kpreempt_disabled());
	KASSERT(pmap != pmap_kernel() || !pmap_md_direct_mapped_vaddr_p(va));

	apv = NULL;
	pv = &VM_PAGE_TO_MD(pg)->mdpg_first;
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: pv %p: was %#"PRIxVADDR"/%p/%p\n",
		    pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
	gen = VM_PAGE_PVLIST_LOCK(pg, true);
	pmap_check_pvlist(pg);
again:
	if (pv->pv_pmap == NULL) {
		KASSERT(pv->pv_next == NULL);
		/*
		 * No entries yet, use header as the first entry
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter_pv: first pv: pmap %p va %#"PRIxVADDR"\n",
			    pmap, va);
#endif
		PMAP_COUNT(primary_mappings);
		PMAP_COUNT(mappings);
#ifdef VM_PAGE_MD_UNCACHED
		pmap_page_clear_attributes(pg, VM_PAGE_MD_UNCACHED);
#endif
		pv->pv_pmap = pmap;
		pv->pv_va = va;
	} else {
		if (pmap_md_vca_add(pg, va, npte))
			goto again;

		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 *
		 * Note: the entry may already be in the table if
		 * we are only changing the protection bits.
		 */

#ifdef PARANOIADIAG
		const paddr_t pa = VM_PAGE_TO_PHYS(pg);
#endif
		for (npv = pv; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va) {
#ifdef PARANOIADIAG
				pt_entry_t *ptep = pmap_pte_lookup(pmap, va);
				pt_entry_t pt_entry = (ptep ? *ptep : 0);
				if (!pte_valid_p(pt_entry)
				    || pte_to_paddr(pt_entry) != pa)
					printf(
		"pmap_enter_pv: found va %#"PRIxVADDR" pa %#"PRIxPADDR" in pv_table but != %x\n",
					    va, pa, pt_entry);
#endif
				PMAP_COUNT(remappings);
				VM_PAGE_PVLIST_UNLOCK(pg);
				if (__predict_false(apv != NULL))
					pmap_pv_free(apv);
				return;
			}
		}
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter_pv: new pv: pmap %p va %#"PRIxVADDR"\n",
			    pmap, va);
#endif
		if (__predict_true(apv == NULL)) {
#ifdef MULTIPROCESSOR
			/*
			 * To allocate a PV, we have to release the PVLIST lock
			 * so get the page generation.  We allocate the PV, and
			 * then reacquire the lock.  
			 */
			VM_PAGE_PVLIST_UNLOCK(pg);
#endif
			apv = (pv_entry_t)pmap_pv_alloc();
			if (apv == NULL)
				panic("pmap_enter_pv: pmap_pv_alloc() failed");
#ifdef MULTIPROCESSOR
			/*
			 * If the generation has changed, then someone else
			 * tinkered with this page so we should
			 * start over.
			 */
			uint16_t oldgen = gen;
			gen = VM_PAGE_PVLIST_LOCK(pg, true);
			if (gen != oldgen)
				goto again;
#endif
		}
		npv = apv;
		apv = NULL;
		npv->pv_va = va;
		npv->pv_pmap = pmap;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
		PMAP_COUNT(mappings);
	}
	pmap_check_pvlist(pg);
	VM_PAGE_PVLIST_UNLOCK(pg);
	if (__predict_false(apv != NULL))
		pmap_pv_free(apv);
}

/*
 * Remove a physical to virtual address translation.
 * If cache was inhibited on this page, and there are no more cache
 * conflicts, restore caching.
 * Flush the cache if the last page is removed (should always be cached
 * at this point).
 */
void
pmap_remove_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, bool dirty)
{
	pv_entry_t pv, npv;
	bool last;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PVENTRY))
		printf("pmap_remove_pv(%p, %#"PRIxVADDR", %#"PRIxPADDR")\n", pmap, va,
		    VM_PAGE_TO_PHYS(pg));
#endif
	KASSERT(kpreempt_disabled());
	pv = &VM_PAGE_TO_MD(pg)->mdpg_first;

	(void)VM_PAGE_PVLIST_LOCK(pg, true);
	pmap_check_pvlist(pg);

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	last = false;
	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			KASSERT(pv->pv_pmap != NULL);
		} else {
#ifdef VM_PAGE_MD_UNCACHED
			pmap_page_clear_attributes(pg, VM_PAGE_MD_UNCACHED);
#endif
			pv->pv_pmap = NULL;
			last = true;	/* Last mapping removed */
		}
		PMAP_COUNT(remove_pvfirst);
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
			PMAP_COUNT(remove_pvsearch);
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
		}
	}
	pmap_md_vca_remove(pg, va);

	pmap_check_pvlist(pg);
	VM_PAGE_PVLIST_UNLOCK(pg);

	/*
	 * Free the pv_entry if needed.
	 */
	if (npv)
		pmap_pv_free(npv);
	if (VM_PAGE_MD_EXECPAGE_P(pg) && dirty) {
		if (last) {
			/*
			 * If this was the page's last mapping, we no longer
			 * care about its execness.
			 */
			pmap_page_clear_attributes(pg, VM_PAGE_MD_EXECPAGE);
			PMAP_COUNT(exec_uncached_remove);
		} else {
			/*
			 * Someone still has it mapped as an executable page
			 * so we must sync it.
			 */
			pmap_page_syncicache(pg);
			PMAP_COUNT(exec_synced_remove);
		}
	}
}

#ifdef MULTIPROCESSOR
struct pmap_pvlist_info {
	kmutex_t *pli_locks[PAGE_SIZE / 32];
	volatile u_int pli_lock_refs[PAGE_SIZE / 32];
	volatile u_int pli_lock_index;
	u_int pli_lock_mask;
} pmap_pvlist_info;

void
pmap_pvlist_lock_init(size_t cache_line_size)
{
	struct pmap_pvlist_info * const pli = &pmap_pvlist_info;
	const vaddr_t lock_page = uvm_pageboot_alloc(PAGE_SIZE);
	vaddr_t lock_va = lock_page;
	if (sizeof(kmutex_t) > cache_line_size) {
		cache_line_size = roundup2(sizeof(kmutex_t), cache_line_size);
	}
	const size_t nlocks = PAGE_SIZE / cache_line_size;
	KASSERT((nlocks & (nlocks - 1)) == 0);
	/*
	 * Now divide the page into a number of mutexes, one per cacheline.
	 */
	for (size_t i = 0; i < nlocks; lock_va += cache_line_size, i++) {
		kmutex_t * const lock = (kmutex_t *)lock_va;
		mutex_init(lock, MUTEX_DEFAULT, IPL_VM);
		pli->pli_locks[i] = lock;
	}
	pli->pli_lock_mask = nlocks - 1;
}

uint16_t
pmap_pvlist_lock(struct vm_page *pg, bool list_change)
{
	struct pmap_pvlist_info * const pli = &pmap_pvlist_info;
	kmutex_t *lock = VM_PAGE_TO_MD(pg)->mdpg_lock;
	int16_t gen;

	/*
	 * Allocate a lock on an as-needed basis.  This will hopefully give us
	 * semi-random distribution not based on page color.
	 */
	if (__predict_false(lock == NULL)) {
		size_t locknum = atomic_add_int_nv(&pli->pli_lock_index, 37);
		size_t lockid = locknum & pli->pli_lock_mask;
		kmutex_t * const new_lock = pli->pli_locks[lockid];
		/*
		 * Set the lock.  If some other thread already did, just use
		 * the one they assigned.
		 */
		lock = atomic_cas_ptr(&VM_PAGE_TO_MD(pg)->mdpg_lock, NULL, new_lock);
		if (lock == NULL) {
			lock = new_lock;
			atomic_inc_uint(&pli->pli_lock_refs[lockid]);
		}
	}

	/*
	 * Now finally lock the pvlists.
	 */
	mutex_spin_enter(lock);

	/*
	 * If the locker will be changing the list, increment the high 16 bits
	 * of attrs so we use that as a generation number.
	 */
	gen = VM_PAGE_PVLIST_GEN(pg);		/* get old value */
	if (list_change)
		atomic_add_int(&VM_PAGE_TO_MD(pg)->mdpg_attrs, 0x10000);

	/*
	 * Return the generation number.
	 */
	return gen;
}
#else
void
pmap_pvlist_lock_init(size_t cache_line_size)
{
	mutex_init(&pmap_pvlist_mutex, MUTEX_DEFAULT, IPL_VM);
}
#endif /* MULTIPROCESSOR */

/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	struct vm_page *pg = PMAP_ALLOC_POOLPAGE(UVM_PGA_USERESERVE);
	if (pg == NULL)
		return NULL;

	return (void *)pmap_map_poolpage(VM_PAGE_TO_PHYS(pg));
}

/*
 * pmap_pv_page_free:
 *
 *	Free a pv_entry pool page.
 */
void
pmap_pv_page_free(struct pool *pp, void *v)
{
	vaddr_t va = (vaddr_t)v;

	KASSERT(pmap_md_direct_mapped_vaddr_p(va));
	const paddr_t pa = pmap_md_direct_mapped_vaddr_to_paddr(va);
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	pmap_md_vca_remove(pg, va);
	pmap_page_clear_attributes(pg, VM_PAGE_MD_POOLPAGE);
	uvm_pagefree(pg);
}

#ifdef PMAP_PREFER
/*
 * Find first virtual address >= *vap that doesn't cause
 * a cache alias conflict.
 */
void
pmap_prefer(vaddr_t foff, vaddr_t *vap, vsize_t sz, int td)
{
	vaddr_t	va;
	vsize_t d;
	vsize_t prefer_mask = ptoa(uvmexp.colormask);

	PMAP_COUNT(prefer_requests);

	prefer_mask |= pmap_md_cache_prefer_mask();

	if (prefer_mask) {
		va = *vap;

		d = foff - va;
		d &= prefer_mask;
		if (d) {
			if (td)
				*vap = trunc_page(va -((-d) & prefer_mask));
			else
				*vap = round_page(va + d);
			PMAP_COUNT(prefer_adjustments);
		}
	}
}
#endif /* PMAP_PREFER */

#ifdef PMAP_MAP_POOLPAGE
vaddr_t
pmap_map_poolpage(paddr_t pa)
{

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg);
	pmap_page_set_attributes(pg, VM_PAGE_MD_POOLPAGE);

	const vaddr_t va = pmap_md_direct_map_paddr(pa);
	pmap_md_vca_add(pg, va, NULL);
	return va;
}

paddr_t
pmap_unmap_poolpage(vaddr_t va)
{

	KASSERT(pmap_md_direct_mapped_vaddr_p(va));
	paddr_t pa = pmap_md_direct_mapped_vaddr_to_paddr(va);

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg);
	pmap_page_clear_attributes(pg, VM_PAGE_MD_POOLPAGE);
	pmap_md_vca_remove(pg, va);

	return pa;
}
#endif /* PMAP_MAP_POOLPAGE */
