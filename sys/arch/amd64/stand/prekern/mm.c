/*	$NetBSD: mm.c,v 1.3 2017/10/18 17:12:42 maxv Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include "prekern.h"

static const pt_entry_t protection_codes[3] = {
	[MM_PROT_READ] = PG_RO | PG_NX,
	[MM_PROT_WRITE] = PG_RW | PG_NX,
	[MM_PROT_EXECUTE] = PG_RO,
	/* RWX does not exist */
};

extern paddr_t kernpa_start, kernpa_end;
vaddr_t iom_base;

paddr_t pa_avail = 0;
static const vaddr_t tmpva = (PREKERNBASE + NKL2_KIMG_ENTRIES * NBPD_L2);

void
mm_init(paddr_t first_pa)
{
	pa_avail = first_pa;
}

static void
mm_enter_pa(paddr_t pa, vaddr_t va, pte_prot_t prot)
{
	PTE_BASE[pl1_i(va)] = pa | PG_V | protection_codes[prot];
}

static void
mm_flush_va(vaddr_t va)
{
	asm volatile("invlpg (%0)" ::"r" (va) : "memory");
}

static paddr_t
mm_palloc(size_t npages)
{
	paddr_t pa;
	size_t i;

	/* Allocate the physical pages */
	pa = pa_avail;
	pa_avail += npages * PAGE_SIZE;

	/* Zero them out */
	for (i = 0; i < npages; i++) {
		mm_enter_pa(pa + i * PAGE_SIZE, tmpva,
		    MM_PROT_READ|MM_PROT_WRITE);
		mm_flush_va(tmpva);
		memset((void *)tmpva, 0, PAGE_SIZE);
	}

	return pa;
}

static bool
mm_pte_is_valid(pt_entry_t pte)
{
	return ((pte & PG_V) != 0);
}

paddr_t
mm_vatopa(vaddr_t va)
{
	return (PTE_BASE[pl1_i(va)] & PG_FRAME);
}

void
mm_mprotect(vaddr_t startva, size_t size, int prot)
{
	size_t i, npages;
	vaddr_t va;
	paddr_t pa;

	ASSERT(size % PAGE_SIZE == 0);
	npages = size / PAGE_SIZE;

	for (i = 0; i < npages; i++) {
		va = startva + i * PAGE_SIZE;
		pa = (PTE_BASE[pl1_i(va)] & PG_FRAME);
		mm_enter_pa(pa, va, prot);
		mm_flush_va(va);
	}
}

static void
mm_map_tree(vaddr_t startva, vaddr_t endva)
{
	size_t i, size, nL4e, nL3e, nL2e;
	size_t L4e_idx, L3e_idx, L2e_idx;
	paddr_t pa;

	size = endva - startva;

	/*
	 * Build L4.
	 */
	L4e_idx = pl4_i(startva);
	nL4e = roundup(size, NBPD_L4) / NBPD_L4;
	ASSERT(L4e_idx == 511);
	ASSERT(nL4e == 1);
	if (!mm_pte_is_valid(L4_BASE[L4e_idx])) {
		pa = mm_palloc(1);
		L4_BASE[L4e_idx] = pa | PG_V | PG_RW;
	}

	/*
	 * Build L3.
	 */
	L3e_idx = pl3_i(startva);
	nL3e = roundup(size, NBPD_L3) / NBPD_L3;
	for (i = 0; i < nL3e; i++) {
		if (mm_pte_is_valid(L3_BASE[L3e_idx+i])) {
			continue;
		}
		pa = mm_palloc(1);
		L3_BASE[L3e_idx+i] = pa | PG_V | PG_RW;
	}

	/*
	 * Build L2.
	 */
	L2e_idx = pl2_i(startva);
	nL2e = roundup(size, NBPD_L2) / NBPD_L2;
	for (i = 0; i < nL2e; i++) {
		if (mm_pte_is_valid(L2_BASE[L2e_idx+i])) {
			continue;
		}
		pa = mm_palloc(1);
		L2_BASE[L2e_idx+i] = pa | PG_V | PG_RW;
	}
}

/*
 * Select a random VA, and create a page tree. The size of this tree is
 * actually hard-coded, and matches the one created by the generic NetBSD
 * locore.
 */
static vaddr_t
mm_rand_base()
{
	vaddr_t randva;
	uint64_t rnd;
	size_t size;

	size = (NKL2_KIMG_ENTRIES + 1) * NBPD_L2;

	/* yes, this is ridiculous */
	rnd = rdtsc();
	randva = rounddown(KASLR_WINDOW_BASE + rnd % (KASLR_WINDOW_SIZE - size),
	    PAGE_SIZE);

	mm_map_tree(randva, randva + size);

	return randva;
}

/*
 * Virtual address space of the kernel:
 * +---------------+---------------------+------------------+-------------+
 * | KERNEL + SYMS | [PRELOADED MODULES] | BOOTSTRAP TABLES | ISA I/O MEM |
 * +---------------+---------------------+------------------+-------------+
 * We basically choose a random VA, and map everything contiguously starting
 * from there. Note that the physical pages allocated by mm_palloc are part
 * of the BOOTSTRAP TABLES.
 */
vaddr_t
mm_map_kernel()
{
	size_t i, npages, size;
	vaddr_t baseva;

	size = (pa_avail - kernpa_start);
	baseva = mm_rand_base();
	npages = size / PAGE_SIZE;

	/* Enter the whole area linearly */
	for (i = 0; i < npages; i++) {
		mm_enter_pa(kernpa_start + i * PAGE_SIZE,
		    baseva + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}

	/* Enter the ISA I/O MEM */
	iom_base = baseva + npages * PAGE_SIZE;
	npages = IOM_SIZE / PAGE_SIZE;
	for (i = 0; i < npages; i++) {
		mm_enter_pa(IOM_BEGIN + i * PAGE_SIZE,
		    iom_base + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}

	return baseva;
}
