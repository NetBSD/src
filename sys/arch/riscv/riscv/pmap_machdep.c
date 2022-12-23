/* $NetBSD: pmap_machdep.c,v 1.15 2022/12/23 10:44:25 skrll Exp $ */

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

#define	__PMAP_PRIVATE

#include <sys/cdefs.h>
__RCSID("$NetBSD: pmap_machdep.c,v 1.15 2022/12/23 10:44:25 skrll Exp $");

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

int riscv_poolpage_vmfreelist = VM_FREELIST_DEFAULT;

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
	if (riscv_poolpage_vmfreelist != VM_FREELIST_DEFAULT)
		return uvm_pagealloc_strat(NULL, 0, NULL, flags,
		    UVM_PGA_STRAT_ONLY, riscv_poolpage_vmfreelist);

	return uvm_pagealloc(NULL, 0, NULL, flags);
}

vaddr_t
pmap_md_map_poolpage(paddr_t pa, vsize_t len)
{
	return PMAP_DIRECT_MAP(pa);
}

void
pmap_md_unmap_poolpage(vaddr_t pa, vsize_t len)
{
	/* nothing to do */
}

bool
pmap_md_direct_mapped_vaddr_p(vaddr_t va)
{
	return VM_MAX_KERNEL_ADDRESS <= va && (intptr_t) va < 0;
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
	return PMAP_DIRECT_MAP(pa);
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


void
pmap_md_xtab_activate(struct pmap *pmap, struct lwp *l)
{
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

	csr_satp_write(0);
}

void
pmap_md_pdetab_init(struct pmap *pmap)
{
	KASSERT(pmap != NULL);

	const vaddr_t pdetabva = (vaddr_t)pmap->pm_md.md_pdetab;
	const paddr_t pdetabpa = pmap_md_direct_mapped_vaddr_to_paddr(pdetabva);
	pmap->pm_md.md_pdetab[NPDEPG-1] = pmap_kernel()->pm_md.md_pdetab[NPDEPG-1];
	pmap->pm_md.md_ppn = pdetabpa >> PAGE_SHIFT;
}

void
pmap_md_pdetab_fini(struct pmap *pmap)
{
        KASSERT(pmap != NULL);
}

void
pmap_bootstrap(vaddr_t vstart, vaddr_t vend)
{
	extern pd_entry_t l1_pte[512];
	pmap_t pm = pmap_kernel();

	pmap_bootstrap_common();

	/* Use the tables we already built in init_mmu() */
	pm->pm_md.md_pdetab = l1_pte;

	/* Get the PPN for l1_pte */
	pm->pm_md.md_ppn = atop(KERN_VTOPHYS((vaddr_t)l1_pte));

	/* Setup basic info like pagesize=PAGE_SIZE */
	uvm_md_init();

	/* init the lock */
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

	/*
	 * Initialize the pools.
	 */
	pool_init(&pmap_pmap_pool, PMAP_SIZE, 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator, IPL_NONE);

	pmap_pvlist_lock_init(/*riscv_dcache_align*/ 64);
}

/* -------------------------------------------------------------------------- */

tlb_asid_t
tlb_get_asid(void)
{
	return csr_asid_read();
}

void
tlb_set_asid(tlb_asid_t asid, struct pmap *pm)
{
	csr_asid_write(asid);
}

#if 0
void    tlb_invalidate_all(void);
void    tlb_invalidate_globals(void);
#endif

void
tlb_invalidate_asids(tlb_asid_t lo, tlb_asid_t hi)
{
	for (; lo <= hi; lo++) {
		__asm __volatile("sfence.vma zero, %[asid]"
		    : /* output operands */
		    : [asid] "r" (lo)
		    : "memory");
	}
}
void
tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	if (asid == KERNEL_PID) {
		__asm __volatile("sfence.vma %[va]"
		    : /* output operands */
		    : [va] "r" (va)
		    : "memory");
	} else {
		__asm __volatile("sfence.vma %[va], %[asid]"
		    : /* output operands */
		    : [va] "r" (va), [asid] "r" (asid)
		    : "memory");
	}
}

bool
tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert_p)
{
	if (asid == KERNEL_PID) {
		__asm __volatile("sfence.vma %[va]"
		    : /* output operands */
		    : [va] "r" (va)
		    : "memory");
	} else {
		__asm __volatile("sfence.vma %[va], %[asid]"
		    : /* output operands */
		    : [va] "r" (va), [asid] "r" (asid)
		    : "memory");
	}
	return false;
}

u_int
tlb_record_asids(u_long *ptr, tlb_asid_t asid_max)
{
	memset(ptr, 0xff, PMAP_TLB_NUM_PIDS / NBBY);
	ptr[0] = -2UL;

	return PMAP_TLB_NUM_PIDS - 1;
}

void
tlb_walk(void *ctx, bool (*func)(void *, vaddr_t, tlb_asid_t, pt_entry_t))
{
	/* no way to view the TLB */
}

#if 0
void    tlb_enter_addr(size_t, const struct tlbmask *);
void    tlb_read_entry(size_t, struct tlbmask *);
void    tlb_write_entry(size_t, const struct tlbmask *);
void    tlb_dump(void (*)(const char *, ...));
#endif
