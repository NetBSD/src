/*	$NetBSD: pmap_machdep.c,v 1.2 2022/12/21 11:39:45 skrll Exp $	*/

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

#include "opt_arm_debug.h"
#include "opt_efi.h"
#include "opt_multiprocessor.h"
#include "opt_uvmhist.h"

#define __PMAP_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_machdep.c,v 1.2 2022/12/21 11:39:45 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>
#include <uvm/pmap/pmap_pvt.h>

#include <aarch64/cpufunc.h>

#include <arm/locore.h>

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

/* Set to LX_BLKPAG_GP if supported. */
uint64_t pmap_attr_gp = 0;

/*
 * Misc variables
 */
vaddr_t virtual_avail;
vaddr_t virtual_end;

bool pmap_devmap_bootstrap_done = false;

paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa) == false)
		return 0;
	return pa;
}

bool
pmap_extract_coherency(pmap_t pm, vaddr_t va, paddr_t *pap, bool *coherentp)
{
	paddr_t pa;
	bool coherency = false;

	if (pm == pmap_kernel()) {
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
	const pt_entry_t * const ptep = pmap_pte_lookup(pm, va);
	pt_entry_t pte;

	if (ptep == NULL || !pte_valid_p(pte = *ptep)) {
		kpreempt_enable();
		return false;
	}
	kpreempt_enable();

	pa = pte_to_paddr(pte) | (va & PGOFSET);

	switch (pte & LX_BLKPAG_ATTR_MASK) {
	case LX_BLKPAG_ATTR_NORMAL_NC:
	case LX_BLKPAG_ATTR_DEVICE_MEM:
	case LX_BLKPAG_ATTR_DEVICE_MEM_NP:
		coherency = true;
		break;
	}

 done:
	if (pap != NULL) {
		*pap = pa;
	}
	if (coherentp != NULL) {
		*coherentp = coherency;
	}
	return true;
}


bool
pmap_fault_fixup(pmap_t pm, vaddr_t va, vm_prot_t ftype, bool user)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);

	KASSERT(!user || (pm != pmap_kernel()));

	UVMHIST_LOG(pmaphist, " pm=%#jx, va=%#jx, ftype=%#jx, user=%jd",
	    (uintptr_t)pm, va, ftype, user);
	UVMHIST_LOG(pmaphist, " ti=%#jx pai=%#jx asid=%#jx",
	    (uintptr_t)cpu_tlb_info(curcpu()),
	    (uintptr_t)PMAP_PAI(pm, cpu_tlb_info(curcpu())),
	    (uintptr_t)PMAP_PAI(pm, cpu_tlb_info(curcpu()))->pai_asid, 0);

	kpreempt_disable();

	bool fixed = false;
	pt_entry_t * const ptep = pmap_pte_lookup(pm, va);
	if (ptep == NULL) {
		UVMHIST_LOG(pmaphist, "... no ptep", 0, 0, 0, 0);
		goto done;
	}

	const pt_entry_t opte = *ptep;
	if (!l3pte_valid(opte)) {
		UVMHIST_LOG(pmaphist, "invalid pte: %016llx: va=%016lx",
		    opte, va, 0, 0);
		goto done;
	}

	const paddr_t pa = l3pte_pa(opte);
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	if (pg == NULL) {
		UVMHIST_LOG(pmaphist, "pg not found: va=%016lx", va, 0, 0, 0);
		goto done;
	}

	struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
	UVMHIST_LOG(pmaphist, " pg=%#jx, opte=%#jx, ptep=%#jx", (uintptr_t)pg,
	    opte, (uintptr_t)ptep, 0);

	if ((ftype & VM_PROT_WRITE) && (opte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RW) {
		/*
		 * This looks like a good candidate for "page modified"
		 * emulation...
		 */
		pmap_page_set_attributes(mdpg, VM_PAGEMD_MODIFIED | VM_PAGEMD_REFERENCED);

		/*
		 * Enable write permissions for the page by setting the Access Flag.
		 */
		// XXXNH LX_BLKPAG_OS_0?
		const pt_entry_t npte = opte | LX_BLKPAG_AF | LX_BLKPAG_OS_0;
		atomic_swap_64(ptep, npte);
		dsb(ishst);
		fixed = true;

		UVMHIST_LOG(pmaphist, " <-- done (mod emul: changed pte "
		    "from %#jx to %#jx)", opte, npte, 0, 0);
	} else if ((ftype & VM_PROT_READ) && (opte & LX_BLKPAG_AP) == LX_BLKPAG_AP_RO) {
		/*
		 * This looks like a good candidate for "page referenced"
		 * emulation.
		 */

		pmap_page_set_attributes(mdpg, VM_PAGEMD_REFERENCED);

		/*
		 * Enable write permissions for the page by setting the Access Flag.
		 */
		const pt_entry_t npte = opte | LX_BLKPAG_AF;
		atomic_swap_64(ptep, npte);
		dsb(ishst);
		fixed = true;

		UVMHIST_LOG(pmaphist, " <-- done (ref emul: changed pte "
		    "from %#jx to %#jx)", opte, npte, 0, 0);
	}

done:
	kpreempt_enable();

	return fixed;
}


void
pmap_icache_sync_range(pmap_t pm, vaddr_t sva, vaddr_t eva)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, "pm %#jx sva %#jx eva %#jx",
	   (uintptr_t)pm, sva, eva, 0);

	KASSERT((sva & PAGE_MASK) == 0);
	KASSERT((eva & PAGE_MASK) == 0);

	pmap_lock(pm);

	for (vaddr_t va = sva; va < eva; va += PAGE_SIZE) {
		pt_entry_t * const ptep = pmap_pte_lookup(pm, va);
		if (ptep == NULL)
			continue;

		pt_entry_t opte = *ptep;
		if (!l3pte_valid(opte)) {
			UVMHIST_LOG(pmaphist, "invalid pte: %016llx: va=%016lx",
			    opte, va, 0, 0);
			goto done;
		}

		if (l3pte_readable(opte)) {
			cpu_icache_sync_range(va, PAGE_SIZE);
		} else {
			/*
			 * change to accessible temporarily
			 * to do cpu_icache_sync_range()
			 */
			struct pmap_asid_info * const pai = PMAP_PAI(pm,
			    cpu_tlb_info(ci));

			atomic_swap_64(ptep, opte | LX_BLKPAG_AF);
			// tlb_invalidate_addr does the dsb(ishst);
			tlb_invalidate_addr(pai->pai_asid, va);
			cpu_icache_sync_range(va, PAGE_SIZE);
			atomic_swap_64(ptep, opte);
			tlb_invalidate_addr(pai->pai_asid, va);
		}
	}
done:
	pmap_unlock(pm);
}


struct vm_page *
pmap_md_alloc_poolpage(int flags)
{

	/*
	 * Any managed page works for us.
	 */
	return uvm_pagealloc(NULL, 0, NULL, flags);
}


vaddr_t
pmap_md_map_poolpage(paddr_t pa, size_t len)
{
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	const vaddr_t va = pmap_md_direct_map_paddr(pa);
	KASSERT(cold || pg != NULL);

	if (pg != NULL) {
		struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
		const pv_entry_t pv = &mdpg->mdpg_first;
		const vaddr_t last_va = trunc_page(pv->pv_va);

		KASSERT(len == PAGE_SIZE || last_va == pa);
		KASSERT(pv->pv_pmap == NULL);
		KASSERT(pv->pv_next == NULL);
		KASSERT(!VM_PAGEMD_EXECPAGE_P(mdpg));

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

	KASSERT(!VM_PAGEMD_EXECPAGE_P(mdpg));

	const pv_entry_t pv = &mdpg->mdpg_first;

	/* Note last mapped address for future color check */
	pv->pv_va = va;

	KASSERT(pv->pv_pmap == NULL);
	KASSERT(pv->pv_next == NULL);

	return pa;
}


bool
pmap_md_direct_mapped_vaddr_p(vaddr_t va)
{

	if (!AARCH64_KVA_P(va))
		return false;

	paddr_t pa = AARCH64_KVA_TO_PA(va);
	if (physical_start <= pa && pa < physical_end)
		return true;

	return false;
}


paddr_t
pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t va)
{

	return AARCH64_KVA_TO_PA(va);
}


vaddr_t
pmap_md_direct_map_paddr(paddr_t pa)
{

	return AARCH64_PA_TO_KVA(pa);
}


bool
pmap_md_io_vaddr_p(vaddr_t va)
{

	if (pmap_devmap_find_va(va, PAGE_SIZE)) {
		return true;
	}
	return false;
}


static void
pmap_md_grow(pmap_pdetab_t *ptb, vaddr_t va, vsize_t vshift,
    vsize_t *remaining)
{
	KASSERT((va & (NBSEG - 1)) == 0);
	const vaddr_t pdetab_mask = PMAP_PDETABSIZE - 1;
	const vsize_t vinc = 1UL << vshift;

	for (size_t i = (va >> vshift) & pdetab_mask;
	    i < PMAP_PDETABSIZE; i++, va += vinc) {
		pd_entry_t * const pde_p =
		    &ptb->pde_pde[(va >> vshift) & pdetab_mask];

		vaddr_t pdeva;
		if (pte_pde_valid_p(*pde_p)) {
			const paddr_t pa = pte_pde_to_paddr(*pde_p);
			pdeva = pmap_md_direct_map_paddr(pa);
		} else {
			/*
			 * uvm_pageboot_alloc() returns a direct mapped address
			 */
			pdeva = uvm_pageboot_alloc(Ln_TABLE_SIZE);
			paddr_t pdepa = AARCH64_KVA_TO_PA(pdeva);
			*pde_p = pte_pde_pdetab(pdepa, true);
			memset((void *)pdeva, 0, PAGE_SIZE);
		}

		if (vshift > SEGSHIFT) {
			pmap_md_grow((pmap_pdetab_t *)pdeva, va,
			    vshift - SEGLENGTH, remaining);
		} else {
			if (*remaining > vinc)
				*remaining -= vinc;
			else
				*remaining = 0;
		}
		if (*remaining == 0)
			return;
	}
}


void
pmap_bootstrap(vaddr_t vstart, vaddr_t vend)
{
	pmap_t pm = pmap_kernel();

	/*
	 * Initialise the kernel pmap object
	 */
	curcpu()->ci_pmap_cur = pm;

	virtual_avail = vstart;
	virtual_end = vend;

	aarch64_tlbi_all();

	pm->pm_l0_pa = __SHIFTOUT(reg_ttbr1_el1_read(), TTBR_BADDR);
	pm->pm_pdetab = (pmap_pdetab_t *)AARCH64_PA_TO_KVA(pm->pm_l0_pa);

	VPRINTF("common ");
	pmap_bootstrap_common();

	VPRINTF("tlb0 ");
	pmap_tlb_info_init(&pmap_tlb0_info);

#ifdef MULTIPROCESSOR
	VPRINTF("kcpusets ");

	kcpuset_create(&pm->pm_onproc, true);
	kcpuset_create(&pm->pm_active, true);
	KASSERT(pm->pm_onproc != NULL);
	KASSERT(pm->pm_active != NULL);
	kcpuset_set(pm->pm_onproc, cpu_number());
	kcpuset_set(pm->pm_active, cpu_number());
#endif

	VPRINTF("nkmempages ");
	/*
	 * Compute the number of pages kmem_arena will have.  This will also
	 * be called by uvm_km_bootstrap later, but that doesn't matter
	 */
	kmeminit_nkmempages();

	/* Get size of buffer cache and set an upper limit */
	buf_setvalimit((VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / 8);
	vsize_t bufsz = buf_memcalc();
	buf_setvalimit(bufsz);

	vsize_t kvmsize = (VM_PHYS_SIZE + (ubc_nwins << ubc_winshift) +
	    bufsz + 16 * NCARGS + pager_map_size) +
	    /*(maxproc * UPAGES) + */nkmempages * NBPG;

#ifdef SYSVSHM
	kvmsize += shminfo.shmall;
#endif

	/* Calculate VA address space and roundup to NBSEG tables */
	kvmsize = roundup(kvmsize, NBSEG);

	/*
	 * Initialize `FYI' variables.	Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.  Must do this before uvm_pageboot_alloc()
	 * can be called.
	 */
	pmap_limits.avail_start = ptoa(uvm_physseg_get_start(uvm_physseg_get_first()));
	pmap_limits.avail_end = ptoa(uvm_physseg_get_end(uvm_physseg_get_last()));

	/*
	 * Update the naive settings in pmap_limits to the actual KVA range.
	 */
	pmap_limits.virtual_start = vstart;
	pmap_limits.virtual_end = vend;

	VPRINTF("\nlimits: %" PRIxVADDR " - %" PRIxVADDR "\n", vstart, vend);

	const vaddr_t kvmstart = vstart;
	pmap_curmaxkvaddr = vstart + kvmsize;

	VPRINTF("kva   : %" PRIxVADDR " - %" PRIxVADDR "\n", kvmstart,
	    pmap_curmaxkvaddr);

	pmap_md_grow(pmap_kernel()->pm_pdetab, kvmstart, XSEGSHIFT, &kvmsize);

#if defined(EFI_RUNTIME)
	vaddr_t efi_l0va = uvm_pageboot_alloc(Ln_TABLE_SIZE);
	KASSERT((efi_l0va & PAGE_MASK) == 0);

	pmap_t efipm = pmap_efirt();
	efipm->pm_l0_pa = AARCH64_KVA_TO_PA(efi_l0va);
	efipm->pm_pdetab = (pmap_pdetab_t *)efi_l0va;

#endif

	pool_init(&pmap_pmap_pool, PMAP_SIZE, 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);

	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
#ifdef KASAN
	    NULL,
#else
	    &pmap_pv_page_allocator,
#endif
	    IPL_NONE);

	pmap_pvlist_lock_init(/*arm_dcache_align*/ 128);

	VPRINTF("done\n");
}


void
pmap_md_xtab_activate(pmap_t pm, struct lwp *l)
{
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pmaphist, " (pm=%#jx l=%#jx)", (uintptr_t)pm, (uintptr_t)l, 0, 0);

	/*
	 * Assume that TTBR1 has only global mappings and TTBR0 only
	 * has non-global mappings.  To prevent speculation from doing
	 * evil things we disable translation table walks using TTBR0
	 * before setting the CONTEXTIDR (ASID) or new TTBR0 value.
	 * Once both are set, table walks are reenabled.
	 */

	const uint64_t old_tcrel1 = reg_tcr_el1_read();
	reg_tcr_el1_write(old_tcrel1 | TCR_EPD0);
	isb();

	struct cpu_info * const ci = curcpu();
	struct pmap_asid_info * const pai = PMAP_PAI(pm, cpu_tlb_info(ci));

	const uint64_t ttbr =
	    __SHIFTIN(pai->pai_asid, TTBR_ASID) |
	    __SHIFTIN(pm->pm_l0_pa, TTBR_BADDR);

	cpu_set_ttbr0(ttbr);

	if (pm != pmap_kernel()) {
		reg_tcr_el1_write(old_tcrel1 & ~TCR_EPD0);
	}

	UVMHIST_LOG(maphist, " pm %#jx pm->pm_l0 %016jx pm->pm_l0_pa %016jx asid %ju... done",
	    (uintptr_t)pm, (uintptr_t)pm->pm_pdetab, (uintptr_t)pm->pm_l0_pa,
	    (uintptr_t)pai->pai_asid);

	KASSERTMSG(ci->ci_pmap_asid_cur == pai->pai_asid, "%u vs %u",
	    ci->ci_pmap_asid_cur, pai->pai_asid);
	ci->ci_pmap_cur = pm;
}


void
pmap_md_xtab_deactivate(pmap_t pm)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	struct cpu_info * const ci = curcpu();
	/*
	 * Disable translation table walks from TTBR0 while no pmap has been
	 * activated.
	 */
	const uint64_t old_tcrel1 = reg_tcr_el1_read();
	reg_tcr_el1_write(old_tcrel1 | TCR_EPD0);
	isb();

	cpu_set_ttbr0(0);

	ci->ci_pmap_cur = pmap_kernel();
	KASSERTMSG(ci->ci_pmap_asid_cur == KERNEL_PID, "ci_pmap_asid_cur %u",
	    ci->ci_pmap_asid_cur);
}


#if defined(EFI_RUNTIME)
void
pmap_md_activate_efirt(void)
{
	kpreempt_disable();

	pmap_md_xtab_activate(pmap_efirt(), NULL);
}
void
pmap_md_deactivate_efirt(void)
{
	pmap_md_xtab_deactivate(pmap_efirt());

	kpreempt_enable();
}
#endif


void
pmap_md_pdetab_init(struct pmap *pm)
{

	KASSERT(pm != NULL);

	pmap_extract(pmap_kernel(), (vaddr_t)pm->pm_pdetab, &pm->pm_l0_pa);
}

void
pmap_md_pdetab_fini(struct pmap *pm)
{

	KASSERT(pm != NULL);
}


void
pmap_md_page_syncicache(struct vm_page_md *mdpg, const kcpuset_t *onproc)
{
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmaphist);

	//XXXNH
}


bool
pmap_md_ok_to_steal_p(const uvm_physseg_t bank, size_t npgs)
{

	return true;
}


pd_entry_t *
pmap_l0table(struct pmap *pm)
{

	return pm->pm_pdetab->pde_pde;
}


static const struct pmap_devmap *pmap_devmap_table;
vaddr_t virtual_devmap_addr;

#define	L1_BLK_MAPPABLE_P(va, pa, size)					\
    ((((va) | (pa)) & L1_OFFSET) == 0 && (size) >= L1_SIZE)

#define	L2_BLK_MAPPABLE_P(va, pa, size)					\
    ((((va) | (pa)) & L2_OFFSET) == 0 && (size) >= L2_SIZE)


static vsize_t
pmap_map_chunk(vaddr_t va, paddr_t pa, vsize_t size,
    vm_prot_t prot, u_int flags)
{
	pt_entry_t attr;
	psize_t blocksize;

	vsize_t resid = round_page(size);
	vsize_t mapped = 0;

	while (resid > 0) {
		if (L1_BLK_MAPPABLE_P(va, pa, resid)) {
			blocksize = L1_SIZE;
			attr = L1_BLOCK;
		} else if (L2_BLK_MAPPABLE_P(va, pa, resid)) {
			blocksize = L2_SIZE;
			attr = L2_BLOCK;
		} else {
			blocksize = L3_SIZE;
			attr = L3_PAGE;
		}

		pt_entry_t pte = pte_make_kenter_pa(pa, NULL, prot, flags);
		pte &= ~LX_TYPE;
		attr |= pte;

		pmapboot_enter(va, pa, blocksize, blocksize, attr, NULL);

		va += blocksize;
		pa += blocksize;
		resid -= blocksize;
		mapped += blocksize;
	}

	return mapped;
}


void
pmap_devmap_register(const struct pmap_devmap *table)
{
	pmap_devmap_table = table;
}


void
pmap_devmap_bootstrap(vaddr_t l0pt, const struct pmap_devmap *table)
{
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
		if ((virtual_devmap_addr == 0) ||
		    (virtual_devmap_addr > va)) {
			virtual_devmap_addr = va;
		}

		pmap_map_chunk(
		    table[i].pd_va,
		    table[i].pd_pa,
		    table[i].pd_size,
		    table[i].pd_prot,
		    table[i].pd_flags);

		pmap_devmap_bootstrap_done = true;
	}
}


const struct pmap_devmap *
pmap_devmap_find_va(vaddr_t va, vsize_t size)
{

	if (pmap_devmap_table == NULL)
		return NULL;

	const vaddr_t endva = va + size;
	for (size_t i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if ((va >= pmap_devmap_table[i].pd_va) &&
		    (endva <= pmap_devmap_table[i].pd_va +
			      pmap_devmap_table[i].pd_size)) {
			return &pmap_devmap_table[i];
		}
	}
	return NULL;
}


const struct pmap_devmap *
pmap_devmap_find_pa(paddr_t pa, psize_t size)
{

	if (pmap_devmap_table == NULL)
		return NULL;

	const paddr_t endpa = pa + size;
	for (size_t i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if (pa >= pmap_devmap_table[i].pd_pa &&
		    (endpa <= pmap_devmap_table[i].pd_pa +
			      pmap_devmap_table[i].pd_size))
			return (&pmap_devmap_table[i]);
	}
	return NULL;
}


#ifdef MULTIPROCESSOR
void
pmap_md_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
	/* nothing */
}
#endif /* MULTIPROCESSOR */
