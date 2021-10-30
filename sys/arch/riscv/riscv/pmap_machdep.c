/* $NetBSD: pmap_machdep.c,v 1.10 2021/10/30 07:18:46 skrll Exp $ */

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

#define __PMAP_PRIVATE

#include <sys/cdefs.h>

__RCSID("$NetBSD: pmap_machdep.c,v 1.10 2021/10/30 07:18:46 skrll Exp $");

#include <sys/param.h>

#include <uvm/uvm.h>

#include <riscv/locore.h>

int riscv_poolpage_vmfreelist = VM_FREELIST_DEFAULT;

vaddr_t pmap_direct_base __read_mostly;
vaddr_t pmap_direct_end __read_mostly;

void
pmap_bootstrap(void)
{

	pmap_bootstrap_common();
}

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
#error "no direct map"
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

bool
pmap_md_tlb_check_entry(void *ctx, vaddr_t va, tlb_asid_t asid, pt_entry_t pte)
{
	return false;
}

void
pmap_md_xtab_activate(struct pmap *pmap, struct lwp *l)
{
	__asm("csrw\tsptbr, %0" :: "r"(pmap->pm_md.md_ptbr));
}

void
pmap_md_xtab_deactivate(struct pmap *pmap)
{
}

void
pmap_md_pdetab_init(struct pmap *pmap)
{
	pmap->pm_md.md_pdetab[NPDEPG-1] = pmap_kernel()->pm_md.md_pdetab[NPDEPG-1];
	pmap->pm_md.md_ptbr =
	    pmap_md_direct_mapped_vaddr_to_paddr((vaddr_t)pmap->pm_md.md_pdetab);
}

/* -------------------------------------------------------------------------- */

tlb_asid_t
tlb_get_asid(void)
{
	return riscvreg_asid_read();
}

void
tlb_set_asid(tlb_asid_t asid, struct pmap *pm)
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
tlb_record_asids(u_long *ptr, tlb_asid_t asid_max)
{
	memset(ptr, 0xff, PMAP_TLB_NUM_PIDS / NBBY);
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
