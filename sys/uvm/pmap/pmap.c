/*	$NetBSD: pmap.c,v 1.40.2.2 2017/12/03 11:39:23 jdolecek Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.40.2.2 2017/12/03 11:39:23 jdolecek Exp $");

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

#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_sysv.h"

#define __PMAP_PRIVATE

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>
#include <uvm/uvm_physseg.h>

#if defined(MULTIPROCESSOR) && defined(PMAP_VIRTUAL_CACHE_ALIASES) \
    && !defined(PMAP_NO_PV_UNCACHED)
#error PMAP_VIRTUAL_CACHE_ALIASES with MULTIPROCESSOR requires \
 PMAP_NO_PV_UNCACHED to be defined
#endif

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

#define PMAP_ASID_RESERVED 0
CTASSERT(PMAP_ASID_RESERVED == 0);

#ifndef PMAP_SEGTAB_ALIGN
#define PMAP_SEGTAB_ALIGN	/* nothing */
#endif
#ifdef _LP64
pmap_segtab_t	pmap_kstart_segtab PMAP_SEGTAB_ALIGN; /* first mid-level segtab for kernel */
#endif
pmap_segtab_t	pmap_kern_segtab PMAP_SEGTAB_ALIGN = { /* top level segtab for kernel */
#ifdef _LP64
	.seg_seg[(VM_MIN_KERNEL_ADDRESS & XSEGOFSET) >> SEGSHIFT] = &pmap_kstart_segtab,
#endif
};

struct pmap_kernel kernel_pmap_store = {
	.kernel_pmap = {
		.pm_count = 1,
		.pm_segtab = &pmap_kern_segtab,
		.pm_minaddr = VM_MIN_KERNEL_ADDRESS,
		.pm_maxaddr = VM_MAX_KERNEL_ADDRESS,
	},
};

struct pmap * const kernel_pmap_ptr = &kernel_pmap_store.kernel_pmap;

struct pmap_limits pmap_limits = {	/* VA and PA limits */
	.virtual_start = VM_MIN_KERNEL_ADDRESS,
};

#ifdef UVMHIST
static struct kern_history_ent pmapexechistbuf[10000];
static struct kern_history_ent pmaphistbuf[10000];
UVMHIST_DEFINE(pmapexechist);
UVMHIST_DEFINE(pmaphist);
#endif

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
struct pool pmap_pmap_pool;
struct pool pmap_pv_pool;

#ifndef PMAP_PV_LOWAT
#define	PMAP_PV_LOWAT	16
#endif
int	pmap_pv_lowat = PMAP_PV_LOWAT;

bool	pmap_initialized = false;
#define	PMAP_PAGE_COLOROK_P(a, b) \
		((((int)(a) ^ (int)(b)) & pmap_page_colormask) == 0)
u_int	pmap_page_colormask;

#define PAGE_IS_MANAGED(pa)	(pmap_initialized && uvm_pageismanaged(pa))

#define PMAP_IS_ACTIVE(pm)						\
	((pm) == pmap_kernel() || 					\
	 (pm) == curlwp->l_proc->p_vmspace->vm_map.pmap)

/* Forward function declarations */
void pmap_page_remove(struct vm_page *);
static void pmap_pvlist_check(struct vm_page_md *);
void pmap_remove_pv(pmap_t, vaddr_t, struct vm_page *, bool);
void pmap_enter_pv(pmap_t, vaddr_t, struct vm_page *, pt_entry_t *, u_int);

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

#if !defined(MULTIPROCESSOR) || !defined(PMAP_MD_NEED_TLB_MISS_LOCK)
#define	pmap_md_tlb_miss_lock_enter()	do { } while(/*CONSTCOND*/0)
#define	pmap_md_tlb_miss_lock_exit()	do { } while(/*CONSTCOND*/0)
#endif /* !MULTIPROCESSOR || !PMAP_MD_NEED_TLB_MISS_LOCK */

#ifndef MULTIPROCESSOR
kmutex_t pmap_pvlist_mutex	__cacheline_aligned;
#endif

/*
 * Debug functions.
 */

#ifdef DEBUG
static inline void
pmap_asid_check(pmap_t pm, const char *func)
{
	if (!PMAP_IS_ACTIVE(pm))
		return;

	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(curcpu()));
	tlb_asid_t asid = tlb_get_asid();
	if (asid != pai->pai_asid)
		panic("%s: inconsistency for active TLB update: %u <-> %u",
		    func, asid, pai->pai_asid);
}
#endif

static void
pmap_addr_range_check(pmap_t pmap, vaddr_t sva, vaddr_t eva, const char *func)
{
#ifdef DEBUG
	if (pmap == pmap_kernel()) {
		if (sva < VM_MIN_KERNEL_ADDRESS)
			panic("%s: kva %#"PRIxVADDR" not in range",
			    func, sva);
		if (eva >= pmap_limits.virtual_end)
			panic("%s: kva %#"PRIxVADDR" not in range",
			    func, eva);
	} else {
		if (eva > VM_MAXUSER_ADDRESS)
			panic("%s: uva %#"PRIxVADDR" not in range",
			    func, eva);
		pmap_asid_check(pmap, func);
	}
#endif
}

/*
 * Misc. functions.
 */

bool
pmap_page_clear_attributes(struct vm_page_md *mdpg, u_int clear_attributes)
{
	volatile unsigned long * const attrp = &mdpg->mdpg_attrs;
#ifdef MULTIPROCESSOR
	for (;;) {
		u_int old_attr = *attrp;
		if ((old_attr & clear_attributes) == 0)
			return false;
		u_int new_attr = old_attr & ~clear_attributes;
		if (old_attr == atomic_cas_ulong(attrp, old_attr, new_attr))
			return true;
	}
#else
	unsigned long old_attr = *attrp;
	if ((old_attr & clear_attributes) == 0)
		return false;
	*attrp &= ~clear_attributes;
	return true;
#endif
}

void
pmap_page_set_attributes(struct vm_page_md *mdpg, u_int set_attributes)
{
#ifdef MULTIPROCESSOR
	atomic_or_ulong(&mdpg->mdpg_attrs, set_attributes);
#else
	mdpg->mdpg_attrs |= set_attributes;
#endif
}

static void
pmap_page_syncicache(struct vm_page *pg)
{
#ifndef MULTIPROCESSOR
	struct pmap * const curpmap = curlwp->l_proc->p_vmspace->vm_map.pmap;
#endif
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv = &mdpg->mdpg_first;
	kcpuset_t *onproc;
#ifdef MULTIPROCESSOR
	kcpuset_create(&onproc, true);
	KASSERT(onproc != NULL);
#else
	onproc = NULL;
#endif
	VM_PAGEMD_PVLIST_READLOCK(mdpg);
	pmap_pvlist_check(mdpg);

	if (pv->pv_pmap != NULL) {
		for (; pv != NULL; pv = pv->pv_next) {
#ifdef MULTIPROCESSOR
			kcpuset_merge(onproc, pv->pv_pmap->pm_onproc);
			if (kcpuset_match(onproc, kcpuset_running)) {
				break;
			}
#else
			if (pv->pv_pmap == curpmap) {
				onproc = curcpu()->ci_data.cpu_kcpuset;
				break;
			}
#endif
		}
	}
	pmap_pvlist_check(mdpg);
	VM_PAGEMD_PVLIST_UNLOCK(mdpg);
	kpreempt_disable();
	pmap_md_page_syncicache(pg, onproc);
	kpreempt_enable();
#ifdef MULTIPROCESSOR
	kcpuset_destroy(onproc);
#endif
}

/*
 * Define the initial bounds of the kernel virtual address space.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{

	*vstartp = pmap_limits.virtual_start;
	*vendp = pmap_limits.virtual_end;
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	vaddr_t virtual_end = pmap_limits.virtual_end;
	maxkvaddr = pmap_round_seg(maxkvaddr) - 1;

	/*
	 * Reserve PTEs for the new KVA space.
	 */
	for (; virtual_end < maxkvaddr; virtual_end += NBSEG) {
		pmap_pte_reserve(pmap_kernel(), virtual_end, 0);
	}

	/*
	 * Don't exceed VM_MAX_KERNEL_ADDRESS!
	 */
	if (virtual_end == 0 || virtual_end > VM_MAX_KERNEL_ADDRESS)
		virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Update new end.
	 */
	pmap_limits.virtual_end = virtual_end;
	return virtual_end;
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
	size_t npgs;
	paddr_t pa;
	vaddr_t va;

	uvm_physseg_t maybe_bank = UVM_PHYSSEG_TYPE_INVALID;

	size = round_page(size);
	npgs = atop(size);

	aprint_debug("%s: need %zu pages\n", __func__, npgs);

	for (uvm_physseg_t bank = uvm_physseg_get_first();
	     uvm_physseg_valid_p(bank);
	     bank = uvm_physseg_get_next(bank)) {

		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

		aprint_debug("%s: seg %"PRIxPHYSSEG": %#"PRIxPADDR" %#"PRIxPADDR" %#"PRIxPADDR" %#"PRIxPADDR"\n",
		    __func__, bank,
		    uvm_physseg_get_avail_start(bank), uvm_physseg_get_start(bank),
		    uvm_physseg_get_avail_end(bank), uvm_physseg_get_end(bank));

		if (uvm_physseg_get_avail_start(bank) != uvm_physseg_get_start(bank)
		    || uvm_physseg_get_avail_start(bank) >= uvm_physseg_get_avail_end(bank)) {
			aprint_debug("%s: seg %"PRIxPHYSSEG": bad start\n", __func__, bank);
			continue;
		}

		if (uvm_physseg_get_avail_end(bank) - uvm_physseg_get_avail_start(bank) < npgs) {
			aprint_debug("%s: seg %"PRIxPHYSSEG": too small for %zu pages\n",
			    __func__, bank, npgs);
			continue;
		}

		if (!pmap_md_ok_to_steal_p(bank, npgs)) {
			continue;
		}

		/*
		 * Always try to allocate from the segment with the least
		 * amount of space left.
		 */
#define VM_PHYSMEM_SPACE(b)	((uvm_physseg_get_avail_end(b)) - (uvm_physseg_get_avail_start(b)))
		if (uvm_physseg_valid_p(maybe_bank) == false
		    || VM_PHYSMEM_SPACE(bank) < VM_PHYSMEM_SPACE(maybe_bank)) {
			maybe_bank = bank;
		}
	}

	if (uvm_physseg_valid_p(maybe_bank)) {
		const uvm_physseg_t bank = maybe_bank;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(uvm_physseg_get_start(bank));
		uvm_physseg_unplug(atop(pa), npgs);

		aprint_debug("%s: seg %"PRIxPHYSSEG": %zu pages stolen (%#"PRIxPADDR" left)\n",
		    __func__, bank, npgs, VM_PHYSMEM_SPACE(bank));

		va = pmap_md_map_poolpage(pa, size);
		memset((void *)va, 0, size);
		return va;
	}

	/*
	 * If we got here, there was no memory left.
	 */
	panic("pmap_steal_memory: no memory to steal %zu pages", npgs);
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(void)
{
	UVMHIST_INIT_STATIC(pmapexechist, pmapexechistbuf);
	UVMHIST_INIT_STATIC(pmaphist, pmaphistbuf);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);

	/*
	 * Initialize the segtab lock.
	 */
	mutex_init(&pmap_segtab_lock, MUTEX_DEFAULT, IPL_HIGH);

	/*
	 * Set a low water mark on the pv_entry pool, so that we are
	 * more likely to have these around even in extreme memory
	 * starvation.
	 */
	pool_setlowat(&pmap_pv_pool, pmap_pv_lowat);

	/*
	 * Set the page colormask but allow pmap_md_init to override it.
	 */
	pmap_page_colormask = ptoa(uvmexp.colormask);

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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	PMAP_COUNT(create);

	pmap_t pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, PMAP_SIZE);

	KASSERT(pmap->pm_pai[0].pai_link.le_prev == NULL);

	pmap->pm_count = 1;
	pmap->pm_minaddr = VM_MIN_ADDRESS;
	pmap->pm_maxaddr = VM_MAXUSER_ADDRESS;

	pmap_segtab_init(pmap);

#ifdef MULTIPROCESSOR
	kcpuset_create(&pmap->pm_active, true);
	kcpuset_create(&pmap->pm_onproc, true);
	KASSERT(pmap->pm_active != NULL);
	KASSERT(pmap->pm_onproc != NULL);
#endif

	UVMHIST_LOG(pmaphist, " <-- done (pmap=%#jx)", (uintptr_t)pmap,
	    0, 0, 0);

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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx)", (uintptr_t)pmap, 0, 0, 0);

	if (atomic_dec_uint_nv(&pmap->pm_count) > 0) {
		PMAP_COUNT(dereference);
		UVMHIST_LOG(pmaphist, " <-- done (deref)", 0, 0, 0, 0);
		return;
	}

	PMAP_COUNT(destroy);
	KASSERT(pmap->pm_count == 0);
	kpreempt_disable();
	pmap_md_tlb_miss_lock_enter();
	pmap_tlb_asid_release_all(pmap);
	pmap_segtab_destroy(pmap, NULL, 0);
	pmap_md_tlb_miss_lock_exit();

#ifdef MULTIPROCESSOR
	kcpuset_destroy(pmap->pm_active);
	kcpuset_destroy(pmap->pm_onproc);
	pmap->pm_active = NULL;
	pmap->pm_onproc = NULL;
#endif

	pool_put(&pmap_pmap_pool, pmap);
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done (freed)", 0, 0, 0, 0);
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx)", (uintptr_t)pmap, 0, 0, 0);
	PMAP_COUNT(reference);

	if (pmap != NULL) {
		atomic_inc_uint(&pmap->pm_count);
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

/*
 *	Make a new pmap (vmspace) active for the given process.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(l=%#jx pmap=%#jx)", (uintptr_t)l,
	    (uintptr_t)pmap, 0, 0);
	PMAP_COUNT(activate);

	kpreempt_disable();
	pmap_md_tlb_miss_lock_enter();
	pmap_tlb_asid_acquire(pmap, l);
	if (l == curlwp) {
		pmap_segtab_activate(pmap, l);
	}
	pmap_md_tlb_miss_lock_exit();
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done (%ju:%ju)", l->l_proc->p_pid,
	    l->l_lid, 0, 0);
}

/*
 * Remove this page from all physical maps in which it resides.
 * Reflects back modify bits to the pager.
 */
void
pmap_page_remove(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

	kpreempt_disable();
	VM_PAGEMD_PVLIST_LOCK(mdpg);
	pmap_pvlist_check(mdpg);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);

	UVMHIST_LOG(pmapexechist, "pg %#jx (pa %#jx) [page removed]: "
	    "execpage cleared", (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), 0, 0);
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
	pmap_page_clear_attributes(mdpg, VM_PAGEMD_EXECPAGE|VM_PAGEMD_UNCACHED);
#else
	pmap_page_clear_attributes(mdpg, VM_PAGEMD_EXECPAGE);
#endif
	PMAP_COUNT(exec_uncached_remove);

	pv_entry_t pv = &mdpg->mdpg_first;
	if (pv->pv_pmap == NULL) {
		VM_PAGEMD_PVLIST_UNLOCK(mdpg);
		kpreempt_enable();
		UVMHIST_LOG(pmaphist, " <-- done (empty)", 0, 0, 0, 0);
		return;
	}

	pv_entry_t npv;
	pv_entry_t pvp = NULL;

	for (; pv != NULL; pv = npv) {
		npv = pv->pv_next;
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		if (pv->pv_va & PV_KENTER) {
			UVMHIST_LOG(pmaphist, " pv %#jx pmap %#jx va %jx"
			    " skip", (uintptr_t)pv, (uintptr_t)pv->pv_pmap,
			    pv->pv_va, 0);

			KASSERT(pv->pv_pmap == pmap_kernel());

			/* Assume no more - it'll get fixed if there are */
			pv->pv_next = NULL;

			/*
			 * pvp is non-null when we already have a PV_KENTER
			 * pv in pvh_first; otherwise we haven't seen a
			 * PV_KENTER pv and we need to copy this one to
			 * pvh_first
			 */
			if (pvp) {
				/*
				 * The previous PV_KENTER pv needs to point to
				 * this PV_KENTER pv
				 */
				pvp->pv_next = pv;
			} else {
				pv_entry_t fpv = &mdpg->mdpg_first;
				*fpv = *pv;
				KASSERT(fpv->pv_pmap == pmap_kernel());
			}
			pvp = pv;
			continue;
		}
#endif
		const pmap_t pmap = pv->pv_pmap;
		vaddr_t va = trunc_page(pv->pv_va);
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		KASSERTMSG(ptep != NULL, "%#"PRIxVADDR " %#"PRIxVADDR, va,
		    pmap_limits.virtual_end);
		pt_entry_t pte = *ptep;
		UVMHIST_LOG(pmaphist, " pv %#jx pmap %#jx va %jx"
		    " pte %jx", (uintptr_t)pv, (uintptr_t)pmap, va,
		    pte_value(pte));
		if (!pte_valid_p(pte))
			continue;
		const bool is_kernel_pmap_p = (pmap == pmap_kernel());
		if (is_kernel_pmap_p) {
			PMAP_COUNT(remove_kernel_pages);
		} else {
			PMAP_COUNT(remove_user_pages);
		}
		if (pte_wired_p(pte))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;

		pmap_md_tlb_miss_lock_enter();
		const pt_entry_t npte = pte_nv_entry(is_kernel_pmap_p);
		pte_set(ptep, npte);
		if (__predict_true(!(pmap->pm_flags & PMAP_DEFERRED_ACTIVATE))) {
			/*
			 * Flush the TLB for the given address.
			 */
			pmap_tlb_invalidate_addr(pmap, va);
		}
		pmap_md_tlb_miss_lock_exit();

		/*
		 * non-null means this is a non-pvh_first pv, so we should
		 * free it.
		 */
		if (pvp) {
			KASSERT(pvp->pv_pmap == pmap_kernel());
			KASSERT(pvp->pv_next == NULL);
			pmap_pv_free(pv);
		} else {
			pv->pv_pmap = NULL;
			pv->pv_next = NULL;
		}
	}

	pmap_pvlist_check(mdpg);
	VM_PAGEMD_PVLIST_UNLOCK(mdpg);
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}


/*
 *	Make a previously active pmap (vmspace) inactive.
 */
void
pmap_deactivate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(l=%#jx pmap=%#jx)", (uintptr_t)l,
	    (uintptr_t)pmap, 0, 0);
	PMAP_COUNT(deactivate);

	kpreempt_disable();
	KASSERT(l == curlwp || l->l_cpu == curlwp->l_cpu);
	pmap_md_tlb_miss_lock_enter();
	curcpu()->ci_pmap_user_segtab = PMAP_INVALID_SEGTAB_ADDRESS;
#ifdef _LP64
	curcpu()->ci_pmap_user_seg0tab = NULL;
#endif
	pmap_tlb_asid_deactivate(pmap);
	pmap_md_tlb_miss_lock_exit();
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done (%ju:%ju)", l->l_proc->p_pid,
	    l->l_lid, 0, 0);
}

void
pmap_update(struct pmap *pmap)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx)", (uintptr_t)pmap, 0, 0, 0);
	PMAP_COUNT(update);

	kpreempt_disable();
#if defined(MULTIPROCESSOR) && defined(PMAP_TLB_NEED_SHOOTDOWN)
	u_int pending = atomic_swap_uint(&pmap->pm_shootdown_pending, 0);
	if (pending && pmap_tlb_shootdown_bystanders(pmap))
		PMAP_COUNT(shootdown_ipis);
#endif
	pmap_md_tlb_miss_lock_enter();
#if defined(DEBUG) && !defined(MULTIPROCESSOR)
	pmap_tlb_check(pmap, pmap_md_tlb_check_entry);
#endif /* DEBUG */

	/*
	 * If pmap_remove_all was called, we deactivated ourselves and nuked
	 * our ASID.  Now we have to reactivate ourselves.
	 */
	if (__predict_false(pmap->pm_flags & PMAP_DEFERRED_ACTIVATE)) {
		pmap->pm_flags ^= PMAP_DEFERRED_ACTIVATE;
		pmap_tlb_asid_acquire(pmap, curlwp);
		pmap_segtab_activate(pmap, curlwp);
	}
	pmap_md_tlb_miss_lock_exit();
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done (kernel=%#jx)",
		    (pmap == pmap_kernel() ? 1 : 0), 0, 0, 0);
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

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx kernel=%c va=%#jx..%#jx)",
	    (uintptr_t)pmap, (is_kernel_pmap_p ? 1 : 0), sva, eva);
	UVMHIST_LOG(pmaphist, "ptep=%#jx, flags(npte)=%#jx",
	    (uintptr_t)ptep, flags, 0, 0);

	KASSERT(kpreempt_disabled());

	for (; sva < eva; sva += NBPG, ptep++) {
		const pt_entry_t pte = *ptep;
		if (!pte_valid_p(pte))
			continue;
		if (is_kernel_pmap_p) {
			PMAP_COUNT(remove_kernel_pages);
		} else {
			PMAP_COUNT(remove_user_pages);
		}
		if (pte_wired_p(pte))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pte_to_paddr(pte));
		if (__predict_true(pg != NULL)) {
			pmap_remove_pv(pmap, sva, pg, pte_modified_p(pte));
		}
		pmap_md_tlb_miss_lock_enter();
		pte_set(ptep, npte);
		if (__predict_true(!(pmap->pm_flags & PMAP_DEFERRED_ACTIVATE))) {

			/*
			 * Flush the TLB for the given address.
			 */
			pmap_tlb_invalidate_addr(pmap, sva);
		}
		pmap_md_tlb_miss_lock_exit();
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);

	return false;
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	const bool is_kernel_pmap_p = (pmap == pmap_kernel());
	const pt_entry_t npte = pte_nv_entry(is_kernel_pmap_p);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx, va=%#jx..%#jx)",
	    (uintptr_t)pmap, sva, eva, 0);

	if (is_kernel_pmap_p) {
		PMAP_COUNT(remove_kernel_calls);
	} else {
		PMAP_COUNT(remove_user_calls);
	}
#ifdef PMAP_FAULTINFO
	curpcb->pcb_faultinfo.pfi_faultaddr = 0;
	curpcb->pcb_faultinfo.pfi_repeats = 0;
	curpcb->pcb_faultinfo.pfi_faultpte = NULL;
#endif
	kpreempt_disable();
	pmap_addr_range_check(pmap, sva, eva, __func__);
	pmap_pte_process(pmap, sva, eva, pmap_pte_remove, npte);
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

/*
 *	pmap_page_protect:
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv;
	vaddr_t va;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pg=%#jx (pa %#jx) prot=%#jx)",
	    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), prot, 0);
	PMAP_COUNT(page_protect);

	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
	case VM_PROT_ALL:
		break;

	/* copy_on_write */
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pv = &mdpg->mdpg_first;
		kpreempt_disable();
		VM_PAGEMD_PVLIST_READLOCK(mdpg);
		pmap_pvlist_check(mdpg);
		/*
		 * Loop over all current mappings setting/clearing as
		 * appropriate.
		 */
		if (pv->pv_pmap != NULL) {
			while (pv != NULL) {
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
				if (pv->pv_va & PV_KENTER) {
					pv = pv->pv_next;
					continue;
				}
#endif
				const pmap_t pmap = pv->pv_pmap;
				va = trunc_page(pv->pv_va);
				const uintptr_t gen =
				    VM_PAGEMD_PVLIST_UNLOCK(mdpg);
				pmap_protect(pmap, va, va + PAGE_SIZE, prot);
				KASSERT(pv->pv_pmap == pmap);
				pmap_update(pmap);
				if (gen != VM_PAGEMD_PVLIST_READLOCK(mdpg)) {
					pv = &mdpg->mdpg_first;
				} else {
					pv = pv->pv_next;
				}
				pmap_pvlist_check(mdpg);
			}
		}
		pmap_pvlist_check(mdpg);
		VM_PAGEMD_PVLIST_UNLOCK(mdpg);
		kpreempt_enable();
		break;

	/* remove_all */
	default:
		pmap_page_remove(pg);
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

static bool
pmap_pte_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *ptep,
	uintptr_t flags)
{
	const vm_prot_t prot = (flags & VM_PROT_ALL);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx kernel=%jx va=%#jx..%#jx)",
	    (uintptr_t)pmap, (pmap == pmap_kernel() ? 1 : 0), sva, eva);
	UVMHIST_LOG(pmaphist, "ptep=%#jx, flags(npte)=%#jx)",
	    (uintptr_t)ptep, flags, 0, 0);

	KASSERT(kpreempt_disabled());
	/*
	 * Change protection on every valid mapping within this segment.
	 */
	for (; sva < eva; sva += NBPG, ptep++) {
		pt_entry_t pte = *ptep;
		if (!pte_valid_p(pte))
			continue;
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pte_to_paddr(pte));
		if (pg != NULL && pte_modified_p(pte)) {
			struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
			if (VM_PAGEMD_EXECPAGE_P(mdpg)) {
				KASSERT(mdpg->mdpg_first.pv_pmap != NULL);
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
				if (VM_PAGEMD_CACHED_P(mdpg)) {
#endif
					UVMHIST_LOG(pmapexechist,
					    "pg %#jx (pa %#jx): "
					    "syncicached performed",
					    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg),
					    0, 0);
					pmap_page_syncicache(pg);
					PMAP_COUNT(exec_synced_protect);
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
				}
#endif
			}
		}
		pte = pte_prot_downgrade(pte, prot);
		if (*ptep != pte) {
			pmap_md_tlb_miss_lock_enter();
			pte_set(ptep, pte);
			/*
			 * Update the TLB if needed.
			 */
			pmap_tlb_update_addr(pmap, sva, pte, PMAP_TLB_NEED_IPI);
			pmap_md_tlb_miss_lock_exit();
		}
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);

	return false;
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx, va=%#jx..%#jx, prot=%ju)",
	    (uintptr_t)pmap, sva, eva, prot);
	PMAP_COUNT(protect);

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
		return;
	}

	/*
	 * Change protection on every valid mapping within this segment.
	 */
	kpreempt_disable();
	pmap_addr_range_check(pmap, sva, eva, __func__);
	pmap_pte_process(pmap, sva, eva, pmap_pte_protect, prot);
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

#if defined(PMAP_VIRTUAL_CACHE_ALIASES) && !defined(PMAP_NO_PV_UNCACHED)
/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a managed page to cached/uncached.
 */
void
pmap_page_cache(struct vm_page *pg, bool cached)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pg=%#jx (pa %#jx) cached=%jd)",
	    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), cached, 0);

	KASSERT(kpreempt_disabled());
	KASSERT(VM_PAGEMD_PVLIST_LOCKED_P(mdpg));

	if (cached) {
		pmap_page_clear_attributes(mdpg, VM_PAGEMD_UNCACHED);
		PMAP_COUNT(page_cache_restorations);
	} else {
		pmap_page_set_attributes(mdpg, VM_PAGEMD_UNCACHED);
		PMAP_COUNT(page_cache_evictions);
	}

	for (pv_entry_t pv = &mdpg->mdpg_first; pv != NULL; pv = pv->pv_next) {
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = trunc_page(pv->pv_va);

		KASSERT(pmap != NULL);
		KASSERT(pmap != pmap_kernel() || !pmap_md_direct_mapped_vaddr_p(va));
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		if (ptep == NULL)
			continue;
		pt_entry_t pte = *ptep;
		if (pte_valid_p(pte)) {
			pte = pte_cached_change(pte, cached);
			pmap_md_tlb_miss_lock_enter();
			pte_set(ptep, pte);
			pmap_tlb_update_addr(pmap, va, pte, PMAP_TLB_NEED_IPI);
			pmap_md_tlb_miss_lock_exit();
		}
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}
#endif	/* PMAP_VIRTUAL_CACHE_ALIASES && !PMAP_NO_PV_UNCACHED */

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
	const bool wired = (flags & PMAP_WIRED) != 0;
	const bool is_kernel_pmap_p = (pmap == pmap_kernel());
	u_int update_flags = (flags & VM_PROT_ALL) != 0 ? PMAP_TLB_INSERT : 0;
#ifdef UVMHIST
	struct kern_history * const histp =
	    ((prot & VM_PROT_EXECUTE) ? &pmapexechist : &pmaphist);
#endif

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(*histp);
	UVMHIST_LOG(*histp, "(pmap=%#jx, va=%#jx, pa=%#jx",
	    (uintptr_t)pmap, va, pa, 0);
	UVMHIST_LOG(*histp, "prot=%#jx flags=%#jx)", prot, flags, 0, 0);

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
	pmap_addr_range_check(pmap, va, va, __func__);

	KASSERTMSG(prot & VM_PROT_READ, "no READ (%#x) in prot %#x",
	    VM_PROT_READ, prot);

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	struct vm_page_md * const mdpg = (pg ? VM_PAGE_TO_MD(pg) : NULL);

	if (pg) {
		/* Set page referenced/modified status based on flags */
		if (flags & VM_PROT_WRITE) {
			pmap_page_set_attributes(mdpg, VM_PAGEMD_MODIFIED|VM_PAGEMD_REFERENCED);
		} else if (flags & VM_PROT_ALL) {
			pmap_page_set_attributes(mdpg, VM_PAGEMD_REFERENCED);
		}

#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		if (!VM_PAGEMD_CACHED_P(mdpg)) {
			flags |= PMAP_NOCACHE;
			PMAP_COUNT(uncached_mappings);
		}
#endif

		PMAP_COUNT(managed_mappings);
	} else {
		/*
		 * Assumption: if it is not part of our managed memory
		 * then it must be device memory which may be volatile.
		 */
		if ((flags & PMAP_CACHE_MASK) == 0)
			flags |= PMAP_NOCACHE;
		PMAP_COUNT(unmanaged_mappings);
	}

	pt_entry_t npte = pte_make_enter(pa, mdpg, prot, flags,
	    is_kernel_pmap_p);

	kpreempt_disable();

	pt_entry_t * const ptep = pmap_pte_reserve(pmap, va, flags);
	if (__predict_false(ptep == NULL)) {
		kpreempt_enable();
		UVMHIST_LOG(*histp, " <-- ENOMEM", 0, 0, 0, 0);
		return ENOMEM;
	}
	const pt_entry_t opte = *ptep;
	const bool resident = pte_valid_p(opte);
	bool remap = false;
	if (resident) {
		if (pte_to_paddr(opte) != pa) {
			KASSERT(!is_kernel_pmap_p);
		    	const pt_entry_t rpte = pte_nv_entry(false);

			pmap_addr_range_check(pmap, va, va + NBPG, __func__);
			pmap_pte_process(pmap, va, va + NBPG, pmap_pte_remove,
			    rpte);
			PMAP_COUNT(user_mappings_changed);
			remap = true;
		}
		update_flags |= PMAP_TLB_NEED_IPI;
	}

	if (!resident || remap) {
		pmap->pm_stats.resident_count++;
	}

	/* Done after case that may sleep/return. */
	if (pg)
		pmap_enter_pv(pmap, va, pg, &npte, 0);

	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * MIPS pages in a MACH page.
	 */
	if (wired) {
		pmap->pm_stats.wired_count++;
		npte = pte_wire_entry(npte);
	}

	UVMHIST_LOG(*histp, "new pte %#jx (pa %#jx)",
	    pte_value(npte), pa, 0, 0);

	KASSERT(pte_valid_p(npte));

	pmap_md_tlb_miss_lock_enter();
	pte_set(ptep, npte);
	pmap_tlb_update_addr(pmap, va, npte, update_flags);
	pmap_md_tlb_miss_lock_exit();
	kpreempt_enable();

	if (pg != NULL && (prot == (VM_PROT_READ | VM_PROT_EXECUTE))) {
		KASSERT(mdpg != NULL);
		PMAP_COUNT(exec_mappings);
		if (!VM_PAGEMD_EXECPAGE_P(mdpg) && pte_cached_p(npte)) {
			if (!pte_deferred_exec_p(npte)) {
				UVMHIST_LOG(*histp, "va=%#jx pg %#jx: "
				    "immediate syncicache",
				    va, (uintptr_t)pg, 0, 0);
				pmap_page_syncicache(pg);
				pmap_page_set_attributes(mdpg,
				    VM_PAGEMD_EXECPAGE);
				PMAP_COUNT(exec_synced_mappings);
			} else {
				UVMHIST_LOG(*histp, "va=%#jx pg %#jx: defer "
				    "syncicache: pte %#jx",
				    va, (uintptr_t)pg, npte, 0);
			}
		} else {
			UVMHIST_LOG(*histp,
			    "va=%#jx pg %#jx: no syncicache cached %jd",
			    va, (uintptr_t)pg, pte_cached_p(npte), 0);
		}
	} else if (pg != NULL && (prot & VM_PROT_EXECUTE)) {
		KASSERT(mdpg != NULL);
		KASSERT(prot & VM_PROT_WRITE);
		PMAP_COUNT(exec_mappings);
		pmap_page_syncicache(pg);
		pmap_page_clear_attributes(mdpg, VM_PAGEMD_EXECPAGE);
		UVMHIST_LOG(*histp,
		    "va=%#jx pg %#jx: immediate syncicache (writeable)",
		    va, (uintptr_t)pg, 0, 0);
	}

	UVMHIST_LOG(*histp, " <-- 0 (OK)", 0, 0, 0, 0);
	return 0;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pmap_t pmap = pmap_kernel();
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	struct vm_page_md * const mdpg = (pg ? VM_PAGE_TO_MD(pg) : NULL);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(va=%#jx pa=%#jx prot=%ju, flags=%#jx)",
	    va, pa, prot, flags);
	PMAP_COUNT(kenter_pa);

	if (mdpg == NULL) {
		PMAP_COUNT(kenter_pa_unmanaged);
		if ((flags & PMAP_CACHE_MASK) == 0)
			flags |= PMAP_NOCACHE;
	} else {
		if ((flags & PMAP_NOCACHE) == 0 && !PMAP_PAGE_COLOROK_P(pa, va))
			PMAP_COUNT(kenter_pa_bad);
	}

	pt_entry_t npte = pte_make_kenter_pa(pa, mdpg, prot, flags);
	kpreempt_disable();
	pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
	KASSERTMSG(ptep != NULL, "%#"PRIxVADDR " %#"PRIxVADDR, va,
	    pmap_limits.virtual_end);
	KASSERT(!pte_valid_p(*ptep));

	/*
	 * No need to track non-managed pages or PMAP_KMPAGEs pages for aliases
	 */
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
	if (pg != NULL && (flags & PMAP_KMPAGE) == 0
	    && pmap_md_virtual_cache_aliasing_p()) {
		pmap_enter_pv(pmap, va, pg, &npte, PV_KENTER);
	}
#endif

	/*
	 * We have the option to force this mapping into the TLB but we
	 * don't.  Instead let the next reference to the page do it.
	 */
	pmap_md_tlb_miss_lock_enter();
	pte_set(ptep, npte);
	pmap_tlb_update_addr(pmap_kernel(), va, npte, 0);
	pmap_md_tlb_miss_lock_exit();
	kpreempt_enable();
#if DEBUG > 1
	for (u_int i = 0; i < PAGE_SIZE / sizeof(long); i++) {
		if (((long *)va)[i] != ((long *)pa)[i])
			panic("%s: contents (%lx) of va %#"PRIxVADDR
			    " != contents (%lx) of pa %#"PRIxPADDR, __func__,
			    ((long *)va)[i], va, ((long *)pa)[i], pa);
	}
#endif

	UVMHIST_LOG(pmaphist, " <-- done (ptep=%#jx)", (uintptr_t)ptep, 0, 0,
	    0);
}

/*
 *	Remove the given range of addresses from the kernel map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */

static bool
pmap_pte_kremove(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *ptep,
	uintptr_t flags)
{
	const pt_entry_t new_pte = pte_nv_entry(true);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist,
	    "(pmap=%#jx, sva=%#jx eva=%#jx ptep=%#jx)",
	    (uintptr_t)pmap, sva, eva, (uintptr_t)ptep);

	KASSERT(kpreempt_disabled());

	for (; sva < eva; sva += NBPG, ptep++) {
		pt_entry_t pte = *ptep;
		if (!pte_valid_p(pte))
			continue;

		PMAP_COUNT(kremove_pages);
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pte_to_paddr(pte));
		if (pg != NULL && pmap_md_virtual_cache_aliasing_p()) {
			pmap_remove_pv(pmap, sva, pg, !pte_readonly_p(pte));
		}
#endif

		pmap_md_tlb_miss_lock_enter();
		pte_set(ptep, new_pte);
		pmap_tlb_invalidate_addr(pmap, sva);
		pmap_md_tlb_miss_lock_exit();
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);

	return false;
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	const vaddr_t sva = trunc_page(va);
	const vaddr_t eva = round_page(va + len);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(va=%#jx len=%#jx)", va, len, 0, 0);

	kpreempt_disable();
	pmap_pte_process(pmap_kernel(), sva, eva, pmap_pte_kremove, 0);
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

void
pmap_remove_all(struct pmap *pmap)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pm=%#jx)", (uintptr_t)pmap, 0, 0, 0);

	KASSERT(pmap != pmap_kernel());

	kpreempt_disable();
	/*
	 * Free all of our ASIDs which means we can skip doing all the
	 * tlb_invalidate_addrs().
	 */
	pmap_md_tlb_miss_lock_enter();
#ifdef MULTIPROCESSOR
	// This should be the last CPU with this pmap onproc
	KASSERT(!kcpuset_isotherset(pmap->pm_onproc, cpu_index(curcpu())));
	if (kcpuset_isset(pmap->pm_onproc, cpu_index(curcpu())))
#endif
		pmap_tlb_asid_deactivate(pmap);
#ifdef MULTIPROCESSOR
	KASSERT(kcpuset_iszero(pmap->pm_onproc));
#endif
	pmap_tlb_asid_release_all(pmap);
	pmap_md_tlb_miss_lock_exit();
	pmap->pm_flags |= PMAP_DEFERRED_ACTIVATE;

#ifdef PMAP_FAULTINFO
	curpcb->pcb_faultinfo.pfi_faultaddr = 0;
	curpcb->pcb_faultinfo.pfi_repeats = 0;
	curpcb->pcb_faultinfo.pfi_faultpte = NULL;
#endif
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%#jx, va=%#jx)", (uintptr_t)pmap, va,
	    0, 0);
	PMAP_COUNT(unwire);

	/*
	 * Don't need to flush the TLB since PG_WIRED is only in software.
	 */
	kpreempt_disable();
	pmap_addr_range_check(pmap, va, va, __func__);
	pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
	KASSERTMSG(ptep != NULL, "pmap %p va %#"PRIxVADDR" invalid STE",
	    pmap, va);
	pt_entry_t pte = *ptep;
	KASSERTMSG(pte_valid_p(pte),
	    "pmap %p va %#"PRIxVADDR" invalid PTE %#"PRIxPTE" @ %p",
	    pmap, va, pte_value(pte), ptep);

	if (pte_wired_p(pte)) {
		pmap_md_tlb_miss_lock_enter();
		pte_set(ptep, pte_unwire_entry(pte));
		pmap_md_tlb_miss_lock_exit();
		pmap->pm_stats.wired_count--;
	}
#ifdef DIAGNOSTIC
	else {
		printf("%s: wiring for pmap %p va %#"PRIxVADDR" unchanged!\n",
		    __func__, pmap, va);
	}
#endif
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
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

	if (pmap == pmap_kernel()) {
		if (pmap_md_direct_mapped_vaddr_p(va)) {
			pa = pmap_md_direct_mapped_vaddr_to_paddr(va);
			goto done;
		}
		if (pmap_md_io_vaddr_p(va))
			panic("pmap_extract: io address %#"PRIxVADDR"", va);

		if (va >= pmap_limits.virtual_end)
			panic("%s: illegal kernel mapped address %#"PRIxVADDR,
			    __func__, va);
	}
	kpreempt_disable();
	const pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
	if (ptep == NULL || !pte_valid_p(*ptep)) {
		kpreempt_enable();
		return false;
	}
	pa = pte_to_paddr(*ptep) | (va & PGOFSET);
	kpreempt_enable();
done:
	if (pap != NULL) {
		*pap = pa;
	}
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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	PMAP_COUNT(copy);
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pg=%#jx (pa %#jx))",
	   (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), 0,0);

	bool rv = pmap_page_clear_attributes(mdpg, VM_PAGEMD_REFERENCED);

	UVMHIST_LOG(pmaphist, " <-- wasref %ju", rv, 0, 0, 0);

	return rv;
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
	return VM_PAGEMD_REFERENCED_P(VM_PAGE_TO_MD(pg));
}

/*
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv = &mdpg->mdpg_first;
	pv_entry_t pv_next;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pg=%#jx (%#jx))",
	    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), 0,0);
	PMAP_COUNT(clear_modify);

	if (VM_PAGEMD_EXECPAGE_P(mdpg)) {
		if (pv->pv_pmap == NULL) {
			UVMHIST_LOG(pmapexechist,
			    "pg %#jx (pa %#jx): execpage cleared",
			    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), 0, 0);
			pmap_page_clear_attributes(mdpg, VM_PAGEMD_EXECPAGE);
			PMAP_COUNT(exec_uncached_clear_modify);
		} else {
			UVMHIST_LOG(pmapexechist,
			    "pg %#jx (pa %#jx): syncicache performed",
			    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), 0, 0);
			pmap_page_syncicache(pg);
			PMAP_COUNT(exec_synced_clear_modify);
		}
	}
	if (!pmap_page_clear_attributes(mdpg, VM_PAGEMD_MODIFIED)) {
		UVMHIST_LOG(pmaphist, " <-- false", 0, 0, 0, 0);
		return false;
	}
	if (pv->pv_pmap == NULL) {
		UVMHIST_LOG(pmaphist, " <-- true (no mappings)", 0, 0, 0, 0);
		return true;
	}

	/*
	 * remove write access from any pages that are dirty
	 * so we can tell if they are written to again later.
	 * flush the VAC first if there is one.
	 */
	kpreempt_disable();
	KASSERT(!VM_PAGEMD_PVLIST_LOCKED_P(mdpg));
	VM_PAGEMD_PVLIST_READLOCK(mdpg);
	pmap_pvlist_check(mdpg);
	for (; pv != NULL; pv = pv_next) {
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = trunc_page(pv->pv_va);

		pv_next = pv->pv_next;
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		if (pv->pv_va & PV_KENTER)
			continue;
#endif
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		KASSERT(ptep);
		pt_entry_t pte = pte_prot_nowrite(*ptep);
		if (*ptep == pte) {
			continue;
		}
		KASSERT(pte_valid_p(pte));
		const uintptr_t gen = VM_PAGEMD_PVLIST_UNLOCK(mdpg);
		pmap_md_tlb_miss_lock_enter();
		pte_set(ptep, pte);
		pmap_tlb_invalidate_addr(pmap, va);
		pmap_md_tlb_miss_lock_exit();
		pmap_update(pmap);
		if (__predict_false(gen != VM_PAGEMD_PVLIST_READLOCK(mdpg))) {
			/*
			 * The list changed!  So restart from the beginning.
			 */
			pv_next = &mdpg->mdpg_first;
			pmap_pvlist_check(mdpg);
		}
	}
	pmap_pvlist_check(mdpg);
	VM_PAGEMD_PVLIST_UNLOCK(mdpg);
	kpreempt_enable();

	UVMHIST_LOG(pmaphist, " <-- true (mappings changed)", 0, 0, 0, 0);
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
	return VM_PAGEMD_MODIFIED_P(VM_PAGE_TO_MD(pg));
}

/*
 *	pmap_set_modified:
 *
 *	Sets the page modified reference bit for the specified page.
 */
void
pmap_set_modified(paddr_t pa)
{
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pmap_page_set_attributes(mdpg, VM_PAGEMD_MODIFIED|VM_PAGEMD_REFERENCED);
}

/******************** pv_entry management ********************/

static void
pmap_pvlist_check(struct vm_page_md *mdpg)
{
#ifdef DEBUG
	pv_entry_t pv = &mdpg->mdpg_first;
	if (pv->pv_pmap != NULL) {
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		const u_int colormask = uvmexp.colormask;
		u_int colors = 0;
#endif
		for (; pv != NULL; pv = pv->pv_next) {
			KASSERT(pv->pv_pmap != pmap_kernel() || !pmap_md_direct_mapped_vaddr_p(pv->pv_va));
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
			colors |= __BIT(atop(pv->pv_va) & colormask);
#endif
		}
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		// Assert that if there is more than 1 color mapped, that the
		// page is uncached.
		KASSERTMSG(!pmap_md_virtual_cache_aliasing_p()
		    || colors == 0 || (colors & (colors-1)) == 0
		    || VM_PAGEMD_UNCACHED_P(mdpg), "colors=%#x uncached=%u",
		    colors, VM_PAGEMD_UNCACHED_P(mdpg));
#endif
	} else {
    		KASSERT(pv->pv_next == NULL);
	}
#endif /* DEBUG */
}

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
pmap_enter_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, pt_entry_t *nptep,
    u_int flags)
{
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, npv, apv;
#ifdef UVMHIST
	bool first = false;
#endif

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist,
	    "(pmap=%#jx va=%#jx pg=%#jx (%#jx)",
	    (uintptr_t)pmap, va, (uintptr_t)pg, VM_PAGE_TO_PHYS(pg));
	UVMHIST_LOG(pmaphist, "nptep=%#jx (%#jx))",
	    (uintptr_t)nptep, pte_value(*nptep), 0, 0);

	KASSERT(kpreempt_disabled());
	KASSERT(pmap != pmap_kernel() || !pmap_md_direct_mapped_vaddr_p(va));
	KASSERTMSG(pmap != pmap_kernel() || !pmap_md_io_vaddr_p(va),
	    "va %#"PRIxVADDR, va);

	apv = NULL;
	VM_PAGEMD_PVLIST_LOCK(mdpg);
again:
	pv = &mdpg->mdpg_first;
	pmap_pvlist_check(mdpg);
	if (pv->pv_pmap == NULL) {
		KASSERT(pv->pv_next == NULL);
		/*
		 * No entries yet, use header as the first entry
		 */
		PMAP_COUNT(primary_mappings);
		PMAP_COUNT(mappings);
#ifdef UVMHIST
		first = true;
#endif
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		KASSERT(VM_PAGEMD_CACHED_P(mdpg));
		// If the new mapping has an incompatible color the last
		// mapping of this page, clean the page before using it.
		if (!PMAP_PAGE_COLOROK_P(va, pv->pv_va)) {
			pmap_md_vca_clean(pg, PMAP_WBINV);
		}
#endif
		pv->pv_pmap = pmap;
		pv->pv_va = va | flags;
	} else {
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		if (pmap_md_vca_add(pg, va, nptep)) {
			goto again;
		}
#endif

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
			if (pmap == npv->pv_pmap
			    && va == trunc_page(npv->pv_va)) {
#ifdef PARANOIADIAG
				pt_entry_t *ptep = pmap_pte_lookup(pmap, va);
				pt_entry_t pte = (ptep != NULL) ? *ptep : 0;
				if (!pte_valid_p(pte) || pte_to_paddr(pte) != pa)
					printf("%s: found va %#"PRIxVADDR
					    " pa %#"PRIxPADDR
					    " in pv_table but != %#"PRIxPTE"\n",
					    __func__, va, pa, pte_value(pte));
#endif
				PMAP_COUNT(remappings);
				VM_PAGEMD_PVLIST_UNLOCK(mdpg);
				if (__predict_false(apv != NULL))
					pmap_pv_free(apv);

				UVMHIST_LOG(pmaphist,
				    " <-- done pv=%#jx (reused)",
				    (uintptr_t)pv, 0, 0, 0);
				return;
			}
		}
		if (__predict_true(apv == NULL)) {
			/*
			 * To allocate a PV, we have to release the PVLIST lock
			 * so get the page generation.  We allocate the PV, and
			 * then reacquire the lock.
			 */
			pmap_pvlist_check(mdpg);
			const uintptr_t gen = VM_PAGEMD_PVLIST_UNLOCK(mdpg);

			apv = (pv_entry_t)pmap_pv_alloc();
			if (apv == NULL)
				panic("pmap_enter_pv: pmap_pv_alloc() failed");

			/*
			 * If the generation has changed, then someone else
			 * tinkered with this page so we should start over.
			 */
			if (gen != VM_PAGEMD_PVLIST_LOCK(mdpg))
				goto again;
		}
		npv = apv;
		apv = NULL;
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
		/*
		 * If need to deal with virtual cache aliases, keep mappings
		 * in the kernel pmap at the head of the list.  This allows
		 * the VCA code to easily use them for cache operations if
		 * present.
		 */
		pmap_t kpmap = pmap_kernel();
		if (pmap != kpmap) {
			while (pv->pv_pmap == kpmap && pv->pv_next != NULL) {
				pv = pv->pv_next;
			}
		}
#endif
		npv->pv_va = va | flags;
		npv->pv_pmap = pmap;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
		PMAP_COUNT(mappings);
	}
	pmap_pvlist_check(mdpg);
	VM_PAGEMD_PVLIST_UNLOCK(mdpg);
	if (__predict_false(apv != NULL))
		pmap_pv_free(apv);

	UVMHIST_LOG(pmaphist, " <-- done pv=%#jx (first %ju)", (uintptr_t)pv,
	    first, 0, 0);
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
	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, npv;
	bool last;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist,
	    "(pmap=%#jx, va=%#jx, pg=%#jx (pa %#jx)",
	    (uintptr_t)pmap, va, (uintptr_t)pg, VM_PAGE_TO_PHYS(pg));
	UVMHIST_LOG(pmaphist, "dirty=%ju)", dirty, 0, 0, 0);

	KASSERT(kpreempt_disabled());
	KASSERT((va & PAGE_MASK) == 0);
	pv = &mdpg->mdpg_first;

	VM_PAGEMD_PVLIST_LOCK(mdpg);
	pmap_pvlist_check(mdpg);

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	last = false;
	if (pmap == pv->pv_pmap && va == trunc_page(pv->pv_va)) {
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			KASSERT(pv->pv_pmap != NULL);
		} else {
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
			pmap_page_clear_attributes(mdpg, VM_PAGEMD_UNCACHED);
#endif
			pv->pv_pmap = NULL;
			last = true;	/* Last mapping removed */
		}
		PMAP_COUNT(remove_pvfirst);
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
			PMAP_COUNT(remove_pvsearch);
			if (pmap == npv->pv_pmap && va == trunc_page(npv->pv_va))
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
		}
	}

	pmap_pvlist_check(mdpg);
	VM_PAGEMD_PVLIST_UNLOCK(mdpg);

#ifdef PMAP_VIRTUAL_CACHE_ALIASES
	pmap_md_vca_remove(pg, va, dirty, last);
#endif

	/*
	 * Free the pv_entry if needed.
	 */
	if (npv)
		pmap_pv_free(npv);
	if (VM_PAGEMD_EXECPAGE_P(mdpg) && dirty) {
		if (last) {
			/*
			 * If this was the page's last mapping, we no longer
			 * care about its execness.
			 */
			UVMHIST_LOG(pmapexechist,
			    "pg %#jx (pa %#jx)last %ju: execpage cleared",
			    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), last, 0);
			pmap_page_clear_attributes(mdpg, VM_PAGEMD_EXECPAGE);
			PMAP_COUNT(exec_uncached_remove);
		} else {
			/*
			 * Someone still has it mapped as an executable page
			 * so we must sync it.
			 */
			UVMHIST_LOG(pmapexechist,
			    "pg %#jx (pa %#jx) last %ju: performed syncicache",
			    (uintptr_t)pg, VM_PAGE_TO_PHYS(pg), last, 0);
			pmap_page_syncicache(pg);
			PMAP_COUNT(exec_synced_remove);
		}
	}

	UVMHIST_LOG(pmaphist, " <-- done", 0, 0, 0, 0);
}

#if defined(MULTIPROCESSOR)
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
		mutex_init(lock, MUTEX_DEFAULT, IPL_HIGH);
		pli->pli_locks[i] = lock;
	}
	pli->pli_lock_mask = nlocks - 1;
}

kmutex_t *
pmap_pvlist_lock_addr(struct vm_page_md *mdpg)
{
	struct pmap_pvlist_info * const pli = &pmap_pvlist_info;
	kmutex_t *lock = mdpg->mdpg_lock;

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
		lock = atomic_cas_ptr(&mdpg->mdpg_lock, NULL, new_lock);
		if (lock == NULL) {
			lock = new_lock;
			atomic_inc_uint(&pli->pli_lock_refs[lockid]);
		}
	}

	/*
	 * Now finally provide the lock.
	 */
	return lock;
}
#else /* !MULTIPROCESSOR */
void
pmap_pvlist_lock_init(size_t cache_line_size)
{
	mutex_init(&pmap_pvlist_mutex, MUTEX_DEFAULT, IPL_HIGH);
}

#ifdef MODULAR
kmutex_t *
pmap_pvlist_lock_addr(struct vm_page_md *mdpg)
{
	/*
	 * We just use a global lock.
	 */
	if (__predict_false(mdpg->mdpg_lock == NULL)) {
		mdpg->mdpg_lock = &pmap_pvlist_mutex;
	}

	/*
	 * Now finally provide the lock.
	 */
	return mdpg->mdpg_lock;
}
#endif /* MODULAR */
#endif /* !MULTIPROCESSOR */

/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	struct vm_page * const pg = PMAP_ALLOC_POOLPAGE(UVM_PGA_USERESERVE);
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
	KASSERT(pg != NULL);
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
	kpreempt_disable();
	pmap_md_vca_remove(pg, va, true, true);
	kpreempt_enable();
#endif
	pmap_page_clear_attributes(VM_PAGE_TO_MD(pg), VM_PAGEMD_POOLPAGE);
	KASSERT(!VM_PAGEMD_EXECPAGE_P(VM_PAGE_TO_MD(pg)));
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
	vsize_t prefer_mask = ptoa(uvmexp.colormask);

	PMAP_COUNT(prefer_requests);

	prefer_mask |= pmap_md_cache_prefer_mask();

	if (prefer_mask) {
		vaddr_t	va = *vap;
		vsize_t d = (foff - va) & prefer_mask;
		if (d) {
			if (td)
				*vap = trunc_page(va - ((-d) & prefer_mask));
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

	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	KASSERT(!VM_PAGEMD_EXECPAGE_P(mdpg));

	pmap_page_set_attributes(mdpg, VM_PAGEMD_POOLPAGE);

	return pmap_md_map_poolpage(pa, NBPG);
}

paddr_t
pmap_unmap_poolpage(vaddr_t va)
{
	KASSERT(pmap_md_direct_mapped_vaddr_p(va));
	paddr_t pa = pmap_md_direct_mapped_vaddr_to_paddr(va);

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg != NULL);
	KASSERT(!VM_PAGEMD_EXECPAGE_P(VM_PAGE_TO_MD(pg)));

	pmap_page_clear_attributes(VM_PAGE_TO_MD(pg), VM_PAGEMD_POOLPAGE);
	pmap_md_unmap_poolpage(va, NBPG);

	return pa;
}
#endif /* PMAP_MAP_POOLPAGE */
