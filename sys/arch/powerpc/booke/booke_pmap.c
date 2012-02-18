/*	$NetBSD: booke_pmap.c,v 1.10.6.1 2012/02/18 07:32:52 mrg Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#define __PMAP_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: booke_pmap.c,v 1.10.6.1 2012/02/18 07:32:52 mrg Exp $");

#include <sys/param.h>
#include <sys/kcore.h>
#include <sys/buf.h>

#include <uvm/uvm.h>

#include <machine/pmap.h>

/*
 * Initialize the kernel pmap.
 */
#ifdef MULTIPROCESSOR
#define	PMAP_SIZE	offsetof(struct pmap, pm_pai[MAXCPUS])
#else
#define	PMAP_SIZE	sizeof(struct pmap)
#endif

CTASSERT(sizeof(struct pmap_segtab) == NBPG);

void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	struct pmap * const pmap = p->p_vmspace->vm_map.pmap;
	vsize_t off = va & PAGE_SIZE;

	kpreempt_disable();
	for (const vaddr_t eva = va + len; va < eva; off = 0) {
		const vaddr_t segeva = min(va + len, va - off + PAGE_SIZE);
		pt_entry_t * const ptep = pmap_pte_lookup(pmap, va);
		if (ptep == NULL) {
			va = segeva;
			continue;
		}
		pt_entry_t pt_entry = *ptep;
		if (!pte_valid_p(pt_entry) || !pte_exec_p(pt_entry)) {
			va = segeva;
			continue;
		}
		kpreempt_enable();
		dcache_wb(pte_to_paddr(pt_entry), segeva - va);
		icache_inv(pte_to_paddr(pt_entry), segeva - va);
		kpreempt_disable();
		va = segeva;
	}
	kpreempt_enable();
}

void
pmap_md_page_syncicache(struct vm_page *pg, __cpuset_t onproc)
{
	/*
	 * If onproc is empty, we could do a
	 * pmap_page_protect(pg, VM_PROT_NONE) and remove all
	 * mappings of the page and clear its execness.  Then
	 * the next time page is faulted, it will get icache
	 * synched.  But this is easier. :)
	 */
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	dcache_wb_page(pa);
	icache_inv_page(pa);
}

vaddr_t
pmap_md_direct_map_paddr(paddr_t pa)
{
	return (vaddr_t) pa;
}

bool
pmap_md_direct_mapped_vaddr_p(vaddr_t va)
{
	return va < VM_MIN_KERNEL_ADDRESS || VM_MAX_KERNEL_ADDRESS <= va;
}

paddr_t
pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t va)
{
	return (paddr_t) va;
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void
pmap_bootstrap(vaddr_t startkernel, vaddr_t endkernel,
	const phys_ram_seg_t *avail, size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		printf(" uvm_page_physload(%#lx,%#lx,%#lx,%#lx,%d)",
		    atop(avail[i].start),
		    atop(avail[i].start + avail[i].size) - 1,
		    atop(avail[i].start),
		    atop(avail[i].start + avail[i].size) - 1,
		    VM_FREELIST_DEFAULT);
		uvm_page_physload(
		    atop(avail[i].start),
		    atop(avail[i].start + avail[i].size) - 1,
		    atop(avail[i].start),
		    atop(avail[i].start + avail[i].size) - 1,
		    VM_FREELIST_DEFAULT);
	}

	pmap_tlb_info_init(&pmap_tlb0_info);		/* init the lock */

	/*
	 * Compute the number of pages kmem_arena will have.
	 */
	kmeminit_nkmempages();

	/*
	 * Figure out how many PTE's are necessary to map the kernel.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */

	/* Get size of buffer cache and set an upper limit */
	buf_setvalimit((VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / 8);
	vsize_t bufsz = buf_memcalc();
	buf_setvalimit(bufsz);

	vsize_t nsegtabs = pmap_round_seg(VM_PHYS_SIZE
	    + (ubc_nwins << ubc_winshift)
	    + bufsz
	    + 16 * NCARGS
	    + pager_map_size
	    + maxproc * USPACE
#ifdef SYSVSHM
	    + NBPG * shminfo.shmall
#endif
	    + NBPG * nkmempages);

	/*
	 * Initialize `FYI' variables.	Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.  Must do this before uvm_pageboot_alloc()
	 * can be called.
	 */
	pmap_limits.avail_start = vm_physmem[0].start << PGSHIFT;
	pmap_limits.avail_end = vm_physmem[vm_nphysseg - 1].end << PGSHIFT;
	const vsize_t max_nsegtabs =
	    (pmap_round_seg(VM_MAX_KERNEL_ADDRESS)
		- pmap_trunc_seg(VM_MIN_KERNEL_ADDRESS)) / NBSEG;
	if (nsegtabs >= max_nsegtabs) {
		pmap_limits.virtual_end = VM_MAX_KERNEL_ADDRESS;
		nsegtabs = max_nsegtabs;
	} else {
		pmap_limits.virtual_end = VM_MIN_KERNEL_ADDRESS
		    + nsegtabs * NBSEG;
	}

	pmap_pvlist_lock_init(curcpu()->ci_ci.dcache_line_size);

	/*
	 * Now actually allocate the kernel PTE array (must be done
	 * after virtual_end is initialized).
	 */
	vaddr_t segtabs =
	    uvm_pageboot_alloc(NBPG * nsegtabs + sizeof(struct pmap_segtab));

	/*
	 * Initialize the kernel's two-level page level.  This only wastes
	 * an extra page for the segment table and allows the user/kernel
	 * access to be common.
	 */
	struct pmap_segtab * const stp = (void *)segtabs;
	segtabs += round_page(sizeof(struct pmap_segtab));
	pt_entry_t **ptp = &stp->seg_tab[VM_MIN_KERNEL_ADDRESS >> SEGSHIFT];
	for (u_int i = 0; i < nsegtabs; i++, segtabs += NBPG) {
		*ptp++ = (void *)segtabs;
	}
	pmap_kernel()->pm_segtab = stp;
	curcpu()->ci_pmap_kern_segtab = stp;
	printf(" kern_segtab=%p", stp);

#if 0
	nsegtabs = (physmem + NPTEPG - 1) / NPTEPG;
	segtabs = uvm_pageboot_alloc(NBPG * nsegtabs);
	ptp = stp->seg_tab;
	pt_entry_t pt_entry = PTE_M|PTE_xX|PTE_xR;
	pt_entry_t *ptep = (void *)segtabs;
	printf("%s: allocated %lu page table pages for mapping %u pages\n",
	    __func__, nsegtabs, physmem);
	for (u_int i = 0; i < nsegtabs; i++, segtabs += NBPG, ptp++) {
		*ptp = ptep;
		for (u_int j = 0; j < NPTEPG; j++, ptep++) {
			*ptep = pt_entry;
			pt_entry += NBPG;
		}
		printf(" [%u]=%p (%#x)", i, *ptp, **ptp);
		pt_entry |= PTE_xW;
		pt_entry &= ~PTE_xX;
	}

	/*
	 * Now make everything before the kernel inaccessible.
	 */
	for (u_int i = 0; i < startkernel / NBPG; i += NBPG) {
		stp->seg_tab[i >> SEGSHIFT][(i & SEGOFSET) >> PAGE_SHIFT] = 0;
	}
#endif

	/*
	 * Initialize the pools.
	 */
	pool_init(&pmap_pmap_pool, PMAP_SIZE, 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator, IPL_NONE);

	tlb_set_asid(0);
}

struct vm_page *
pmap_md_alloc_poolpage(int flags)
{
	/*
	 * Any managed page works for us.
	 */
	return uvm_pagealloc(NULL, 0, NULL, flags);
}

void
pmap_zero_page(paddr_t pa)
{
	dcache_zero_page(pa);

	KASSERT(!VM_PAGEMD_EXECPAGE_P(VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(pa))));
}

void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	const size_t line_size = curcpu()->ci_ci.dcache_line_size;
	const paddr_t end = src + PAGE_SIZE;

	while (src < end) {
		__asm(
			"dcbt	%2,%1"	"\n\t"	/* touch next src cachline */
			"dcba	0,%1"	"\n\t" 	/* don't fetch dst cacheline */
		    :: "b"(src), "b"(dst), "b"(line_size));
		for (u_int i = 0;
		     i < line_size;
		     src += 32, dst += 32, i += 32) {
			__asm(
				"lmw	24,0(%0)" "\n\t"
				"stmw	24,0(%1)"
			    :: "b"(src), "b"(dst)
			    : "r24", "r25", "r26", "r27",
			      "r28", "r29", "r30", "r31");
		}
	}

	KASSERT(!VM_PAGEMD_EXECPAGE_P(VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(dst - PAGE_SIZE))));
}

void
pmap_md_init(void)
{

	/* nothing for now */
}

bool
pmap_md_io_vaddr_p(vaddr_t va)
{
	return va >= pmap_limits.avail_end
	    && !(VM_MIN_KERNEL_ADDRESS <= va && va < VM_MAX_KERNEL_ADDRESS);
}

bool
pmap_md_tlb_check_entry(void *ctx, vaddr_t va, tlb_asid_t asid, pt_entry_t pte)
{
	pmap_t pm = ctx;
        struct pmap_asid_info * const pai = PMAP_PAI(pm, curcpu()->ci_tlb_info);

	if (asid != pai->pai_asid)
		return true;

	const pt_entry_t * const ptep = pmap_pte_lookup(pm, va);
	KASSERT(ptep != NULL);
	pt_entry_t xpte = *ptep;
	xpte &= ~((xpte & (PTE_UNSYNCED|PTE_UNMODIFIED)) << 1);
	xpte ^= xpte & (PTE_UNSYNCED|PTE_UNMODIFIED|PTE_WIRED);

	KASSERTMSG(pte == xpte,
	    "pm=%p va=%#"PRIxVADDR" asid=%u: TLB pte (%#x) != real pte (%#x/%#x)",
	    pm, va, asid, pte, xpte, *ptep);

	return true;
}

#ifdef MULTIPROCESSOR
void
pmap_md_tlb_info_attach(struct pmap_tlb_info *ti, struct cpu_info *ci)
{
	/* nothing */
}
#endif /* MULTIPROCESSOR */
