/*	$NetBSD: mm.c,v 1.14 2017/11/15 18:02:36 maxv Exp $	*/

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

#define PAD_TEXT	0xCC
#define PAD_RODATA	0x00
#define PAD_DATA	0x00

#define ELFROUND	64

static const pt_entry_t protection_codes[3] = {
	[MM_PROT_READ] = PG_RO | PG_NX,
	[MM_PROT_WRITE] = PG_RW | PG_NX,
	[MM_PROT_EXECUTE] = PG_RO,
	/* RWX does not exist */
};

struct bootspace bootspace;

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

static void
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

void
mm_bootspace_mprotect(void)
{
	int prot;
	size_t i;

	/* Remap the kernel segments with proper permissions. */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_TEXT) {
			prot = MM_PROT_READ|MM_PROT_EXECUTE;
		} else if (bootspace.segs[i].type == BTSEG_RODATA) {
			prot = MM_PROT_READ;
		} else {
			continue;
		}
		mm_mprotect(bootspace.segs[i].va, bootspace.segs[i].sz, prot);
	}

	print_state(true, "Segments protection updated");
}

static size_t
mm_nentries_range(vaddr_t startva, vaddr_t endva, size_t pgsz)
{
	size_t npages;

	npages = roundup((endva / PAGE_SIZE), (pgsz / PAGE_SIZE)) -
	    rounddown((startva / PAGE_SIZE), (pgsz / PAGE_SIZE));
	return (npages / (pgsz / PAGE_SIZE));
}

static void
mm_map_tree(vaddr_t startva, vaddr_t endva)
{
	size_t i, nL4e, nL3e, nL2e;
	size_t L4e_idx, L3e_idx, L2e_idx;
	paddr_t pa;

	/*
	 * Build L4.
	 */
	L4e_idx = pl4_i(startva);
	nL4e = mm_nentries_range(startva, endva, NBPD_L4);
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
	nL3e = mm_nentries_range(startva, endva, NBPD_L3);
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
	nL2e = mm_nentries_range(startva, endva, NBPD_L2);
	for (i = 0; i < nL2e; i++) {
		if (mm_pte_is_valid(L2_BASE[L2e_idx+i])) {
			continue;
		}
		pa = mm_palloc(1);
		L2_BASE[L2e_idx+i] = pa | PG_V | PG_RW;
	}
}

static uint64_t
mm_rand_num64(void)
{
	/* XXX: yes, this is ridiculous, will be fixed soon */
	return rdtsc();
}

static void
mm_map_head(void)
{
	size_t i, npages, size;
	uint64_t rnd;
	vaddr_t randva;

	/*
	 * To get the size of the head, we give a look at the read-only
	 * mapping of the kernel we created in locore. We're identity mapped,
	 * so kernpa = kernva.
	 */
	size = elf_get_head_size((vaddr_t)kernpa_start);
	npages = size / PAGE_SIZE;

	rnd = mm_rand_num64();
	randva = rounddown(HEAD_WINDOW_BASE + rnd % (HEAD_WINDOW_SIZE - size),
	    PAGE_SIZE);
	mm_map_tree(randva, randva + size);

	/* Enter the area and build the ELF info */
	for (i = 0; i < npages; i++) {
		mm_enter_pa(kernpa_start + i * PAGE_SIZE,
		    randva + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}
	elf_build_head(randva);

	/* Register the values in bootspace */
	bootspace.head.va = randva;
	bootspace.head.pa = kernpa_start;
	bootspace.head.sz = size;
}

static vaddr_t
mm_randva_kregion(size_t size, size_t align)
{
	vaddr_t sva, eva;
	vaddr_t randva;
	uint64_t rnd;
	size_t i;
	bool ok;

	while (1) {
		rnd = mm_rand_num64();
		randva = rounddown(KASLR_WINDOW_BASE +
		    rnd % (KASLR_WINDOW_SIZE - size), align);

		/* Detect collisions */
		ok = true;
		for (i = 0; i < BTSPACE_NSEGS; i++) {
			if (bootspace.segs[i].type == BTSEG_NONE) {
				continue;
			}
			sva = bootspace.segs[i].va;
			eva = sva + bootspace.segs[i].sz;

			if ((sva <= randva) && (randva < eva)) {
				ok = false;
				break;
			}
			if ((sva < randva + size) && (randva + size <= eva)) {
				ok = false;
				break;
			}
		}
		if (ok) {
			break;
		}
	}

	mm_map_tree(randva, randva + size);

	return randva;
}

static paddr_t
bootspace_getend(void)
{
	paddr_t pa, max = 0;
	size_t i;

	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_NONE) {
			continue;
		}
		pa = bootspace.segs[i].pa + bootspace.segs[i].sz;
		if (pa > max)
			max = pa;
	}

	return max;
}

static void
bootspace_addseg(int type, vaddr_t va, paddr_t pa, size_t sz)
{
	size_t i;

	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_NONE) {
			bootspace.segs[i].type = type;
			bootspace.segs[i].va = va;
			bootspace.segs[i].pa = pa;
			bootspace.segs[i].sz = sz;
			return;
		}
	}

	fatal("bootspace_addseg: segments full");
}

static size_t
mm_shift_segment(vaddr_t va, size_t pagesz, size_t elfsz, size_t elfalign)
{
	size_t shiftsize, offset;
	uint64_t rnd;

	if (elfalign == 0) {
		elfalign = ELFROUND;
	}

	shiftsize = roundup(elfsz, pagesz) - roundup(elfsz, elfalign);
	if (shiftsize == 0) {
		return 0;
	}

	rnd = mm_rand_num64();
	offset = roundup(rnd % shiftsize, elfalign);
	ASSERT((va + offset) % elfalign == 0);

	memmove((void *)(va + offset), (void *)va, elfsz);

	return offset;
}

vaddr_t
mm_map_segment(int segtype, paddr_t pa, size_t elfsz, size_t elfalign)
{
	size_t i, npages, size, pagesz, offset;
	vaddr_t randva;
	char pad;

	if (elfsz < PAGE_SIZE) {
		pagesz = NBPD_L1;
	} else {
		pagesz = NBPD_L2;
	}

	size = roundup(elfsz, pagesz);
	randva = mm_randva_kregion(size, pagesz);

	npages = size / PAGE_SIZE;
	for (i = 0; i < npages; i++) {
		mm_enter_pa(pa + i * PAGE_SIZE,
		    randva + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}

	offset = mm_shift_segment(randva, pagesz, elfsz, elfalign);
	ASSERT(offset + elfsz <= size);

	if (segtype == BTSEG_TEXT) {
		pad = PAD_TEXT;
	} else if (segtype == BTSEG_RODATA) {
		pad = PAD_RODATA;
	} else {
		pad = PAD_DATA;
	}
	memset((void *)randva, pad, offset);
	memset((void *)(randva + offset + elfsz), pad, size - elfsz - offset);

	bootspace_addseg(segtype, randva, pa, size);

	return (randva + offset);
}

static void
mm_map_boot(void)
{
	size_t i, npages, size;
	vaddr_t randva;
	paddr_t bootpa;

	/*
	 * The "boot" region is special: its page tree has a fixed size, but
	 * the number of pages entered is lower.
	 */

	/* Create the page tree */
	size = (NKL2_KIMG_ENTRIES + 1) * NBPD_L2;
	randva = mm_randva_kregion(size, PAGE_SIZE);

	/* Enter the area and build the ELF info */
	bootpa = bootspace_getend();
	size = (pa_avail - bootpa);
	npages = size / PAGE_SIZE;
	for (i = 0; i < npages; i++) {
		mm_enter_pa(bootpa + i * PAGE_SIZE,
		    randva + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}
	elf_build_boot(randva, bootpa);

	/* Enter the ISA I/O MEM */
	iom_base = randva + npages * PAGE_SIZE;
	npages = IOM_SIZE / PAGE_SIZE;
	for (i = 0; i < npages; i++) {
		mm_enter_pa(IOM_BEGIN + i * PAGE_SIZE,
		    iom_base + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}

	/* Register the values in bootspace */
	bootspace.boot.va = randva;
	bootspace.boot.pa = bootpa;
	bootspace.boot.sz = (size_t)(iom_base + IOM_SIZE) -
	    (size_t)bootspace.boot.va;

	/* Initialize the values that are located in the "boot" region */
	extern uint64_t PDPpaddr;
	bootspace.spareva = bootspace.boot.va + NKL2_KIMG_ENTRIES * NBPD_L2;
	bootspace.pdir = bootspace.boot.va + (PDPpaddr - bootspace.boot.pa);
	bootspace.emodule = bootspace.boot.va + NKL2_KIMG_ENTRIES * NBPD_L2;
}

/*
 * There are five independent regions: head, text, rodata, data, boot. They are
 * all mapped at random VAs.
 *
 * Head contains the ELF Header and ELF Section Headers, and we use them to
 * map the rest of the regions. Head must be placed in memory *before* the
 * other regions.
 *
 * At the end of this function, the bootspace structure is fully constructed.
 */
void
mm_map_kernel(void)
{
	memset(&bootspace, 0, sizeof(bootspace));
	mm_map_head();
	print_state(true, "Head region mapped");
	elf_map_sections();
	print_state(true, "Segments mapped");
	mm_map_boot();
	print_state(true, "Boot region mapped");
}
