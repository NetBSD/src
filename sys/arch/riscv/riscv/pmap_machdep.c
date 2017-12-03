/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__RCSID("$NetBSD: pmap_machdep.c,v 1.2.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>

#include <uvm/uvm.h>

#include <riscv/locore.h>

int riscv_poolpage_vmfreelist = VM_FREELIST_DEFAULT;

void
pmap_zero_page(paddr_t pa)
{
#ifdef POOL_PHYSTOV
	memset((void *)POOL_PHYSTOV(pa), 0, PAGE_SIZE);
#else
#error FIX pmap_zero_page!
#endif
}

void
pmap_copy_page(paddr_t src, paddr_t dst)
{
#ifdef POOL_PHYSTOV
	memcpy((void *)POOL_PHYSTOV(dst), (const void *)POOL_PHYSTOV(src),
	    PAGE_SIZE);
#else
#error FIX pmap_copy_page!
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
	return POOL_PHYSTOV(pa);
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
	KASSERT(VM_MAX_KERNEL_ADDRESS <= va && (intptr_t) va < 0);
	const pmap_pdetab_t *ptb = pmap_kernel()->pm_pdetab;
	pd_entry_t pde;

#ifdef _LP64
	pde = ptb->pde_pde[(va >> XSEGSHIFT) & (NPDEPG-1)];
	if ((pde & PTE_V) == 0) {
		return -(paddr_t)1;
	}
	if ((pde & PTE_T) == 0) {
		return pde & ~XSEGOFSET;
	}
	ptb = (const pmap_pdetab_t *)POOL_PHYSTOV(pte_pde_to_paddr(pde));
#endif
	pde = ptb->pde_pde[(va >> SEGSHIFT) & (NPDEPG-1)];
	if ((pde & PTE_V) == 0) {
		return -(paddr_t)1;
	}
	if ((pde & PTE_T) == 0) {
		return pde & ~SEGOFSET;
	}
	return -(paddr_t)1;
}

vaddr_t
pmap_md_direct_map_paddr(paddr_t pa)
{
	return POOL_PHYSTOV(pa);
}

void
pmap_md_init(void)
{
        pmap_tlb_info_evcnt_attach(&pmap_tlb0_info);
}

bool
pmap_md_tlb_check_entry(void *ctx, vaddr_t va, tlb_asid_t asid, pt_entry_t pte)
{
	return false;
}
 
void
pmap_md_pdetab_activate(struct pmap *pmap)
{
	__asm("csrw\tsptbr, %0" :: "r"(pmap->pm_md.md_ptbr));
}

void
pmap_md_pdetab_init(struct pmap *pmap)
{
	pmap->pm_pdetab[NPDEPG-1] = pmap_kernel()->pm_pdetab[NPDEPG-1];
	pmap->pm_md.md_ptbr =
	    pmap_md_direct_mapped_vaddr_to_paddr((vaddr_t)pmap->pm_pdetab);
}

// TLB mainenance routines

tlb_asid_t
tlb_get_asid(void)
{
	return riscvreg_asid_read();
}

void
tlb_set_asid(tlb_asid_t asid)
{
	riscvreg_asid_write(asid);
}

#if 0
void    tlb_invalidate_all(void);
void    tlb_invalidate_globals(void);
#endif
void
tlb_invalidate_asids(tlb_asid_t lo, tlb_asid_t hi)
{
	__asm __volatile("sfence.vm" ::: "memory");
}
void
tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	__asm __volatile("sfence.vm" ::: "memory");
}

bool
tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert_p)
{
	__asm __volatile("sfence.vm" ::: "memory");
	return false;
}

u_int
tlb_record_asids(u_long *ptr)
{
	memset(ptr, 0xff, PMAP_TLB_NUM_PIDS / (8 * sizeof(u_long)));
	ptr[0] = -2UL;
	return PMAP_TLB_NUM_PIDS - 1;
}

#if 0
void    tlb_enter_addr(size_t, const struct tlbmask *);
void    tlb_read_entry(size_t, struct tlbmask *);
void    tlb_write_entry(size_t, const struct tlbmask *);
void    tlb_walk(void *, bool (*)(void *, vaddr_t, tlb_asid_t, pt_entry_t));
void    tlb_dump(void (*)(const char *, ...));
#endif
