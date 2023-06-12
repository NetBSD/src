/* $NetBSD: pmap_machdep.c,v 1.18 2023/06/12 19:04:14 skrll Exp $ */

/*
 * Copyright (c) 2014, 2019, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas (of 3am Software Foundry), Maxime Villard, and
 * Nick Hudson.
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

#include "opt_riscv_debug.h"
#include "opt_multiprocessor.h"

#define	__PMAP_PRIVATE

#include <sys/cdefs.h>
__RCSID("$NetBSD: pmap_machdep.c,v 1.18 2023/06/12 19:04:14 skrll Exp $");

#include <sys/param.h>
#include <sys/buf.h>

#include <uvm/uvm.h>

#include <riscv/machdep.h>
#include <riscv/sysreg.h>

#ifdef VERBOSE_INIT_RISCV
#define	VPRINTF(...)	printf(__VA_ARGS__)
#else
#define	VPRINTF(...)	__nothing
#endif

vaddr_t pmap_direct_base __read_mostly;
vaddr_t pmap_direct_end __read_mostly;

void
pmap_zero_page(paddr_t pa)
{
#ifdef _LP64
#ifdef PMAP_DIRECT_MAP
	memset((void *)PMAP_DIRECT_MAP(pa), 0, PAGE_SIZE);
#else
#error "no direct map"
#endif
#else
	KASSERT(false);
#endif
}

void
pmap_copy_page(paddr_t src, paddr_t dst)
{
#ifdef _LP64
#ifdef PMAP_DIRECT_MAP
	memcpy((void *)PMAP_DIRECT_MAP(dst), (const void *)PMAP_DIRECT_MAP(src),
	    PAGE_SIZE);
#else
#error "no direct map"
#endif
#else
	KASSERT(false);
#endif
}

struct vm_page *
pmap_md_alloc_poolpage(int flags)
{

	return uvm_pagealloc(NULL, 0, NULL, flags);
}

vaddr_t
pmap_md_map_poolpage(paddr_t pa, vsize_t len)
{
#ifdef _LP64
	return PMAP_DIRECT_MAP(pa);
#else
	panic("not supported");
#endif
}

void
pmap_md_unmap_poolpage(vaddr_t pa, vsize_t len)
{
	/* nothing to do */
}


bool
pmap_md_direct_mapped_vaddr_p(vaddr_t va)
{
#ifdef _LP64
	return RISCV_DIRECTMAP_P(va);
#else
	return false;
#endif
}

bool
pmap_md_io_vaddr_p(vaddr_t va)
{
	return false;
}

paddr_t
pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t va)
{
#ifdef _LP64
#ifdef PMAP_DIRECT_MAP
	return PMAP_DIRECT_UNMAP(va);
#else
	KASSERT(false);
	return 0;
#endif
#else
	KASSERT(false);
	return 0;
#endif
}

vaddr_t
pmap_md_direct_map_paddr(paddr_t pa)
{
#ifdef _LP64
	return PMAP_DIRECT_MAP(pa);
#else
	panic("not supported");
#endif
}

void
pmap_md_init(void)
{
	pmap_tlb_info_evcnt_attach(&pmap_tlb0_info);
}

bool
pmap_md_ok_to_steal_p(const uvm_physseg_t bank, size_t npgs)
{
	return true;
}

#ifdef MULTIPROCESSOR
void
pmap_md_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
}
#endif


void
pmap_md_xtab_activate(struct pmap *pmap, struct lwp *l)
{
//	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	struct cpu_info * const ci = curcpu();
	struct pmap_asid_info * const pai = PMAP_PAI(pmap, cpu_tlb_info(ci));

	uint64_t satp =
#ifdef _LP64
	    __SHIFTIN(SATP_MODE_SV39, SATP_MODE) |
#else
	    __SHIFTIN(SATP_MODE_SV32, SATP_MODE) |
#endif
	    __SHIFTIN(pai->pai_asid, SATP_ASID) |
	    __SHIFTIN(pmap->pm_md.md_ppn, SATP_PPN);

	csr_satp_write(satp);
}

void
pmap_md_xtab_deactivate(struct pmap *pmap)
{

	/* switch to kernel pmap */
	pmap_md_xtab_activate(pmap_kernel(), NULL);
}

void
pmap_md_pdetab_init(struct pmap *pmap)
{
	KASSERT(pmap != NULL);

	const vaddr_t pdetabva = (vaddr_t)pmap->pm_pdetab;
	const paddr_t pdetabpa = pmap_md_direct_mapped_vaddr_to_paddr(pdetabva);
	pmap->pm_md.md_ppn = pdetabpa >> PAGE_SHIFT;

	/* XXXSB can we "pre-optimise" this by keeping a list of pdes to copy? */
	/* XXXSB for relatively normal size memory (8gb) we only need 10-20ish ptes? */
	/* XXXSB most (all?) of these ptes are  in two consecutive ranges. */
	for (size_t i = NPDEPG / 2; i < NPDEPG; ++i) {
		/*
		 * XXXSB where/when do new entries in pmap_kernel()->pm_pdetab
		 * XXXSB get added to existing pmaps?
		 *
		 * pmap_growkernal doesn't have support for fixing up exiting
		 * pmaps. (yet)
		 *
		 * Various options:
		 *
		 * - do the x86 thing. maintain a list of pmaps and update them
		 *   all in pmap_growkernel.
		 * - make sure the top level entries are populated and them simply
		 *   copy "them all" here.  If pmap_growkernel runs the new entries
		 *   will become visible to all pmaps.
		 * - ...
		 */

		/* XXXSB is this any faster than blindly copying all "high" entries? */
		pd_entry_t pde = pmap_kernel()->pm_pdetab->pde_pde[i];

		/*  we might have leaf entries (direct map) as well as non-leaf */
		if (pde) {
			pmap->pm_pdetab->pde_pde[i] = pde;
		}
	}
}

void
pmap_md_pdetab_fini(struct pmap *pmap)
{

	if (pmap == pmap_kernel())
		return;
	for (size_t i = NPDEPG / 2; i < NPDEPG; ++i) {
		KASSERT(pte_invalid_pde() == 0);
		pmap->pm_pdetab->pde_pde[i] = 0;
	}
}

static void
pmap_md_grow(pmap_pdetab_t *ptb, vaddr_t va, vsize_t vshift,
    vsize_t *remaining)
{
	KASSERT((va & (NBSEG - 1)) == 0);
#ifdef _LP64
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
			pdeva = uvm_pageboot_alloc(PAGE_SIZE);
			paddr_t pdepa = RISCV_KVA_TO_PA(pdeva);
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
    #endif
}


void
pmap_bootstrap(vaddr_t vstart, vaddr_t vend)
{
	extern pmap_pdetab_t bootstrap_pde[PAGE_SIZE / sizeof(pd_entry_t)];

//	pmap_pdetab_t * const kptb = &pmap_kern_pdetab;
	pmap_t pm = pmap_kernel();

	VPRINTF("common ");
	pmap_bootstrap_common();

#ifdef MULTIPROCESSOR
	VPRINTF("cpusets ");
	struct cpu_info * const ci = curcpu();
	kcpuset_create(&ci->ci_shootdowncpus, true);
#endif

	VPRINTF("bs_pde %p ", bootstrap_pde);

//	kend = (kend + 0x200000 - 1) & -0x200000;

	/* Use the tables we already built in init_riscv() */
	pm->pm_pdetab = bootstrap_pde;

	/* Get the PPN for our page table root */
	pm->pm_md.md_ppn = atop(KERN_VTOPHYS((vaddr_t)bootstrap_pde));

	/* Setup basic info like pagesize=PAGE_SIZE */
//	uvm_md_init();

	/* init the lock */
	// XXXNH per cpu?
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

	VPRINTF("limits: %" PRIxVADDR " - %" PRIxVADDR "\n", vstart, vend);

	const vaddr_t kvmstart = vstart;
	pmap_curmaxkvaddr = vstart + kvmsize;

	VPRINTF("kva   : %" PRIxVADDR " - %" PRIxVADDR "\n", kvmstart,
	    pmap_curmaxkvaddr);

	pmap_md_grow(pmap_kernel()->pm_pdetab, kvmstart, XSEGSHIFT, &kvmsize);

	/*
	 * Initialize the pools.
	 */

	pool_init(&pmap_pmap_pool, PMAP_SIZE, 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);

	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
#ifdef KASAN
	    NULL,
#else
	    &pmap_pv_page_allocator,
#endif
	    IPL_NONE);

	// riscv_dcache_align
	pmap_pvlist_lock_init(CACHE_LINE_SIZE);
}


vsize_t
pmap_kenter_range(vaddr_t va, paddr_t pa, vsize_t size,
    vm_prot_t prot, u_int flags)
{
	extern pd_entry_t l1_pte[PAGE_SIZE / sizeof(pd_entry_t)];

	vaddr_t sva = MEGAPAGE_TRUNC(va);
	paddr_t spa = MEGAPAGE_TRUNC(pa);
	const vaddr_t eva = MEGAPAGE_ROUND(va + size);
	const vaddr_t pdetab_mask = PMAP_PDETABSIZE - 1;
	const vsize_t vshift = SEGSHIFT;

	while (sva < eva) {
		const size_t sidx = (sva >> vshift) & pdetab_mask;

		l1_pte[sidx] = PA_TO_PTE(spa) | PTE_KERN | PTE_HARDWIRED | PTE_RW;
		spa += NBSEG;
		sva += NBSEG;
	}

	return 0;
}
