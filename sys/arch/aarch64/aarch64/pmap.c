/*	$NetBSD: pmap.c,v 1.135 2022/04/17 15:20:36 skrll Exp $	*/

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.135 2022/04/17 15:20:36 skrll Exp $");

#include "opt_arm_debug.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_efi.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_pmap.h"
#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/types.h>

#include <sys/asan.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <uvm/uvm.h>
#include <uvm/pmap/pmap_pvt.h>

#include <arm/cpufunc.h>

#include <aarch64/pmap.h>
#include <aarch64/pte.h>
#include <aarch64/armreg.h>
#include <aarch64/locore.h>
#include <aarch64/machdep.h>
#ifdef DDB
#include <aarch64/db_machdep.h>
#include <ddb/db_access.h>
#endif

#include <arm/cpufunc.h>

//#define PMAP_PV_DEBUG

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

#ifdef UVMHIST

#ifndef UVMHIST_PMAPHIST_SIZE
#define UVMHIST_PMAPHIST_SIZE	(1024 * 4)
#endif

struct kern_history_ent pmaphistbuf[UVMHIST_PMAPHIST_SIZE];
UVMHIST_DEFINE(pmaphist) = UVMHIST_INITIALIZER(pmaphist, pmaphistbuf);

static void
pmap_hist_init(void)
{
	static bool inited = false;
	if (inited == false) {
		UVMHIST_LINK_STATIC(pmaphist);
		inited = true;
	}
}
#define PMAP_HIST_INIT()	pmap_hist_init()

#else /* UVMHIST */

#define PMAP_HIST_INIT()	((void)0)

#endif /* UVMHIST */


#ifdef PMAPCOUNTERS
#define PMAP_COUNT(name)		(pmap_evcnt_##name.ev_count++ + 0)
#define PMAP_COUNTER(name, desc)					\
	struct evcnt pmap_evcnt_##name =				\
	    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", desc);	\
	EVCNT_ATTACH_STATIC(pmap_evcnt_##name)

PMAP_COUNTER(pdp_alloc_boot, "page table page allocate (uvm_pageboot_alloc)");
PMAP_COUNTER(pdp_alloc, "page table page allocate (uvm_pagealloc)");
PMAP_COUNTER(pdp_free, "page table page free (uvm_pagefree)");

PMAP_COUNTER(pv_enter, "pv_entry fill");
PMAP_COUNTER(pv_remove_dyn, "pv_entry free and unlink dynamic");
PMAP_COUNTER(pv_remove_emb, "pv_entry clear embedded");
PMAP_COUNTER(pv_remove_nopv, "no pv_entry found when removing pv");

PMAP_COUNTER(activate, "pmap_activate call");
PMAP_COUNTER(deactivate, "pmap_deactivate call");
PMAP_COUNTER(create, "pmap_create call");
PMAP_COUNTER(destroy, "pmap_destroy call");

PMAP_COUNTER(page_protect, "pmap_page_protect call");
PMAP_COUNTER(protect, "pmap_protect call");
PMAP_COUNTER(protect_remove_fallback, "pmap_protect with no-read");
PMAP_COUNTER(protect_none, "pmap_protect non-exists pages");
PMAP_COUNTER(protect_managed, "pmap_protect managed pages");
PMAP_COUNTER(protect_unmanaged, "pmap_protect unmanaged pages");
PMAP_COUNTER(protect_pvmanaged, "pmap_protect pv-tracked unmanaged pages");

PMAP_COUNTER(clear_modify, "pmap_clear_modify call");
PMAP_COUNTER(clear_modify_pages, "pmap_clear_modify pages");
PMAP_COUNTER(clear_reference, "pmap_clear_reference call");
PMAP_COUNTER(clear_reference_pages, "pmap_clear_reference pages");

PMAP_COUNTER(fixup_referenced, "page reference emulations");
PMAP_COUNTER(fixup_modified, "page modification emulations");

PMAP_COUNTER(kern_mappings_bad, "kernel pages mapped (bad color)");
PMAP_COUNTER(kern_mappings_bad_wired, "kernel pages mapped (wired bad color)");
PMAP_COUNTER(user_mappings_bad, "user pages mapped (bad color, not wired)");
PMAP_COUNTER(user_mappings_bad_wired, "user pages mapped (bad color, wired)");
PMAP_COUNTER(kern_mappings, "kernel pages mapped");
PMAP_COUNTER(user_mappings, "user pages mapped");
PMAP_COUNTER(user_mappings_changed, "user mapping changed");
PMAP_COUNTER(kern_mappings_changed, "kernel mapping changed");
PMAP_COUNTER(uncached_mappings, "uncached pages mapped");
PMAP_COUNTER(unmanaged_mappings, "unmanaged pages mapped");
PMAP_COUNTER(pvmanaged_mappings, "pv-tracked unmanaged pages mapped");
PMAP_COUNTER(managed_mappings, "managed pages mapped");
PMAP_COUNTER(mappings, "pages mapped (including remapped)");
PMAP_COUNTER(remappings, "pages remapped");

PMAP_COUNTER(pv_entry_cannotalloc, "pv_entry allocation failure");

PMAP_COUNTER(unwire, "pmap_unwire call");
PMAP_COUNTER(unwire_failure, "pmap_unwire failure");

#else /* PMAPCOUNTERS */
#define PMAP_COUNT(name)		__nothing
#endif /* PMAPCOUNTERS */

/*
 * invalidate TLB entry for ASID and VA.
 */
#define AARCH64_TLBI_BY_ASID_VA(asid, va)			\
	do {							\
		if ((asid) == 0)				\
			aarch64_tlbi_by_va((va));		\
		else						\
			aarch64_tlbi_by_asid_va((asid), (va));	\
	} while (0/*CONSTCOND*/)

/*
 * require access permission in pte to invalidate instruction cache.
 * change the pte to be accessible temporarily before cpu_icache_sync_range().
 * this macro modifies PTE (*ptep). need to update PTE after this.
 */
#define PTE_ICACHE_SYNC_PAGE(pte, ptep, asid, va)			\
	do {								\
		atomic_swap_64((ptep), (pte) | LX_BLKPAG_AF);		\
		AARCH64_TLBI_BY_ASID_VA((asid), (va));			\
		cpu_icache_sync_range((va), PAGE_SIZE);			\
	} while (0/*CONSTCOND*/)

#define VM_PAGE_TO_PP(pg)	(&(pg)->mdpage.mdpg_pp)

#define L3INDEXMASK	(L3_SIZE * Ln_ENTRIES - 1)
#define PDPSWEEP_TRIGGER	512

static pt_entry_t *_pmap_pte_lookup_l3(struct pmap *, vaddr_t);
static pt_entry_t *_pmap_pte_lookup_bs(struct pmap *, vaddr_t, vsize_t *);
static pt_entry_t _pmap_pte_adjust_prot(pt_entry_t, vm_prot_t, vm_prot_t, bool);
static pt_entry_t _pmap_pte_adjust_cacheflags(pt_entry_t, u_int);
static void _pmap_remove(struct pmap *, vaddr_t, vaddr_t, bool,
    struct pv_entry **);
static int _pmap_enter(struct pmap *, vaddr_t, paddr_t, vm_prot_t, u_int, bool);
static int _pmap_get_pdp(struct pmap *, vaddr_t, bool, int, paddr_t *,
    struct vm_page **);

static struct pmap kernel_pmap __cacheline_aligned;
static struct pmap efirt_pmap __cacheline_aligned;

struct pmap * const kernel_pmap_ptr = &kernel_pmap;

pmap_t
pmap_efirt(void)
{
	return &efirt_pmap;
}

static vaddr_t pmap_maxkvaddr;

vaddr_t virtual_avail, virtual_end;
vaddr_t virtual_devmap_addr;
bool pmap_devmap_bootstrap_done = false;

static struct pool_cache _pmap_cache;
static struct pool_cache _pmap_pv_pool;

/* Set to LX_BLKPAG_GP if supported. */
uint64_t pmap_attr_gp = 0;

static inline void
pmap_pv_lock(struct pmap_page *pp)
{

	mutex_enter(&pp->pp_pvlock);
}

static inline void
pmap_pv_unlock(struct pmap_page *pp)
{

	mutex_exit(&pp->pp_pvlock);
}


static inline void
pm_lock(struct pmap *pm)
{
	mutex_enter(&pm->pm_lock);
}

static inline void
pm_unlock(struct pmap *pm)
{
	mutex_exit(&pm->pm_lock);
}

static bool
pm_reverse_lock(struct pmap *pm, struct pmap_page *pp)
{

	KASSERT(mutex_owned(&pp->pp_pvlock));

	if (__predict_true(mutex_tryenter(&pm->pm_lock)))
		return true;

	if (pm != pmap_kernel())
		pmap_reference(pm);
	mutex_exit(&pp->pp_pvlock);
	mutex_enter(&pm->pm_lock);
	/* nothing, just wait for lock */
	mutex_exit(&pm->pm_lock);
	if (pm != pmap_kernel())
		pmap_destroy(pm);
	mutex_enter(&pp->pp_pvlock);
	return false;
}

static inline struct pmap_page *
phys_to_pp(paddr_t pa)
{
	struct vm_page *pg;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg != NULL)
		return VM_PAGE_TO_PP(pg);

#ifdef __HAVE_PMAP_PV_TRACK
	return pmap_pv_tracked(pa);
#else
	return NULL;
#endif /* __HAVE_PMAP_PV_TRACK */
}

#define IN_RANGE(va, sta, end)	(((sta) <= (va)) && ((va) < (end)))

#define IN_DIRECTMAP_ADDR(va)	\
	IN_RANGE((va), AARCH64_DIRECTMAP_START, AARCH64_DIRECTMAP_END)

#define	PMAP_EFIVA_P(va) \
     IN_RANGE((va), EFI_RUNTIME_VA, EFI_RUNTIME_VA + EFI_RUNTIME_SIZE)

#ifdef MODULAR
#define IN_MODULE_VA(va)	IN_RANGE((va), module_start, module_end)
#else
#define IN_MODULE_VA(va)	false
#endif

#ifdef DIAGNOSTIC

#define KERNEL_ADDR_P(va)						\
    (IN_RANGE((va), VM_MIN_KERNEL_ADDRESS,  VM_MAX_KERNEL_ADDRESS) ||	\
     PMAP_EFIVA_P(va))

#define KASSERT_PM_ADDR(pm, va)						\
    do {								\
	int space = aarch64_addressspace(va);				\
	if ((pm) == pmap_kernel()) {					\
		KASSERTMSG(space == AARCH64_ADDRSPACE_UPPER,		\
		    "%s: kernel pm %p: va=%016lx"			\
		    " is out of upper address space",			\
		    __func__, (pm), (va));				\
		KASSERTMSG(KERNEL_ADDR_P(va),				\
		    "%s: kernel pm %p: va=%016lx"			\
		    " is not kernel address",				\
		    __func__, (pm), (va));				\
	} else {							\
		KASSERTMSG(space == AARCH64_ADDRSPACE_LOWER,		\
		    "%s: user pm %p: va=%016lx"				\
		    " is out of lower address space",			\
		    __func__, (pm), (va));				\
		KASSERTMSG(IN_RANGE((va),				\
		    VM_MIN_ADDRESS, VM_MAX_ADDRESS),			\
		    "%s: user pm %p: va=%016lx"				\
		    " is not user address",				\
		    __func__, (pm), (va));				\
	}								\
    } while (0 /* CONSTCOND */)
#else /* DIAGNOSTIC */
#define KASSERT_PM_ADDR(pm,va)
#endif /* DIAGNOSTIC */


static const struct pmap_devmap *pmap_devmap_table;

static vsize_t
pmap_map_chunk(vaddr_t va, paddr_t pa, vsize_t size,
    vm_prot_t prot, u_int flags)
{
	pt_entry_t attr;
	vsize_t resid = round_page(size);

	attr = _pmap_pte_adjust_prot(0, prot, VM_PROT_ALL, false);
	attr = _pmap_pte_adjust_cacheflags(attr, flags);
	pmapboot_enter_range(va, pa, resid, attr, printf);

	return resid;
}

void
pmap_devmap_register(const struct pmap_devmap *table)
{
	pmap_devmap_table = table;
}

void
pmap_devmap_bootstrap(vaddr_t l0pt, const struct pmap_devmap *table)
{
	bool done = false;
	vaddr_t va;
	int i;

	pmap_devmap_register(table);

	VPRINTF("%s:\n", __func__);
	for (i = 0; table[i].pd_size != 0; i++) {
		VPRINTF(" devmap: pa %08lx-%08lx = va %016lx\n",
		    table[i].pd_pa,
		    table[i].pd_pa + table[i].pd_size - 1,
		    table[i].pd_va);
		va = table[i].pd_va;

		KASSERT((VM_KERNEL_IO_ADDRESS <= va) &&
		    (va < (VM_KERNEL_IO_ADDRESS + VM_KERNEL_IO_SIZE)));

		/* update and check virtual_devmap_addr */
		if (virtual_devmap_addr == 0 || virtual_devmap_addr > va) {
			virtual_devmap_addr = va;
		}

		pmap_map_chunk(
		    table[i].pd_va,
		    table[i].pd_pa,
		    table[i].pd_size,
		    table[i].pd_prot,
		    table[i].pd_flags);
		done = true;
	}
	if (done)
		pmap_devmap_bootstrap_done = true;
}

const struct pmap_devmap *
pmap_devmap_find_va(vaddr_t va, vsize_t size)
{
	paddr_t endva;
	int i;

	if (pmap_devmap_table == NULL)
		return NULL;

	endva = va + size;
	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if ((va >= pmap_devmap_table[i].pd_va) &&
		    (endva <= pmap_devmap_table[i].pd_va +
		              pmap_devmap_table[i].pd_size))
			return &pmap_devmap_table[i];
	}
	return NULL;
}

const struct pmap_devmap *
pmap_devmap_find_pa(paddr_t pa, psize_t size)
{
	paddr_t endpa;
	int i;

	if (pmap_devmap_table == NULL)
		return NULL;

	endpa = pa + size;
	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if (pa >= pmap_devmap_table[i].pd_pa &&
		    (endpa <= pmap_devmap_table[i].pd_pa +
		             pmap_devmap_table[i].pd_size))
			return (&pmap_devmap_table[i]);
	}
	return NULL;
}

vaddr_t
pmap_devmap_phystov(paddr_t pa)
{
	const struct pmap_devmap *table;
	paddr_t offset;

	table = pmap_devmap_find_pa(pa, 0);
	if (table == NULL)
		return 0;

	offset = pa - table->pd_pa;
	return table->pd_va + offset;
}

vaddr_t
pmap_devmap_vtophys(paddr_t va)
{
	const struct pmap_devmap *table;
	vaddr_t offset;

	table = pmap_devmap_find_va(va, 0);
	if (table == NULL)
		return 0;

	offset = va - table->pd_va;
	return table->pd_pa + offset;
}

void
pmap_bootstrap(vaddr_t vstart, vaddr_t vend)
{
	struct pmap *kpm;
	pd_entry_t *l0;
	paddr_t l0pa;

	PMAP_HIST_INIT();	/* init once */

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLED(pmaphist);

	uvmexp.ncolors = aarch64_cache_vindexsize / PAGE_SIZE;

	/* devmap already uses last of va? */
	if (virtual_devmap_addr != 0 && virtual_devmap_addr < vend)
		vend = virtual_devmap_addr;

	virtual_avail = vstart;
	virtual_end = vend;
	pmap_maxkvaddr = vstart;

	l0pa = reg_ttbr1_el1_read();
	l0 = (void *)AARCH64_PA_TO_KVA(l0pa);

	pmap_tlb_info_init(&pmap_tlb0_info);

	memset(&kernel_pmap, 0, sizeof(kernel_pmap));

	kpm = pmap_kernel();
	struct pmap_asid_info * const pai = PMAP_PAI(kpm, cpu_tlb_info(ci));

	pai->pai_asid = KERNEL_PID;
	kpm->pm_refcnt = 1;
	kpm->pm_idlepdp = 0;
	kpm->pm_l0table = l0;
	kpm->pm_l0table_pa = l0pa;
	kpm->pm_onproc = kcpuset_running;
	kpm->pm_active = kcpuset_running;
	kpm->pm_activated = true;
	LIST_INIT(&kpm->pm_vmlist);
	LIST_INIT(&kpm->pm_pvlist);	/* not used for kernel pmap */
	mutex_init(&kpm->pm_lock, MUTEX_DEFAULT, IPL_NONE);

	CTASSERT(sizeof(kpm->pm_stats.wired_count) == sizeof(long));
	CTASSERT(sizeof(kpm->pm_stats.resident_count) == sizeof(long));

#if defined(EFI_RUNTIME)
	memset(&efirt_pmap, 0, sizeof(efirt_pmap));
	struct pmap * const efipm = &efirt_pmap;
	struct pmap_asid_info * const efipai = PMAP_PAI(efipm, cpu_tlb_info(ci));

	efipai->pai_asid = KERNEL_PID;
	efipm->pm_refcnt = 1;

	vaddr_t efi_l0va = uvm_pageboot_alloc(Ln_TABLE_SIZE);
	KASSERT((efi_l0va & PAGE_MASK) == 0);

	efipm->pm_l0table = (pd_entry_t *)efi_l0va;
	memset(efipm->pm_l0table, 0, Ln_TABLE_SIZE);

	efipm->pm_l0table_pa = AARCH64_KVA_TO_PA(efi_l0va);

	efipm->pm_activated = false;
	LIST_INIT(&efipm->pm_vmlist);
	LIST_INIT(&efipm->pm_pvlist);	/* not used for efi pmap */
	mutex_init(&efipm->pm_lock, MUTEX_DEFAULT, IPL_NONE);
#endif
}

#ifdef MULTIPROCESSOR
void
pmap_md_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
	/* nothing */
}
#endif /* MULTIPROCESSOR */

static inline void
_pmap_adj_wired_count(struct pmap *pm, int adj)
{

	if (pm == pmap_kernel()) {
		atomic_add_long(&pm->pm_stats.wired_count, adj);
	} else {
		KASSERT(mutex_owned(&pm->pm_lock));
		pm->pm_stats.wired_count += adj;
	}
}

static inline void
_pmap_adj_resident_count(struct pmap *pm, int adj)
{

	if (pm == pmap_kernel()) {
		atomic_add_long(&pm->pm_stats.resident_count, adj);
	} else {
		KASSERT(mutex_owned(&pm->pm_lock));
		pm->pm_stats.resident_count += adj;
	}
}

inline static int
_pmap_color(vaddr_t addr)	/* or paddr_t */
{
	return (addr >> PGSHIFT) & (uvmexp.ncolors - 1);
}

static int
_pmap_pmap_ctor(void *arg, void *v, int flags)
{
	memset(v, 0, sizeof(struct pmap));
	return 0;
}

static int
_pmap_pv_ctor(void *arg, void *v, int flags)
{
	memset(v, 0, sizeof(struct pv_entry));
	return 0;
}

pd_entry_t *
pmap_l0table(struct pmap *pm)
{

	return pm->pm_l0table;
}

void
pmap_init(void)
{

	pool_cache_bootstrap(&_pmap_cache, sizeof(struct pmap),
	    coherency_unit, 0, 0, "pmappl", NULL, IPL_NONE, _pmap_pmap_ctor,
	    NULL, NULL);

	pool_cache_bootstrap(&_pmap_pv_pool, sizeof(struct pv_entry),
	    32, 0, PR_LARGECACHE, "pvpl", NULL, IPL_NONE, _pmap_pv_ctor,
	    NULL, NULL);

	pmap_tlb_info_evcnt_attach(&pmap_tlb0_info);
}

void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = virtual_avail;
	*vendp = virtual_end;
}

vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	int npage;
	paddr_t pa;
	vaddr_t va;
	psize_t bank_npage;
	uvm_physseg_t bank;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "size=%llu, *vstartp=%llx, *vendp=%llx",
	    size, *vstartp, *vendp, 0);

	size = round_page(size);
	npage = atop(size);

	for (bank = uvm_physseg_get_first(); uvm_physseg_valid_p(bank);
	    bank = uvm_physseg_get_next(bank)) {

		bank_npage = uvm_physseg_get_avail_end(bank) -
		    uvm_physseg_get_avail_start(bank);
		if (npage <= bank_npage)
			break;
	}

	if (!uvm_physseg_valid_p(bank)) {
		panic("%s: no memory", __func__);
	}

	/* Steal pages */
	pa = ptoa(uvm_physseg_get_avail_start(bank));
	va = AARCH64_PA_TO_KVA(pa);
	uvm_physseg_unplug(atop(pa), npage);

	for (; npage > 0; npage--, pa += PAGE_SIZE)
		pmap_zero_page(pa);

	return va;
}

void
pmap_reference(struct pmap *pm)
{
	atomic_inc_uint(&pm->pm_refcnt);
}

static paddr_t
pmap_alloc_pdp(struct pmap *pm, struct vm_page **pgp, int flags, bool waitok)
{
	paddr_t pa;
	struct vm_page *pg;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, flags=%08x, waitok=%d",
	    pm, flags, waitok, 0);

	if (uvm.page_init_done) {
		int aflags = ((flags & PMAP_CANFAIL) ? 0 : UVM_PGA_USERESERVE) |
		    UVM_PGA_ZERO;
 retry:
		pg = uvm_pagealloc(NULL, 0, NULL, aflags);
		if (pg == NULL) {
			if (waitok) {
				uvm_wait("pmap_alloc_pdp");
				goto retry;
			}
			return POOL_PADDR_INVALID;
		}

		LIST_INSERT_HEAD(&pm->pm_vmlist, pg, pageq.list);
		pg->flags &= ~PG_BUSY;	/* never busy */
		pg->wire_count = 1;	/* max = 1 + Ln_ENTRIES = 513 */
		pa = VM_PAGE_TO_PHYS(pg);
		PMAP_COUNT(pdp_alloc);
		PMAP_PAGE_INIT(VM_PAGE_TO_PP(pg));
	} else {
		/* uvm_pageboot_alloc() returns a direct mapping address */
		pg = NULL;
		pa = AARCH64_KVA_TO_PA(
		    uvm_pageboot_alloc(Ln_TABLE_SIZE));
		PMAP_COUNT(pdp_alloc_boot);
	}
	if (pgp != NULL)
		*pgp = pg;

	UVMHIST_LOG(pmaphist, "pa=%llx, pg=%llx",
	    pa, pg, 0, 0);

	return pa;
}

static void
pmap_free_pdp(struct pmap *pm, struct vm_page *pg)
{

	KASSERT(pm != pmap_kernel());
	KASSERT(VM_PAGE_TO_PP(pg)->pp_pv.pv_pmap == NULL);
	KASSERT(VM_PAGE_TO_PP(pg)->pp_pv.pv_next == NULL);

	LIST_REMOVE(pg, pageq.list);
	pg->wire_count = 0;
	uvm_pagefree(pg);
	PMAP_COUNT(pdp_free);
}

/* free empty page table pages */
static void
_pmap_sweep_pdp(struct pmap *pm)
{
	struct vm_page *pg, *tmp;
	pd_entry_t *ptep_in_parent, opte __diagused;
	paddr_t pa, pdppa;
	uint16_t wirecount __diagused;

	KASSERT(mutex_owned(&pm->pm_lock) || pm->pm_refcnt == 0);

	LIST_FOREACH_SAFE(pg, &pm->pm_vmlist, pageq.list, tmp) {
		if (pg->wire_count != 1)
			continue;

		pa = VM_PAGE_TO_PHYS(pg);
		if (pa == pm->pm_l0table_pa)
			continue;

		ptep_in_parent = VM_PAGE_TO_MD(pg)->mdpg_ptep_parent;
		if (ptep_in_parent == NULL) {
			/* no parent */
			pmap_free_pdp(pm, pg);
			continue;
		}

		/* unlink from parent */
		opte = atomic_swap_64(ptep_in_parent, 0);
		KASSERT(lxpde_valid(opte));
		wirecount = --pg->wire_count; /* 1 -> 0 */
		KASSERT(wirecount == 0);
		pmap_free_pdp(pm, pg);

		/* L3->L2->L1. no need for L0 */
		pdppa = AARCH64_KVA_TO_PA(trunc_page((vaddr_t)ptep_in_parent));
		if (pdppa == pm->pm_l0table_pa)
			continue;

		pg = PHYS_TO_VM_PAGE(pdppa);
		KASSERT(pg != NULL);
		KASSERTMSG(pg->wire_count >= 1,
		    "wire_count=%d", pg->wire_count);
		/* decrement wire_count of parent */
		wirecount = --pg->wire_count;
		KASSERTMSG(pg->wire_count <= (Ln_ENTRIES + 1),
		    "pm=%p, pg=%p, wire_count=%d",
		    pm, pg, pg->wire_count);
	}
	pm->pm_idlepdp = 0;
}

static void
_pmap_free_pdp_all(struct pmap *pm, bool free_l0)
{
	struct vm_page *pg, *pgtmp, *pg_reserve;

	pg_reserve = free_l0 ? NULL : PHYS_TO_VM_PAGE(pm->pm_l0table_pa);
	LIST_FOREACH_SAFE(pg, &pm->pm_vmlist, pageq.list, pgtmp) {
		if (pg == pg_reserve)
			continue;
		pmap_free_pdp(pm, pg);
	}
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct pmap *pm = pmap_kernel();
	struct vm_page *pg;
	int error;
	vaddr_t va;
	paddr_t pa;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "maxkvaddr=%llx, pmap_maxkvaddr=%llx",
	    maxkvaddr, pmap_maxkvaddr, 0, 0);

	mutex_enter(&pm->pm_lock);
	for (va = pmap_maxkvaddr & L2_FRAME; va <= maxkvaddr; va += L2_SIZE) {
		error = _pmap_get_pdp(pm, va, false, 0, &pa, &pg);
		if (error != 0) {
			panic("%s: cannot allocate L3 table error=%d",
			    __func__, error);
		}
	}
	kasan_shadow_map((void *)pmap_maxkvaddr,
	    (size_t)(va - pmap_maxkvaddr));
	pmap_maxkvaddr = va;
	mutex_exit(&pm->pm_lock);

	return va;
}

bool
pmap_extract(struct pmap *pm, vaddr_t va, paddr_t *pap)
{

	return pmap_extract_coherency(pm, va, pap, NULL);
}

bool
pmap_extract_coherency(struct pmap *pm, vaddr_t va, paddr_t *pap,
    bool *coherencyp)
{
	pt_entry_t *ptep, pte;
	paddr_t pa;
	vsize_t blocksize = 0;
	int space;
	bool coherency, valid;
	extern char __kernel_text[];
	extern char _end[];

	coherency = false;

	space = aarch64_addressspace(va);
	if (pm == pmap_kernel()) {
		if (space != AARCH64_ADDRSPACE_UPPER)
			return false;

		if (IN_RANGE(va, (vaddr_t)__kernel_text, (vaddr_t)_end)) {
			/* kernel text/data/bss are definitely linear mapped */
			pa = KERN_VTOPHYS(va);
			goto mapped;
		} else if (IN_DIRECTMAP_ADDR(va)) {
			/*
			 * also direct mapping is linear mapped, but areas that
			 * have no physical memory haven't been mapped.
			 * fast lookup by using the S1E1R/PAR_EL1 registers.
			 */
			register_t s = daif_disable(DAIF_I|DAIF_F);
			reg_s1e1r_write(va);
			isb();
			uint64_t par = reg_par_el1_read();
			reg_daif_write(s);

			if (par & PAR_F)
				return false;
			pa = (__SHIFTOUT(par, PAR_PA) << PAR_PA_SHIFT) +
			    (va & __BITS(PAR_PA_SHIFT - 1, 0));
			goto mapped;
		}
	} else {
		if (space != AARCH64_ADDRSPACE_LOWER)
			return false;
	}

	/*
	 * other areas, it isn't able to examined using the PAR_EL1 register,
	 * because the page may be in an access fault state due to
	 * reference bit emulation.
	 */
	if (pm != pmap_kernel())
		mutex_enter(&pm->pm_lock);
	ptep = _pmap_pte_lookup_bs(pm, va, &blocksize);
	valid = (ptep != NULL && lxpde_valid(pte = *ptep));
	if (pm != pmap_kernel())
		mutex_exit(&pm->pm_lock);

	if (!valid) {
		return false;
	}

	pa = lxpde_pa(pte) + (va & (blocksize - 1));

	switch (pte & LX_BLKPAG_ATTR_MASK) {
	case LX_BLKPAG_ATTR_NORMAL_NC:
	case LX_BLKPAG_ATTR_DEVICE_MEM:
	case LX_BLKPAG_ATTR_DEVICE_MEM_SO:
		coherency = true;
		break;
	}

 mapped:
	if (pap != NULL)
		*pap = pa;
	if (coherencyp != NULL)
		*coherencyp = coherency;
	return true;
}

paddr_t
vtophys(vaddr_t va)
{
	struct pmap *pm;
	paddr_t pa;

	/* even if TBI is disabled, AARCH64_ADDRTOP_TAG means KVA */
	if ((uint64_t)va & AARCH64_ADDRTOP_TAG)
		pm = pmap_kernel();
	else
		pm = curlwp->l_proc->p_vmspace->vm_map.pmap;

	if (pmap_extract(pm, va, &pa) == false)
		return VTOPHYS_FAILED;
	return pa;
}

/*
 * return pointer of the pte. regardess of whether the entry is valid or not.
 */
static pt_entry_t *
_pmap_pte_lookup_bs(struct pmap *pm, vaddr_t va, vsize_t *bs)
{
	pt_entry_t *ptep;
	pd_entry_t *l0, *l1, *l2, *l3;
	pd_entry_t pde;
	vsize_t blocksize;
	unsigned int idx;

	KASSERT(pm == pmap_kernel() || mutex_owned(&pm->pm_lock));

	/*
	 * traverse L0 -> L1 -> L2 -> L3
	 */
	blocksize = L0_SIZE;
	l0 = pm->pm_l0table;
	idx = l0pde_index(va);
	ptep = &l0[idx];
	pde = *ptep;
	if (!l0pde_valid(pde))
		goto done;

	blocksize = L1_SIZE;
	l1 = (pd_entry_t *)AARCH64_PA_TO_KVA(l0pde_pa(pde));
	idx = l1pde_index(va);
	ptep = &l1[idx];
	pde = *ptep;
	if (!l1pde_valid(pde) || l1pde_is_block(pde))
		goto done;

	blocksize = L2_SIZE;
	l2 = (pd_entry_t *)AARCH64_PA_TO_KVA(l1pde_pa(pde));
	idx = l2pde_index(va);
	ptep = &l2[idx];
	pde = *ptep;
	if (!l2pde_valid(pde) || l2pde_is_block(pde))
		goto done;

	blocksize = L3_SIZE;
	l3 = (pd_entry_t *)AARCH64_PA_TO_KVA(l2pde_pa(pde));
	idx = l3pte_index(va);
	ptep = &l3[idx];

 done:
	if (bs != NULL)
		*bs = blocksize;
	return ptep;
}

static pt_entry_t *
_pmap_pte_lookup_l3(struct pmap *pm, vaddr_t va)
{
	pt_entry_t *ptep;
	vsize_t blocksize = 0;

	ptep = _pmap_pte_lookup_bs(pm, va, &blocksize);
	if ((ptep != NULL) && (blocksize == L3_SIZE))
		return ptep;

	return NULL;
}

void
pmap_icache_sync_range(pmap_t pm, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *ptep = NULL, pte;
	vaddr_t va;
	vsize_t blocksize = 0;

	KASSERT_PM_ADDR(pm, sva);

	pm_lock(pm);

	for (va = sva; va < eva; va = (va + blocksize) & ~(blocksize - 1)) {
		/* va is belong to the same L3 table as before? */
		if ((blocksize == L3_SIZE) && ((va & L3INDEXMASK) != 0)) {
			ptep++;
		} else {
			ptep = _pmap_pte_lookup_bs(pm, va, &blocksize);
			if (ptep == NULL)
				break;
		}

		pte = *ptep;
		if (!lxpde_valid(pte))
			continue;

		vaddr_t eob = (va + blocksize) & ~(blocksize - 1);
		vsize_t len = ulmin(eva, eob) - va;

		if (l3pte_readable(pte)) {
			cpu_icache_sync_range(va, len);
		} else {
			/*
			 * change to accessible temporally
			 * to do cpu_icache_sync_range()
			 */
			struct pmap_asid_info * const pai = PMAP_PAI(pm,
			    cpu_tlb_info(ci));

			atomic_swap_64(ptep, pte | LX_BLKPAG_AF);
			AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);
			cpu_icache_sync_range(va, len);
			atomic_swap_64(ptep, pte);
			AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);
		}
	}

	pm_unlock(pm);
}

/*
 * Routine:	pmap_procwr
 *
 * Function:
 *	Synchronize caches corresponding to [addr, addr+len) in p.
 *
 */
void
pmap_procwr(struct proc *p, vaddr_t sva, int len)
{

	if (__predict_true(p == curproc))
		cpu_icache_sync_range(sva, len);
	else {
		struct pmap *pm = p->p_vmspace->vm_map.pmap;
		paddr_t pa;
		vaddr_t va, eva;
		int tlen;

		for (va = sva; len > 0; va = eva, len -= tlen) {
			eva = uimin(va + len, trunc_page(va + PAGE_SIZE));
			tlen = eva - va;
			if (!pmap_extract(pm, va, &pa))
				continue;
			va = AARCH64_PA_TO_KVA(pa);
			cpu_icache_sync_range(va, tlen);
		}
	}
}

static pt_entry_t
_pmap_pte_adjust_prot(pt_entry_t pte, vm_prot_t prot, vm_prot_t refmod,
    bool user)
{
	vm_prot_t masked;
	pt_entry_t xn;

	masked = prot & refmod;
	pte &= ~(LX_BLKPAG_OS_RWMASK|LX_BLKPAG_AF|LX_BLKPAG_DBM|LX_BLKPAG_AP);

	/*
	 * keep actual prot in the pte as OS_{READ|WRITE} for ref/mod emulation,
	 * and set the DBM bit for HAFDBS if it has write permission.
	 */
	pte |= LX_BLKPAG_OS_READ;	/* a valid pte can always be readable */
	if (prot & VM_PROT_WRITE)
		pte |= LX_BLKPAG_OS_WRITE|LX_BLKPAG_DBM;

	switch (masked & (VM_PROT_READ|VM_PROT_WRITE)) {
	case 0:
	default:
		/*
		 * it cannot be accessed because there is no AF bit,
		 * but the AF bit will be added by fixup() or HAFDBS.
		 */
		pte |= LX_BLKPAG_AP_RO;
		break;
	case VM_PROT_READ:
		/*
		 * as it is RO, it cannot be written as is,
		 * but it may be changed to RW by fixup() or HAFDBS.
		 */
		pte |= LX_BLKPAG_AF;
		pte |= LX_BLKPAG_AP_RO;
		break;
	case VM_PROT_WRITE:
	case VM_PROT_READ|VM_PROT_WRITE:
		/* fully readable and writable */
		pte |= LX_BLKPAG_AF;
		pte |= LX_BLKPAG_AP_RW;
		break;
	}

	/* executable for kernel or user? first set never exec both */
	pte |= (LX_BLKPAG_UXN|LX_BLKPAG_PXN);
	/* and either to executable */
	xn = user ? LX_BLKPAG_UXN : LX_BLKPAG_PXN;
	if (prot & VM_PROT_EXECUTE)
		pte &= ~xn;

	return pte;
}

static pt_entry_t
_pmap_pte_adjust_cacheflags(pt_entry_t pte, u_int flags)
{

	pte &= ~LX_BLKPAG_ATTR_MASK;

	switch (flags & (PMAP_CACHE_MASK|PMAP_DEV_MASK)) {
	case PMAP_DEV_SO ... PMAP_DEV_SO | PMAP_CACHE_MASK:
		pte |= LX_BLKPAG_ATTR_DEVICE_MEM_SO;	/* Device-nGnRnE */
		break;
	case PMAP_DEV ... PMAP_DEV | PMAP_CACHE_MASK:
		pte |= LX_BLKPAG_ATTR_DEVICE_MEM;	/* Device-nGnRE */
		break;
	case PMAP_NOCACHE:
	case PMAP_NOCACHE_OVR:
	case PMAP_WRITE_COMBINE:
		pte |= LX_BLKPAG_ATTR_NORMAL_NC;	/* only no-cache */
		break;
	case PMAP_WRITE_BACK:
	case 0:
	default:
		pte |= LX_BLKPAG_ATTR_NORMAL_WB;
		break;
	}

	return pte;
}

#ifdef ARMV81_HAFDBS
static inline void
_pmap_reflect_refmod_in_pp(pt_entry_t pte, struct pmap_page *pp)
{
	if (!lxpde_valid(pte))
		return;

	/*
	 * In order to retain referenced/modified information,
	 * it should be reflected from pte in the pmap_page.
	 */
	if (pte & LX_BLKPAG_AF)
		pp->pp_pv.pv_va |= VM_PROT_READ;
	if ((pte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RW)
		pp->pp_pv.pv_va |= VM_PROT_WRITE;
}
#endif

static struct pv_entry *
_pmap_remove_pv(struct pmap_page *pp, struct pmap *pm, vaddr_t va,
    pt_entry_t pte)
{
	struct pv_entry *pv, *ppv;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pp=%p, pm=%p, va=%llx, pte=%llx",
	    pp, pm, va, pte);

	KASSERT(mutex_owned(&pm->pm_lock));	/* for pv_proc */
	KASSERT(mutex_owned(&pp->pp_pvlock));

#ifdef ARMV81_HAFDBS
	if (aarch64_hafdbs_enabled != ID_AA64MMFR1_EL1_HAFDBS_NONE)
		_pmap_reflect_refmod_in_pp(pte, pp);
#endif

	for (ppv = NULL, pv = &pp->pp_pv; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap == pm && trunc_page(pv->pv_va) == va) {
			break;
		}
		ppv = pv;
	}

	if (pm != pmap_kernel() && pv != NULL)
		LIST_REMOVE(pv, pv_proc);

	if (ppv == NULL) {
		/* embedded in pmap_page */
		pv->pv_pmap = NULL;
		pv = NULL;
		PMAP_COUNT(pv_remove_emb);
	} else if (pv != NULL) {
		/* dynamically allocated */
		ppv->pv_next = pv->pv_next;
		PMAP_COUNT(pv_remove_dyn);
	} else {
		PMAP_COUNT(pv_remove_nopv);
	}

	return pv;
}

#if defined(PMAP_PV_DEBUG) || defined(DDB)

static char *
str_vmflags(uint32_t flags)
{
	static int idx = 0;
	static char buf[4][32];	/* XXX */
	char *p;

	p = buf[idx];
	idx = (idx + 1) & 3;

	p[0] = (flags & VM_PROT_READ) ? 'R' : '-';
	p[1] = (flags & VM_PROT_WRITE) ? 'W' : '-';
	p[2] = (flags & VM_PROT_EXECUTE) ? 'X' : '-';
	if (flags & PMAP_WIRED)
		memcpy(&p[3], ",WIRED\0", 7);
	else
		p[3] = '\0';

	return p;
}

void
pmap_db_mdpg_print(struct vm_page *pg, void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct pmap_page *pp = VM_PAGE_TO_PP(pg);
	struct pv_entry *pv;
	int i, flags;

	i = 0;
	flags = pp->pp_pv.pv_va & (PAGE_SIZE - 1);

	pr("pp=%p\n", pp);
	pr(" pp flags=%08x %s\n", flags, str_vmflags(flags));

	for (pv = &pp->pp_pv; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap == NULL) {
			KASSERT(pv == &pp->pp_pv);
			continue;
		}
		struct pmap * const pm = pv->pv_pmap;
		struct pmap_asid_info * const pai = PMAP_PAI(pm,
		    cpu_tlb_info(ci));

		pr("  pv[%d] pv=%p\n", i, pv);
		pr("    pv[%d].pv_pmap = %p (asid=%d)\n", i, pm, pai->pai_asid);
		pr("    pv[%d].pv_va   = %016lx (color=%d)\n", i,
		    trunc_page(pv->pv_va), _pmap_color(pv->pv_va));
		pr("    pv[%d].pv_ptep = %p\n", i, pv->pv_ptep);
		i++;
	}
}
#endif /* PMAP_PV_DEBUG & DDB */

static int
_pmap_enter_pv(struct pmap_page *pp, struct pmap *pm, struct pv_entry **pvp,
    vaddr_t va, pt_entry_t *ptep, paddr_t pa, u_int flags)
{
	struct pv_entry *pv;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pp=%p, pm=%p, va=%llx, pa=%llx", pp, pm, va,
	    pa);
	UVMHIST_LOG(pmaphist, "ptep=%p, flags=%08x", ptep, flags, 0, 0);

	KASSERT(mutex_owned(&pp->pp_pvlock));
	KASSERT(trunc_page(va) == va);

	/*
	 * mapping cannot be already registered at this VA.
	 */
	if (pp->pp_pv.pv_pmap == NULL) {
		/*
		 * claim pv_entry embedded in pmap_page.
		 * take care not to wipe out acc/mod flags.
		 */
		pv = &pp->pp_pv;
		pv->pv_va = (pv->pv_va & (PAGE_SIZE - 1)) | va;
	} else {
		/*
		 * create and link new pv.
		 * pv is already allocated at beginning of _pmap_enter().
		 */
		pv = *pvp;
		if (pv == NULL)
			return ENOMEM;
		*pvp = NULL;
		pv->pv_next = pp->pp_pv.pv_next;
		pp->pp_pv.pv_next = pv;
		pv->pv_va = va;
	}
	pv->pv_pmap = pm;
	pv->pv_ptep = ptep;
	PMAP_COUNT(pv_enter);

	if (pm != pmap_kernel())
		LIST_INSERT_HEAD(&pm->pm_pvlist, pv, pv_proc);

#ifdef PMAP_PV_DEBUG
	printf("pv %p alias added va=%016lx -> pa=%016lx\n", pv, va, pa);
	pv_dump(pp, printf);
#endif

	return 0;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{

	_pmap_enter(pmap_kernel(), va, pa, prot, flags | PMAP_WIRED, true);
}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
	struct pmap *kpm = pmap_kernel();

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "va=%llx, size=%llx", va, size, 0, 0);

	KDASSERT((va & PGOFSET) == 0);
	KDASSERT((size & PGOFSET) == 0);

	KDASSERT(!IN_DIRECTMAP_ADDR(va));
	KDASSERT(IN_RANGE(va, VM_MIN_KERNEL_ADDRESS, VM_MAX_KERNEL_ADDRESS));

	_pmap_remove(kpm, va, va + size, true, NULL);
}

static void
_pmap_protect_pv(struct pmap_page *pp, struct pv_entry *pv, vm_prot_t prot)
{
	pt_entry_t *ptep, pte;
	vm_prot_t pteprot;
	uint32_t mdattr;
	const bool user = (pv->pv_pmap != pmap_kernel());

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pp=%p, pv=%p, prot=%08x", pp, pv, prot, 0);

	KASSERT(mutex_owned(&pv->pv_pmap->pm_lock));

	ptep = pv->pv_ptep;
	pte = *ptep;

	/* get prot mask from pte */
	pteprot = VM_PROT_READ;	/* a valid pte can always be readable */
	if ((pte & (LX_BLKPAG_OS_WRITE|LX_BLKPAG_DBM)) != 0)
		pteprot |= VM_PROT_WRITE;
	if (l3pte_executable(pte, user))
		pteprot |= VM_PROT_EXECUTE;

#ifdef ARMV81_HAFDBS
	if (aarch64_hafdbs_enabled != ID_AA64MMFR1_EL1_HAFDBS_NONE)
		_pmap_reflect_refmod_in_pp(pte, pp);
#endif
	/* get prot mask from referenced/modified */
	mdattr = pp->pp_pv.pv_va & (VM_PROT_READ | VM_PROT_WRITE);

	/* new prot = prot & pteprot & mdattr */
	pte = _pmap_pte_adjust_prot(pte, prot & pteprot, mdattr, user);
	atomic_swap_64(ptep, pte);

	struct pmap * const pm = pv->pv_pmap;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));

	AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, trunc_page(pv->pv_va));
}

void
pmap_protect(struct pmap *pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pt_entry_t *ptep = NULL, pte;
	vaddr_t va;
	vsize_t blocksize = 0;
	const bool user = (pm != pmap_kernel());

	KASSERT((prot & VM_PROT_READ) || !(prot & VM_PROT_WRITE));

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, sva=%016lx, eva=%016lx, prot=%08x",
	    pm, sva, eva, prot);

	KASSERT_PM_ADDR(pm, sva);
	KASSERT(!IN_DIRECTMAP_ADDR(sva));

	/* PROT_EXEC requires implicit PROT_READ */
	if (prot & VM_PROT_EXECUTE)
		prot |= VM_PROT_READ;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		PMAP_COUNT(protect_remove_fallback);
		pmap_remove(pm, sva, eva);
		return;
	}
	PMAP_COUNT(protect);

	KDASSERT((sva & PAGE_MASK) == 0);
	KDASSERT((eva & PAGE_MASK) == 0);

	pm_lock(pm);

	for (va = sva; va < eva; va = (va + blocksize) & ~(blocksize - 1)) {
#ifdef UVMHIST
		pt_entry_t opte;
#endif
		struct vm_page *pg;
		struct pmap_page *pp;
		paddr_t pa;
		uint32_t mdattr;
		bool executable;

		/* va is belong to the same L3 table as before? */
		if ((blocksize == L3_SIZE) && ((va & L3INDEXMASK) != 0))
			ptep++;
		else
			ptep = _pmap_pte_lookup_bs(pm, va, &blocksize);

		pte = *ptep;
		if (!lxpde_valid(pte)) {
			PMAP_COUNT(protect_none);
			continue;
		}

		pa = lxpde_pa(pte);
		pg = PHYS_TO_VM_PAGE(pa);
		if (pg != NULL) {
			pp = VM_PAGE_TO_PP(pg);
			PMAP_COUNT(protect_managed);
		} else {
#ifdef __HAVE_PMAP_PV_TRACK
			pp = pmap_pv_tracked(pa);
#ifdef PMAPCOUNTERS
			if (pp != NULL)
				PMAP_COUNT(protect_pvmanaged);
			else
				PMAP_COUNT(protect_unmanaged);
#endif
#else
			pp = NULL;
			PMAP_COUNT(protect_unmanaged);
#endif /* __HAVE_PMAP_PV_TRACK */
		}

		if (pp != NULL) {
#ifdef ARMV81_HAFDBS
			if (aarch64_hafdbs_enabled != ID_AA64MMFR1_EL1_HAFDBS_NONE)
				_pmap_reflect_refmod_in_pp(pte, pp);
#endif
			/* get prot mask from referenced/modified */
			mdattr = pp->pp_pv.pv_va &
			    (VM_PROT_READ | VM_PROT_WRITE);
		} else {
			/* unmanaged page */
			mdattr = VM_PROT_ALL;
		}

#ifdef UVMHIST
		opte = pte;
#endif
		executable = l3pte_executable(pte, user);
		pte = _pmap_pte_adjust_prot(pte, prot, mdattr, user);

		struct pmap_asid_info * const pai = PMAP_PAI(pm,
		    cpu_tlb_info(ci));
		if (!executable && (prot & VM_PROT_EXECUTE)) {
			/* non-exec -> exec */
			UVMHIST_LOG(pmaphist, "icache_sync: "
			    "pm=%p, va=%016lx, pte: %016lx -> %016lx",
			    pm, va, opte, pte);

			if (!l3pte_readable(pte)) {
				PTE_ICACHE_SYNC_PAGE(pte, ptep, pai->pai_asid,
				    va);
				atomic_swap_64(ptep, pte);
				AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);
			} else {
				atomic_swap_64(ptep, pte);
				AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);
				cpu_icache_sync_range(va, PAGE_SIZE);
			}
		} else {
			atomic_swap_64(ptep, pte);
			AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);
		}
	}

	pm_unlock(pm);
}

#if defined(EFI_RUNTIME)
void
pmap_activate_efirt(void)
{
	kpreempt_disable();

	struct cpu_info *ci = curcpu();
	struct pmap *pm = &efirt_pmap;
	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, " (pm=%#jx)", (uintptr_t)pm, 0, 0, 0);

	ci->ci_pmap_asid_cur = pai->pai_asid;
	UVMHIST_LOG(pmaphist, "setting asid to %#jx", pai->pai_asid,
	    0, 0, 0);
	tlb_set_asid(pai->pai_asid, pm);

	/* Re-enable translation table walks using TTBR0 */
	uint64_t tcr = reg_tcr_el1_read();
	reg_tcr_el1_write(tcr & ~TCR_EPD0);
	isb();
	pm->pm_activated = true;

	PMAP_COUNT(activate);
}
#endif

void
pmap_activate(struct lwp *l)
{
	struct pmap *pm = l->l_proc->p_vmspace->vm_map.pmap;
	uint64_t tcr;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "lwp=%p (pid=%d, kernel=%u)", l,
	    l->l_proc->p_pid, pm == pmap_kernel() ? 1 : 0, 0);

	KASSERT((reg_tcr_el1_read() & TCR_EPD0) != 0);

	if (pm == pmap_kernel())
		return;
	if (l != curlwp)
		return;

	KASSERT(pm->pm_l0table != NULL);

	/* this calls tlb_set_asid which calls cpu_set_ttbr0 */
	pmap_tlb_asid_acquire(pm, l);

	UVMHIST_LOG(pmaphist, "lwp=%p, asid=%d", l,
	    PMAP_PAI(pm, cpu_tlb_info(ci))->pai_asid, 0, 0);

	/* Re-enable translation table walks using TTBR0 */
	tcr = reg_tcr_el1_read();
	reg_tcr_el1_write(tcr & ~TCR_EPD0);
	isb();

	pm->pm_activated = true;

	PMAP_COUNT(activate);
}

#if defined(EFI_RUNTIME)
void
pmap_deactivate_efirt(void)
{
	struct cpu_info * const ci = curcpu();
	struct pmap * const pm = &efirt_pmap;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);

	/* Disable translation table walks using TTBR0 */
	uint64_t tcr = reg_tcr_el1_read();
	reg_tcr_el1_write(tcr | TCR_EPD0);
	isb();

	UVMHIST_LOG(pmaphist, "setting asid to %#jx", KERNEL_PID,
	    0, 0, 0);

	ci->ci_pmap_asid_cur = KERNEL_PID;
        tlb_set_asid(KERNEL_PID, pmap_kernel());

	pm->pm_activated = false;

	PMAP_COUNT(deactivate);

	kpreempt_enable();
}
#endif

void
pmap_deactivate(struct lwp *l)
{
	struct pmap *pm = l->l_proc->p_vmspace->vm_map.pmap;
	uint64_t tcr;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "lwp=%p (pid=%d, (kernel=%u))", l,
	    l->l_proc->p_pid, pm == pmap_kernel() ? 1 : 0, 0);

	/* Disable translation table walks using TTBR0 */
	tcr = reg_tcr_el1_read();
	reg_tcr_el1_write(tcr | TCR_EPD0);
	isb();

	UVMHIST_LOG(pmaphist, "lwp=%p, asid=%d", l,
	    PMAP_PAI(pm, cpu_tlb_info(ci))->pai_asid, 0, 0);

	pmap_tlb_asid_deactivate(pm);

	pm->pm_activated = false;

	PMAP_COUNT(deactivate);
}

struct pmap *
pmap_create(void)
{
	struct pmap *pm;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLED(pmaphist);

	pm = pool_cache_get(&_pmap_cache, PR_WAITOK);
	memset(pm, 0, sizeof(*pm));
	pm->pm_refcnt = 1;
	pm->pm_idlepdp = 0;
	LIST_INIT(&pm->pm_vmlist);
	LIST_INIT(&pm->pm_pvlist);
	mutex_init(&pm->pm_lock, MUTEX_DEFAULT, IPL_NONE);

	kcpuset_create(&pm->pm_active, true);
	kcpuset_create(&pm->pm_onproc, true);

	pm->pm_l0table_pa = pmap_alloc_pdp(pm, NULL, 0, true);
	KASSERT(pm->pm_l0table_pa != POOL_PADDR_INVALID);
	pm->pm_l0table = (pd_entry_t *)AARCH64_PA_TO_KVA(pm->pm_l0table_pa);
	KASSERT(((vaddr_t)pm->pm_l0table & (PAGE_SIZE - 1)) == 0);

	UVMHIST_LOG(pmaphist, "pm=%p, pm_l0table=%016lx, pm_l0table_pa=%016lx",
	    pm, pm->pm_l0table, pm->pm_l0table_pa, 0);

	PMAP_COUNT(create);
	return pm;
}

void
pmap_destroy(struct pmap *pm)
{
	unsigned int refcnt;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, pm_l0table=%016lx, refcnt=%jd",
	    pm, pm->pm_l0table, pm->pm_refcnt, 0);

	if (pm == NULL)
		return;

	if (pm == pmap_kernel())
		panic("cannot destroy kernel pmap");

	membar_release();
	refcnt = atomic_dec_uint_nv(&pm->pm_refcnt);
	if (refcnt > 0)
		return;
	membar_acquire();

	KASSERT(LIST_EMPTY(&pm->pm_pvlist));
	pmap_tlb_asid_release_all(pm);

	_pmap_free_pdp_all(pm, true);
	mutex_destroy(&pm->pm_lock);

	kcpuset_destroy(pm->pm_active);
	kcpuset_destroy(pm->pm_onproc);

	pool_cache_put(&_pmap_cache, pm);

	PMAP_COUNT(destroy);
}

static inline void
_pmap_pdp_setparent(struct pmap *pm, struct vm_page *pg, pt_entry_t *ptep)
{

	if ((pm != pmap_kernel()) && (pg != NULL)) {
		KASSERT(mutex_owned(&pm->pm_lock));
		VM_PAGE_TO_MD(pg)->mdpg_ptep_parent = ptep;
	}
}

/*
 * increment reference counter of the page descriptor page.
 * the reference counter should be equal to
 *  1 + num of valid entries the page has.
 */
static inline void
_pmap_pdp_addref(struct pmap *pm, paddr_t pdppa, struct vm_page *pdppg_hint)
{
	struct vm_page *pg;

	/* kernel L0-L3 pages will never be freed */
	if (pm == pmap_kernel())
		return;

#if defined(EFI_RUNTIME)
	/* EFI runtme L0-L3 pages will never be freed */
	if (pm == pmap_efirt())
		return;
#endif

	KASSERT(mutex_owned(&pm->pm_lock));

	/* no need for L0 page */
	if (pm->pm_l0table_pa == pdppa)
		return;

	pg = pdppg_hint;
	if (pg == NULL)
		pg = PHYS_TO_VM_PAGE(pdppa);
	KASSERT(pg != NULL);

	pg->wire_count++;

	KASSERTMSG(pg->wire_count <= (Ln_ENTRIES + 1),
	    "pg=%p, wire_count=%d", pg, pg->wire_count);
}

/*
 * decrement reference counter of the page descriptor page.
 * if reference counter is 1(=empty), pages will be freed, and return true.
 * otherwise return false.
 * kernel page, or L0 page descriptor page will be never freed.
 */
static bool
_pmap_pdp_delref(struct pmap *pm, paddr_t pdppa, bool do_free_pdp)
{
	struct vm_page *pg;
	bool removed;
	uint16_t wirecount;

	/* kernel L0-L3 pages will never be freed */
	if (pm == pmap_kernel())
		return false;

#if defined(EFI_RUNTIME)
	/* EFI runtme L0-L3 pages will never be freed */
	if (pm == pmap_efirt())
		return false;
#endif

	KASSERT(mutex_owned(&pm->pm_lock));

	/* no need for L0 page */
	if (pm->pm_l0table_pa == pdppa)
		return false;

	pg = PHYS_TO_VM_PAGE(pdppa);
	KASSERT(pg != NULL);

	wirecount = --pg->wire_count;

	if (!do_free_pdp) {
		/*
		 * pm_idlepdp is counted by only pmap_page_protect() with
		 * VM_PROT_NONE. it is not correct because without considering
		 * pmap_enter(), but useful hint to just sweep.
		 */
		if (wirecount == 1)
			pm->pm_idlepdp++;
		return false;
	}

	/* if no reference, free pdp */
	removed = false;
	while (wirecount == 1) {
		pd_entry_t *ptep_in_parent, opte __diagused;
		ptep_in_parent = VM_PAGE_TO_MD(pg)->mdpg_ptep_parent;
		if (ptep_in_parent == NULL) {
			/* no parent */
			pmap_free_pdp(pm, pg);
			removed = true;
			break;
		}

		/* unlink from parent */
		opte = atomic_swap_64(ptep_in_parent, 0);
		KASSERT(lxpde_valid(opte));
		wirecount = atomic_add_32_nv(&pg->wire_count, -1); /* 1 -> 0 */
		KASSERT(wirecount == 0);
		pmap_free_pdp(pm, pg);
		removed = true;

		/* L3->L2->L1. no need for L0 */
		pdppa = AARCH64_KVA_TO_PA(trunc_page((vaddr_t)ptep_in_parent));
		if (pdppa == pm->pm_l0table_pa)
			break;

		pg = PHYS_TO_VM_PAGE(pdppa);
		KASSERT(pg != NULL);
		KASSERTMSG(pg->wire_count >= 1,
		    "wire_count=%d", pg->wire_count);
		/* decrement wire_count of parent */
		wirecount = atomic_add_32_nv(&pg->wire_count, -1);
		KASSERTMSG(pg->wire_count <= (Ln_ENTRIES + 1),
		    "pm=%p, pg=%p, wire_count=%d",
		    pm, pg, pg->wire_count);
	}

	return removed;
}

/*
 * traverse L0 -> L1 -> L2 -> L3 table with growing pdp if needed.
 */
static int
_pmap_get_pdp(struct pmap *pm, vaddr_t va, bool kenter, int flags,
    paddr_t *pap, struct vm_page **pgp)
{
	pd_entry_t *l0, *l1, *l2;
	struct vm_page *pdppg, *pdppg0;
	paddr_t pdppa, pdppa0;
	unsigned int idx;
	pd_entry_t pde;

	KASSERT(kenter || mutex_owned(&pm->pm_lock));

	l0 = pm->pm_l0table;

	idx = l0pde_index(va);
	pde = l0[idx];
	if (!l0pde_valid(pde)) {
		KASSERTMSG(!kenter || IN_MODULE_VA(va) || PMAP_EFIVA_P(va),
		    "%s va %" PRIxVADDR, kenter ? "kernel" : "user", va);
		/* no need to increment L0 occupancy. L0 page never freed */
		pdppa = pmap_alloc_pdp(pm, &pdppg, flags, false);  /* L1 pdp */
		if (pdppa == POOL_PADDR_INVALID) {
			return ENOMEM;
		}
		atomic_swap_64(&l0[idx], pdppa | L0_TABLE);
		_pmap_pdp_setparent(pm, pdppg, &l0[idx]);
	} else {
		pdppa = l0pde_pa(pde);
		pdppg = NULL;
	}
	l1 = (void *)AARCH64_PA_TO_KVA(pdppa);

	idx = l1pde_index(va);
	pde = l1[idx];
	if (!l1pde_valid(pde)) {
		KASSERTMSG(!kenter || IN_MODULE_VA(va) || PMAP_EFIVA_P(va),
		    "%s va %" PRIxVADDR, kenter ? "kernel" : "user", va);
		pdppa0 = pdppa;
		pdppg0 = pdppg;
		pdppa = pmap_alloc_pdp(pm, &pdppg, flags, false);  /* L2 pdp */
		if (pdppa == POOL_PADDR_INVALID) {
			return ENOMEM;
		}
		atomic_swap_64(&l1[idx], pdppa | L1_TABLE);
		_pmap_pdp_addref(pm, pdppa0, pdppg0);	/* L1 occupancy++ */
		_pmap_pdp_setparent(pm, pdppg, &l1[idx]);
	} else {
		pdppa = l1pde_pa(pde);
		pdppg = NULL;
	}
	l2 = (void *)AARCH64_PA_TO_KVA(pdppa);

	idx = l2pde_index(va);
	pde = l2[idx];
	if (!l2pde_valid(pde)) {
		KASSERTMSG(!kenter || IN_MODULE_VA(va) || PMAP_EFIVA_P(va),
		    "%s va %" PRIxVADDR, kenter ? "kernel" : "user", va);
		pdppa0 = pdppa;
		pdppg0 = pdppg;
		pdppa = pmap_alloc_pdp(pm, &pdppg, flags, false);  /* L3 pdp */
		if (pdppa == POOL_PADDR_INVALID) {
			return ENOMEM;
		}
		atomic_swap_64(&l2[idx], pdppa | L2_TABLE);
		_pmap_pdp_addref(pm, pdppa0, pdppg0);	/* L2 occupancy++ */
		_pmap_pdp_setparent(pm, pdppg, &l2[idx]);
	} else {
		pdppa = l2pde_pa(pde);
		pdppg = NULL;
	}
	*pap = pdppa;
	*pgp = pdppg;
	return 0;
}

static int
_pmap_enter(struct pmap *pm, vaddr_t va, paddr_t pa, vm_prot_t prot,
    u_int flags, bool kenter)
{
	struct vm_page *pdppg;
	struct pmap_page *pp, *opp, *pps[2];
	struct pv_entry *spv, *opv = NULL;
	pt_entry_t attr, pte, opte, *ptep;
	pd_entry_t *l3;
	paddr_t pdppa;
	uint32_t mdattr;
	unsigned int idx;
	int error = 0;
#if defined(EFI_RUNTIME)
	const bool efirt_p = pm == pmap_efirt();
#else
	const bool efirt_p = false;
#endif
	const bool kernel_p = pm == pmap_kernel();
	const bool user = !kernel_p && !efirt_p;
	bool need_sync_icache, need_enter_pv;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, kentermode=%d", pm, kenter, 0, 0);
	UVMHIST_LOG(pmaphist, "va=%016lx, pa=%016lx, prot=%08x, flags=%08x",
	    va, pa, prot, flags);

	KASSERT_PM_ADDR(pm, va);
	KASSERT(!IN_DIRECTMAP_ADDR(va));
	KASSERT(prot & VM_PROT_READ);

#ifdef PMAPCOUNTERS
	PMAP_COUNT(mappings);
	if (_pmap_color(va) == _pmap_color(pa)) {
		if (user) {
			PMAP_COUNT(user_mappings);
		} else {
			PMAP_COUNT(kern_mappings);
		}
	} else if (flags & PMAP_WIRED) {
		if (user) {
			PMAP_COUNT(user_mappings_bad_wired);
		} else {
			PMAP_COUNT(kern_mappings_bad_wired);
		}
	} else {
		if (user) {
			PMAP_COUNT(user_mappings_bad);
		} else {
			PMAP_COUNT(kern_mappings_bad);
		}
	}
#endif

	if (kenter) {
		pp = NULL;
		spv = NULL;
		need_enter_pv = false;
	} else {
		struct vm_page *pg = PHYS_TO_VM_PAGE(pa);
		if (pg != NULL) {
			pp = VM_PAGE_TO_PP(pg);
			PMAP_COUNT(managed_mappings);
		} else {
#ifdef __HAVE_PMAP_PV_TRACK
			pp = pmap_pv_tracked(pa);
#ifdef PMAPCOUNTERS
			if (pp != NULL)
				PMAP_COUNT(pvmanaged_mappings);
			else
				PMAP_COUNT(unmanaged_mappings);
#endif
#else
			pp = NULL;
			PMAP_COUNT(unmanaged_mappings);
#endif /* __HAVE_PMAP_PV_TRACK */
		}

		if (pp != NULL) {
			/*
			 * allocate pv in advance of pm_lock().
			 */
			spv = pool_cache_get(&_pmap_pv_pool, PR_NOWAIT);
			need_enter_pv = true;
		} else {
			spv = NULL;
			need_enter_pv = false;
		}

		pm_lock(pm);
		if (pm->pm_idlepdp >= PDPSWEEP_TRIGGER) {
			_pmap_sweep_pdp(pm);
		}
	}

	/*
	 * traverse L0 -> L1 -> L2 -> L3 table with growing pdp if needed.
	 */
	error = _pmap_get_pdp(pm, va, kenter, flags, &pdppa, &pdppg);
	if (error != 0) {
		if (flags & PMAP_CANFAIL) {
			goto fail0;
		}
		panic("%s: cannot allocate L3 table error=%d", __func__,
		    error);
	}

	l3 = (void *)AARCH64_PA_TO_KVA(pdppa);

	idx = l3pte_index(va);
	ptep = &l3[idx];	/* as PTE */
	opte = *ptep;
	need_sync_icache = (prot & VM_PROT_EXECUTE) && !efirt_p;

	/* for lock ordering for old page and new page */
	pps[0] = pp;
	pps[1] = NULL;

	/* remap? */
	if (l3pte_valid(opte)) {
		bool need_remove_pv;

		KASSERT(!kenter);	/* pmap_kenter_pa() cannot override */
		if (opte & LX_BLKPAG_OS_WIRED) {
			_pmap_adj_wired_count(pm, -1);
		}
		_pmap_adj_resident_count(pm, -1);
#ifdef PMAPCOUNTERS
		PMAP_COUNT(remappings);
		if (user) {
			PMAP_COUNT(user_mappings_changed);
		} else {
			PMAP_COUNT(kern_mappings_changed);
		}
#endif
		UVMHIST_LOG(pmaphist,
		    "va=%016lx has already mapped."
		    " old-pa=%016lx new-pa=%016lx, old-pte=%016llx",
		    va, l3pte_pa(opte), pa, opte);

		if (pa == l3pte_pa(opte)) {
			/* old and new pte have same pa, no need to update pv */
			need_remove_pv = (pp == NULL);
			need_enter_pv = false;
			if (need_sync_icache && l3pte_executable(opte, user))
				need_sync_icache = false;
		} else {
			need_remove_pv = true;
		}

		if (need_remove_pv &&
		    ((opp = phys_to_pp(l3pte_pa(opte))) != NULL)) {
			/*
			 * need to lock both pp and opp(old pp)
			 * against deadlock, and 'pp' maybe NULL.
			 */
			if (pp < opp) {
				pps[0] = pp;
				pps[1] = opp;
			} else {
				pps[0] = opp;
				pps[1] = pp;
			}
			if (pps[0] != NULL)
				pmap_pv_lock(pps[0]);
			if (pps[1] != NULL)
				pmap_pv_lock(pps[1]);
			opv = _pmap_remove_pv(opp, pm, va, opte);
		} else {
			if (pp != NULL)
				pmap_pv_lock(pp);
		}
		opte = atomic_swap_64(ptep, 0);
	} else {
		if (pp != NULL)
			pmap_pv_lock(pp);
	}

	if (!l3pte_valid(opte))
		_pmap_pdp_addref(pm, pdppa, pdppg);	/* L3 occupancy++ */

	/*
	 * read permission is treated as an access permission internally.
	 * require to add PROT_READ even if only PROT_WRITE or PROT_EXEC
	 */
	if (prot & (VM_PROT_WRITE|VM_PROT_EXECUTE))
		prot |= VM_PROT_READ;
	if (flags & (VM_PROT_WRITE|VM_PROT_EXECUTE))
		flags |= VM_PROT_READ;

	mdattr = VM_PROT_READ | VM_PROT_WRITE;
	if (need_enter_pv) {
		KASSERT(!kenter);
		error = _pmap_enter_pv(pp, pm, &spv, va, ptep, pa, flags);
		if (error != 0) {
			/*
			 * If pmap_enter() fails,
			 * it must not leave behind an existing pmap entry.
			 */
			if (lxpde_valid(opte)) {
				KASSERT((vaddr_t)l3 == trunc_page((vaddr_t)ptep));
				_pmap_pdp_delref(pm, AARCH64_KVA_TO_PA((vaddr_t)l3),
				    true);
				struct pmap_asid_info * const pai = PMAP_PAI(pm,
				    cpu_tlb_info(ci));

				AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);
			}
			PMAP_COUNT(pv_entry_cannotalloc);
			if (flags & PMAP_CANFAIL)
				goto fail1;
			panic("pmap_enter: failed to allocate pv_entry");
		}
	}

	if (pp != NULL) {
		/* update referenced/modified flags */
		KASSERT(!kenter);
		pp->pp_pv.pv_va |= (flags & (VM_PROT_READ | VM_PROT_WRITE));
		mdattr &= (uint32_t)pp->pp_pv.pv_va;
	}

#ifdef PMAPCOUNTERS
	switch (flags & PMAP_CACHE_MASK) {
	case PMAP_NOCACHE:
	case PMAP_NOCACHE_OVR:
		PMAP_COUNT(uncached_mappings);
		break;
	}
#endif

	attr = L3_PAGE | (kenter ? 0 : LX_BLKPAG_NG);
	attr = _pmap_pte_adjust_prot(attr, prot, mdattr, user);
	attr = _pmap_pte_adjust_cacheflags(attr, flags);
	if (VM_MAXUSER_ADDRESS > va && !efirt_p)
		attr |= LX_BLKPAG_APUSER;
	if (flags & PMAP_WIRED)
		attr |= LX_BLKPAG_OS_WIRED;
#ifdef MULTIPROCESSOR
	attr |= LX_BLKPAG_SH_IS;
#endif

	pte = pa | attr;

	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));
	const tlb_asid_t asid = pai->pai_asid;

	if (need_sync_icache) {
		/* non-exec -> exec */
		UVMHIST_LOG(pmaphist,
		    "icache_sync: pm=%p, va=%016lx, pte: %016lx -> %016lx",
		    pm, va, opte, pte);

		if (!l3pte_readable(pte)) {
			PTE_ICACHE_SYNC_PAGE(pte, ptep, asid, va);
			atomic_swap_64(ptep, pte);
			AARCH64_TLBI_BY_ASID_VA(asid, va);
		} else {
			atomic_swap_64(ptep, pte);
			AARCH64_TLBI_BY_ASID_VA(asid, va);
			cpu_icache_sync_range(va, PAGE_SIZE);
		}
	} else {
		atomic_swap_64(ptep, pte);
		AARCH64_TLBI_BY_ASID_VA(asid, va);
	}

	if (pte & LX_BLKPAG_OS_WIRED) {
		_pmap_adj_wired_count(pm, 1);
	}
	_pmap_adj_resident_count(pm, 1);

 fail1:
	if (pps[1] != NULL)
		pmap_pv_unlock(pps[1]);
	if (pps[0] != NULL)
		pmap_pv_unlock(pps[0]);
 fail0:
	if (!kenter) {
		pm_unlock(pm);

		/* spare pv was not used. discard */
		if (spv != NULL)
			pool_cache_put(&_pmap_pv_pool, spv);

		if (opv != NULL)
			pool_cache_put(&_pmap_pv_pool, opv);
	}

	return error;
}

int
pmap_enter(struct pmap *pm, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	return _pmap_enter(pm, va, pa, prot, flags, false);
}


bool
pmap_remove_all(struct pmap *pm)
{
	struct pmap_page *pp;
	struct pv_entry *pv, *pvtmp, *opv, *pvtofree = NULL;
	pt_entry_t pte, *ptep;
	paddr_t pa;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p", pm, 0, 0, 0);

	KASSERT(pm != pmap_kernel());

	UVMHIST_LOG(pmaphist, "pm=%p, asid=%d", pm,
	    PMAP_PAI(pm, cpu_tlb_info(ci))->pai_asid, 0, 0);

	pm_lock(pm);

	LIST_FOREACH_SAFE(pv, &pm->pm_pvlist, pv_proc, pvtmp) {
		ptep = pv->pv_ptep;
		pte = *ptep;

		KASSERTMSG(lxpde_valid(pte),
		    "pte is not valid: pmap=%p, va=%016lx",
		    pm, pv->pv_va);

		pa = lxpde_pa(pte);
		pp = phys_to_pp(pa);

		KASSERTMSG(pp != NULL,
		    "no pmap_page of physical address:%016lx, "
		    "pmap=%p, va=%016lx",
		    pa, pm, pv->pv_va);

		pmap_pv_lock(pp);
		opv = _pmap_remove_pv(pp, pm, trunc_page(pv->pv_va), pte);
		pmap_pv_unlock(pp);
		if (opv != NULL) {
			opv->pv_next = pvtofree;
			pvtofree = opv;
		}
	}
	/* all PTE should now be cleared */
	pm->pm_stats.wired_count = 0;
	pm->pm_stats.resident_count = 0;

	/* clear L0 page table page */
	pmap_zero_page(pm->pm_l0table_pa);

	aarch64_tlbi_by_asid(PMAP_PAI(pm, cpu_tlb_info(ci))->pai_asid);

	/* free L1-L3 page table pages, but not L0 */
	_pmap_free_pdp_all(pm, false);

	pm_unlock(pm);

	for (pv = pvtofree; pv != NULL; pv = pvtmp) {
		pvtmp = pv->pv_next;
		pool_cache_put(&_pmap_pv_pool, pv);
	}

	return true;
}

static void
_pmap_remove(struct pmap *pm, vaddr_t sva, vaddr_t eva, bool kremove,
    struct pv_entry **pvtofree)
{
	pt_entry_t pte, *ptep = NULL;
	struct pmap_page *pp;
	struct pv_entry *opv;
	paddr_t pa;
	vaddr_t va;
	vsize_t blocksize = 0;
	bool pdpremoved;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, sva=%016lx, eva=%016lx, kremove=%d",
	    pm, sva, eva, kremove);

	KASSERT(kremove || mutex_owned(&pm->pm_lock));

	for (va = sva; (va < eva) && (pm->pm_stats.resident_count != 0);
	    va = (va + blocksize) & ~(blocksize - 1)) {

		/* va is belong to the same L3 table as before? */
		if ((blocksize == L3_SIZE) && ((va & L3INDEXMASK) != 0))
			ptep++;
		else
			ptep = _pmap_pte_lookup_bs(pm, va, &blocksize);

		pte = *ptep;
		if (!lxpde_valid(pte))
			continue;

		if (!kremove) {
			pa = lxpde_pa(pte);
			pp = phys_to_pp(pa);
			if (pp != NULL) {

				pmap_pv_lock(pp);
				opv = _pmap_remove_pv(pp, pm, va, pte);
				pmap_pv_unlock(pp);
				if (opv != NULL) {
					opv->pv_next = *pvtofree;
					*pvtofree = opv;
				}
			}
		}

		pte = atomic_swap_64(ptep, 0);
		if (!lxpde_valid(pte))
			continue;
		struct pmap_asid_info * const pai = PMAP_PAI(pm,
		    cpu_tlb_info(ci));

		pdpremoved = _pmap_pdp_delref(pm,
		    AARCH64_KVA_TO_PA(trunc_page((vaddr_t)ptep)), true);
		AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);

		if (pdpremoved) {
			/*
			 * this Ln page table page has been removed.
			 * skip to next Ln table
			 */
			blocksize *= Ln_ENTRIES;
		}

		if ((pte & LX_BLKPAG_OS_WIRED) != 0) {
			_pmap_adj_wired_count(pm, -1);
		}
		_pmap_adj_resident_count(pm, -1);
	}
}

void
pmap_remove(struct pmap *pm, vaddr_t sva, vaddr_t eva)
{
	struct pv_entry *pvtofree = NULL;
	struct pv_entry *pv, *pvtmp;

	KASSERT_PM_ADDR(pm, sva);
	KASSERT(!IN_DIRECTMAP_ADDR(sva));

	pm_lock(pm);
	_pmap_remove(pm, sva, eva, false, &pvtofree);
	pm_unlock(pm);

	for (pv = pvtofree; pv != NULL; pv = pvtmp) {
		pvtmp = pv->pv_next;
		pool_cache_put(&_pmap_pv_pool, pv);
	}
}

static void
pmap_page_remove(struct pmap_page *pp, vm_prot_t prot)
{
	struct pv_entry *pv, *pvtmp;
	struct pv_entry *pvtofree = NULL;
	struct pmap *pm;
	pt_entry_t opte;

	/* remove all pages reference to this physical page */
	pmap_pv_lock(pp);
	for (pv = &pp->pp_pv; pv != NULL;) {
		if ((pm = pv->pv_pmap) == NULL) {
			KASSERT(pv == &pp->pp_pv);
			pv = pp->pp_pv.pv_next;
			continue;
		}
		if (!pm_reverse_lock(pm, pp)) {
			/* now retry */
			pv = &pp->pp_pv;
			continue;
		}
		opte = atomic_swap_64(pv->pv_ptep, 0);
		struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));
		const vaddr_t va = trunc_page(pv->pv_va);

		if (lxpde_valid(opte)) {
			_pmap_pdp_delref(pm,
			    AARCH64_KVA_TO_PA(trunc_page(
			    (vaddr_t)pv->pv_ptep)), false);
			AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);

			if ((opte & LX_BLKPAG_OS_WIRED) != 0) {
				_pmap_adj_wired_count(pm, -1);
			}
			_pmap_adj_resident_count(pm, -1);
		}
		pvtmp = _pmap_remove_pv(pp, pm, va, opte);
		if (pvtmp == NULL) {
			KASSERT(pv == &pp->pp_pv);
		} else {
			KASSERT(pv == pvtmp);
			KASSERT(pp->pp_pv.pv_next == pv->pv_next);
			pv->pv_next = pvtofree;
			pvtofree = pv;
		}
		pm_unlock(pm);
		pv = pp->pp_pv.pv_next;
	}
	pmap_pv_unlock(pp);

	for (pv = pvtofree; pv != NULL; pv = pvtmp) {
		pvtmp = pv->pv_next;
		pool_cache_put(&_pmap_pv_pool, pv);
	}
}

#ifdef __HAVE_PMAP_PV_TRACK
void
pmap_pv_protect(paddr_t pa, vm_prot_t prot)
{
	struct pmap_page *pp;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pa=%016lx, prot=%08x", pa, prot, 0, 0);

	pp = pmap_pv_tracked(pa);
	if (pp == NULL)
		panic("pmap_pv_protect: page not pv-tracked: %#" PRIxPADDR, pa);

	KASSERT(prot == VM_PROT_NONE);
	pmap_page_remove(pp, prot);
}
#endif

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct pv_entry *pv;
	struct pmap_page *pp;
	struct pmap *pm;

	KASSERT((prot & VM_PROT_READ) || !(prot & VM_PROT_WRITE));

	pp = VM_PAGE_TO_PP(pg);

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pg=%p, pp=%p, pa=%016lx, prot=%08x",
	    pg, pp, VM_PAGE_TO_PHYS(pg), prot);

	/* do an unlocked check first */
	if (atomic_load_relaxed(&pp->pp_pv.pv_pmap) == NULL &&
	    atomic_load_relaxed(&pp->pp_pv.pv_next) == NULL) {
		return;
	}

	if ((prot & (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)) ==
	    VM_PROT_NONE) {
		pmap_page_remove(pp, prot);
	} else {
		pmap_pv_lock(pp);
		pv = &pp->pp_pv;
		while (pv != NULL) {
			if ((pm = pv->pv_pmap) == NULL) {
				KASSERT(pv == &pp->pp_pv);
				pv = pv->pv_next;
				continue;
			}
			if (!pm_reverse_lock(pm, pp)) {
				/* retry */
				pv = &pp->pp_pv;
				continue;
			}
			_pmap_protect_pv(pp, pv, prot);
			pm_unlock(pm);
			pv = pv->pv_next;
		}
		pmap_pv_unlock(pp);
	}
}

void
pmap_unwire(struct pmap *pm, vaddr_t va)
{
	pt_entry_t pte, *ptep;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, va=%016lx", pm, va, 0, 0);

	PMAP_COUNT(unwire);

	KASSERT_PM_ADDR(pm, va);
	KASSERT(!IN_DIRECTMAP_ADDR(va));

	pm_lock(pm);
	ptep = _pmap_pte_lookup_l3(pm, va);
	if (ptep != NULL) {
		pte = *ptep;
		if (!l3pte_valid(pte) ||
		    ((pte & LX_BLKPAG_OS_WIRED) == 0)) {
			/* invalid pte, or pte is not wired */
			PMAP_COUNT(unwire_failure);
			pm_unlock(pm);
			return;
		}

		pte &= ~LX_BLKPAG_OS_WIRED;
		atomic_swap_64(ptep, pte);

		_pmap_adj_wired_count(pm, -1);
	}
	pm_unlock(pm);
}

bool
pmap_fault_fixup(struct pmap *pm, vaddr_t va, vm_prot_t accessprot, bool user)
{
	struct pmap_page *pp;
	pt_entry_t *ptep, pte;
	vm_prot_t pmap_prot;
	paddr_t pa;
	bool fixed = false;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm=%p, va=%016lx, accessprot=%08x",
	    pm, va, accessprot, 0);

#if 0
	KASSERT_PM_ADDR(pm, va);
#else
	if (((pm == pmap_kernel()) &&
	    !(IN_RANGE(va, VM_MIN_KERNEL_ADDRESS, VM_MAX_KERNEL_ADDRESS))) ||
	    ((pm != pmap_kernel()) &&
	    !(IN_RANGE(va, VM_MIN_ADDRESS, VM_MAX_ADDRESS)))) {

		UVMHIST_LOG(pmaphist,
		    "pmap space and va mismatch: kernel=%jd, va=%016lx",
		    pm == pmap_kernel(), va, 0, 0);
		return false;
	}
#endif

	pm_lock(pm);

	ptep = _pmap_pte_lookup_l3(pm, va);
	if (ptep == NULL) {
		UVMHIST_LOG(pmaphist, "pte_lookup failure: va=%016lx",
		    va, 0, 0, 0);
		goto done;
	}

	pte = *ptep;
	if (!l3pte_valid(pte)) {
		UVMHIST_LOG(pmaphist, "invalid pte: %016llx: va=%016lx",
		    pte, va, 0, 0);
		goto done;
	}

	pa = l3pte_pa(*ptep);
	pp = phys_to_pp(pa);
	if (pp == NULL) {
		UVMHIST_LOG(pmaphist, "pmap_page not found: va=%016lx", va, 0, 0, 0);
		goto done;
	}

	/*
	 * Get the prot specified by pmap_enter().
	 * A valid pte is considered a readable page.
	 * If DBM is 1, it is considered a writable page.
	 */
	pmap_prot = VM_PROT_READ;
	if ((pte & (LX_BLKPAG_OS_WRITE|LX_BLKPAG_DBM)) != 0)
		pmap_prot |= VM_PROT_WRITE;

	if (l3pte_executable(pte, pm != pmap_kernel()))
		pmap_prot |= VM_PROT_EXECUTE;

	UVMHIST_LOG(pmaphist, "va=%016lx, pmapprot=%08x, accessprot=%08x",
	    va, pmap_prot, accessprot, 0);

	/* ignore except read/write */
	accessprot &= (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* PROT_EXEC requires implicit PROT_READ */
	if (accessprot & VM_PROT_EXECUTE)
		accessprot |= VM_PROT_READ;

	/* no permission to read/write/execute for this page */
	if ((pmap_prot & accessprot) != accessprot) {
		UVMHIST_LOG(pmaphist, "no permission to access", 0, 0, 0, 0);
		goto done;
	}

	/* pte is readable and writable, but occurred fault? probably copy(9) */
	if ((pte & LX_BLKPAG_AF) && ((pte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RW))
		goto done;

	pmap_pv_lock(pp);
	if ((pte & LX_BLKPAG_AF) == 0) {
		/* pte has no AF bit, set referenced and AF bit */
		UVMHIST_LOG(pmaphist,
		    "REFERENCED:"
		    " va=%016lx, pa=%016lx, pte_prot=%08x, accessprot=%08x",
		    va, pa, pmap_prot, accessprot);
		pp->pp_pv.pv_va |= VM_PROT_READ;	/* set referenced */
		pte |= LX_BLKPAG_AF;

		PMAP_COUNT(fixup_referenced);
	}
	if ((accessprot & VM_PROT_WRITE) &&
	    ((pte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RO)) {
		/* pte is not RW. set modified and RW */

		UVMHIST_LOG(pmaphist, "MODIFIED:"
		    " va=%016lx, pa=%016lx, pte_prot=%08x, accessprot=%08x",
		    va, pa, pmap_prot, accessprot);
		pp->pp_pv.pv_va |= VM_PROT_WRITE;	/* set modified */
		pte &= ~LX_BLKPAG_AP;
		pte |= LX_BLKPAG_AP_RW;

		PMAP_COUNT(fixup_modified);
	}
	pmap_pv_unlock(pp);

	atomic_swap_64(ptep, pte);
	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));
	AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);

	fixed = true;

 done:
	pm_unlock(pm);
	return fixed;
}

bool
pmap_clear_modify(struct vm_page *pg)
{
	struct pv_entry *pv;
	struct pmap_page * const pp = VM_PAGE_TO_PP(pg);
	pt_entry_t *ptep, pte, opte;
	vaddr_t va;
#ifdef ARMV81_HAFDBS
	bool modified;
#endif

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pg=%p, flags=%08x",
	    pg, (int)(pp->pp_pv.pv_va & (PAGE_SIZE - 1)), 0, 0);

	PMAP_COUNT(clear_modify);

	/*
	 * if this is a new page, assert it has no mappings and simply zap
	 * the stored attributes without taking any locks.
	 */
	if ((pg->flags & PG_FAKE) != 0) {
		KASSERT(atomic_load_relaxed(&pp->pp_pv.pv_pmap) == NULL);
		KASSERT(atomic_load_relaxed(&pp->pp_pv.pv_next) == NULL);
		atomic_store_relaxed(&pp->pp_pv.pv_va, 0);
		return false;
	}

	pmap_pv_lock(pp);

	if (
#ifdef ARMV81_HAFDBS
	    aarch64_hafdbs_enabled != ID_AA64MMFR1_EL1_HAFDBS_AD &&
#endif
	    (pp->pp_pv.pv_va & VM_PROT_WRITE) == 0) {
		pmap_pv_unlock(pp);
		return false;
	}
#ifdef ARMV81_HAFDBS
	modified = ((pp->pp_pv.pv_va & VM_PROT_WRITE) != 0);
#endif
	pp->pp_pv.pv_va &= ~(vaddr_t)VM_PROT_WRITE;

	for (pv = &pp->pp_pv; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap == NULL) {
			KASSERT(pv == &pp->pp_pv);
			continue;
		}

		PMAP_COUNT(clear_modify_pages);

		va = trunc_page(pv->pv_va);

		ptep = pv->pv_ptep;
		opte = pte = *ptep;
 tryagain:
		if (!l3pte_valid(pte))
			continue;
		if ((pte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RO)
			continue;
#ifdef ARMV81_HAFDBS
		modified = true;
#endif
		/* clear write permission */
		pte &= ~LX_BLKPAG_AP;
		pte |= LX_BLKPAG_AP_RO;

		/* XXX: possible deadlock if using PM_LOCK(). this is racy */
		if ((pte = atomic_cas_64(ptep, opte, pte)) != opte) {
			opte = pte;
			goto tryagain;
		}

		struct pmap * const pm = pv->pv_pmap;
		struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));
		AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);

		UVMHIST_LOG(pmaphist,
		    "va=%016llx, ptep=%p, pa=%016lx, RW -> RO",
		    va, ptep, l3pte_pa(pte), 0);
	}

	pmap_pv_unlock(pp);

#ifdef ARMV81_HAFDBS
	return modified;
#else
	return true;
#endif
}

bool
pmap_clear_reference(struct vm_page *pg)
{
	struct pv_entry *pv;
	struct pmap_page * const pp = VM_PAGE_TO_PP(pg);
	pt_entry_t *ptep, pte, opte;
	vaddr_t va;
#ifdef ARMV81_HAFDBS
	bool referenced;
#endif

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pg=%p, pp=%p, flags=%08x",
	    pg, pp, (int)(pp->pp_pv.pv_va & (PAGE_SIZE - 1)), 0);

	pmap_pv_lock(pp);

	if (
#ifdef ARMV81_HAFDBS
	    aarch64_hafdbs_enabled == ID_AA64MMFR1_EL1_HAFDBS_NONE &&
#endif
	    (pp->pp_pv.pv_va & VM_PROT_READ) == 0) {
		pmap_pv_unlock(pp);
		return false;
	}
#ifdef ARMV81_HAFDBS
	referenced = ((pp->pp_pv.pv_va & VM_PROT_READ) != 0);
#endif
	pp->pp_pv.pv_va &= ~(vaddr_t)VM_PROT_READ;

	PMAP_COUNT(clear_reference);
	for (pv = &pp->pp_pv; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap == NULL) {
			KASSERT(pv == &pp->pp_pv);
			continue;
		}

		PMAP_COUNT(clear_reference_pages);

		va = trunc_page(pv->pv_va);

		ptep = pv->pv_ptep;
		opte = pte = *ptep;
 tryagain:
		if (!l3pte_valid(pte))
			continue;
		if ((pte & LX_BLKPAG_AF) == 0)
			continue;
#ifdef ARMV81_HAFDBS
		referenced = true;
#endif
		/* clear access permission */
		pte &= ~LX_BLKPAG_AF;

		/* XXX: possible deadlock if using PM_LOCK(). this is racy */
		if ((pte = atomic_cas_64(ptep, opte, pte)) != opte) {
			opte = pte;
			goto tryagain;
		}

		struct pmap * const pm = pv->pv_pmap;
		struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));
		AARCH64_TLBI_BY_ASID_VA(pai->pai_asid, va);

		UVMHIST_LOG(pmaphist, "va=%016llx, ptep=%p, pa=%016lx, unse AF",
		    va, ptep, l3pte_pa(pte), 0);
	}

	pmap_pv_unlock(pp);

#ifdef ARMV81_HAFDBS
	return referenced;
#else
	return true;
#endif
}

bool
pmap_is_modified(struct vm_page *pg)
{
	struct pmap_page * const pp = VM_PAGE_TO_PP(pg);

	if (pp->pp_pv.pv_va & VM_PROT_WRITE)
		return true;

#ifdef ARMV81_HAFDBS
	/* check hardware dirty flag on each pte */
	if (aarch64_hafdbs_enabled == ID_AA64MMFR1_EL1_HAFDBS_AD) {
		struct pv_entry *pv;
		pt_entry_t *ptep, pte;

		pmap_pv_lock(pp);
		for (pv = &pp->pp_pv; pv != NULL; pv = pv->pv_next) {
			if (pv->pv_pmap == NULL) {
				KASSERT(pv == &pp->pp_pv);
				continue;
			}

			ptep = pv->pv_ptep;
			pte = *ptep;
			if (!l3pte_valid(pte))
				continue;

			if ((pte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RW) {
				pp->pp_pv.pv_va |= VM_PROT_WRITE;
				pmap_pv_unlock(pp);
				return true;
			}
		}
		pmap_pv_unlock(pp);
	}
#endif

	return false;
}

bool
pmap_is_referenced(struct vm_page *pg)
{
	struct pmap_page * const pp = VM_PAGE_TO_PP(pg);

	if (pp->pp_pv.pv_va & VM_PROT_READ)
		return true;

#ifdef ARMV81_HAFDBS
	/* check hardware access flag on each pte */
	if (aarch64_hafdbs_enabled != ID_AA64MMFR1_EL1_HAFDBS_NONE) {
		struct pv_entry *pv;
		pt_entry_t *ptep, pte;

		pmap_pv_lock(pp);
		for (pv = &pp->pp_pv; pv != NULL; pv = pv->pv_next) {
			if (pv->pv_pmap == NULL) {
				KASSERT(pv == &pp->pp_pv);
				continue;
			}

			ptep = pv->pv_ptep;
			pte = *ptep;
			if (!l3pte_valid(pte))
				continue;

			if (pte & LX_BLKPAG_AF) {
				pp->pp_pv.pv_va |= VM_PROT_READ;
				pmap_pv_unlock(pp);
				return true;
			}
		}
		pmap_pv_unlock(pp);
	}
#endif

	return false;
}

/* get pointer to kernel segment L2 or L3 table entry */
pt_entry_t *
kvtopte(vaddr_t va)
{
	KASSERT(IN_RANGE(va, VM_MIN_KERNEL_ADDRESS, VM_MAX_KERNEL_ADDRESS));

	return _pmap_pte_lookup_bs(pmap_kernel(), va, NULL);
}

#ifdef DDB

void
pmap_db_pmap_print(struct pmap *pm,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));

	pr(" pm_asid       = %d\n", pai->pai_asid);
	pr(" pm_l0table    = %p\n", pm->pm_l0table);
	pr(" pm_l0table_pa = %lx\n", pm->pm_l0table_pa);
	pr(" pm_activated  = %d\n\n", pm->pm_activated);
}
#endif /* DDB */

