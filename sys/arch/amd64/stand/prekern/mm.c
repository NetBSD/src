/*	$NetBSD: mm.c,v 1.24.6.1 2020/02/29 20:18:16 ad Exp $	*/

/*
 * Copyright (c) 2017-2020 The NetBSD Foundation, Inc. All rights reserved.
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

#define ELFROUND	64

static const uint8_t pads[4] = {
	[BTSEG_NONE] = 0x00,
	[BTSEG_TEXT] = 0xCC,
	[BTSEG_RODATA] = 0x00,
	[BTSEG_DATA] = 0x00
};

#define MM_PROT_READ	0x00
#define MM_PROT_WRITE	0x01
#define MM_PROT_EXECUTE	0x02

static const pt_entry_t protection_codes[3] = {
	[MM_PROT_READ] = PTE_NX,
	[MM_PROT_WRITE] = PTE_W | PTE_NX,
	[MM_PROT_EXECUTE] = 0,
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
	if (PTE_BASE[pl1_i(va)] & PTE_P) {
		fatal("mm_enter_pa: mapping already present");
	}
	PTE_BASE[pl1_i(va)] = pa | PTE_P | protection_codes[prot];
}

static void
mm_reenter_pa(paddr_t pa, vaddr_t va, pte_prot_t prot)
{
	PTE_BASE[pl1_i(va)] = pa | PTE_P | protection_codes[prot];
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
		mm_reenter_pa(pa + i * PAGE_SIZE, tmpva,
		    MM_PROT_READ|MM_PROT_WRITE);
		mm_flush_va(tmpva);
		memset((void *)tmpva, 0, PAGE_SIZE);
	}

	return pa;
}

static bool
mm_pte_is_valid(pt_entry_t pte)
{
	return ((pte & PTE_P) != 0);
}

static void
mm_mprotect(vaddr_t startva, size_t size, pte_prot_t prot)
{
	size_t i, npages;
	vaddr_t va;
	paddr_t pa;

	ASSERT(size % PAGE_SIZE == 0);
	npages = size / PAGE_SIZE;

	for (i = 0; i < npages; i++) {
		va = startva + i * PAGE_SIZE;
		pa = (PTE_BASE[pl1_i(va)] & PTE_FRAME);
		mm_reenter_pa(pa, va, prot);
		mm_flush_va(va);
	}
}

void
mm_bootspace_mprotect(void)
{
	pte_prot_t prot;
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

	/* Build L4. */
	L4e_idx = pl4_i(startva);
	nL4e = mm_nentries_range(startva, endva, NBPD_L4);
	ASSERT(L4e_idx == 511);
	ASSERT(nL4e == 1);
	if (!mm_pte_is_valid(L4_BASE[L4e_idx])) {
		pa = mm_palloc(1);
		L4_BASE[L4e_idx] = pa | PTE_P | PTE_W;
	}

	/* Build L3. */
	L3e_idx = pl3_i(startva);
	nL3e = mm_nentries_range(startva, endva, NBPD_L3);
	for (i = 0; i < nL3e; i++) {
		if (mm_pte_is_valid(L3_BASE[L3e_idx+i])) {
			continue;
		}
		pa = mm_palloc(1);
		L3_BASE[L3e_idx+i] = pa | PTE_P | PTE_W;
	}

	/* Build L2. */
	L2e_idx = pl2_i(startva);
	nL2e = mm_nentries_range(startva, endva, NBPD_L2);
	for (i = 0; i < nL2e; i++) {
		if (mm_pte_is_valid(L2_BASE[L2e_idx+i])) {
			continue;
		}
		pa = mm_palloc(1);
		L2_BASE[L2e_idx+i] = pa | PTE_P | PTE_W;
	}
}

static vaddr_t
mm_randva_kregion(size_t size, size_t pagesz)
{
	vaddr_t sva, eva;
	vaddr_t randva;
	uint64_t rnd;
	size_t i;
	bool ok;

	while (1) {
		prng_get_rand(&rnd, sizeof(rnd));
		randva = rounddown(KASLR_WINDOW_BASE +
		    rnd % (KASLR_WINDOW_SIZE - size), pagesz);

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
			if (randva < sva && eva < (randva + size)) {
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

	/*
	 * If possible, shift the segment in memory using a random offset. Once
	 * shifted the segment remains in the same page, of size pagesz. Make
	 * sure to respect the ELF alignment constraint.
	 */

	if (elfalign == 0) {
		elfalign = ELFROUND;
	}

	ASSERT(pagesz >= elfalign);
	ASSERT(pagesz % elfalign == 0);
	shiftsize = roundup(elfsz, pagesz) - roundup(elfsz, elfalign);
	if (shiftsize == 0) {
		return 0;
	}

	prng_get_rand(&rnd, sizeof(rnd));
	offset = roundup(rnd % shiftsize, elfalign);
	ASSERT((va + offset) % elfalign == 0);

	memmove((void *)(va + offset), (void *)va, elfsz);

	return offset;
}

static void
mm_map_head(void)
{
	size_t i, npages, size;
	uint64_t rnd;
	vaddr_t randva;

	/*
	 * The HEAD window is 1GB below the main KASLR window. This is to
	 * ensure that head always comes first in virtual memory. The reason
	 * for that is that we use (headva + sh_offset), and sh_offset is
	 * unsigned.
	 */

	/*
	 * To get the size of the head, we give a look at the read-only
	 * mapping of the kernel we created in locore. We're identity mapped,
	 * so kernpa = kernva.
	 */
	size = elf_get_head_size((vaddr_t)kernpa_start);
	npages = size / PAGE_SIZE;

	/*
	 * Choose a random range of VAs in the HEAD window, and create the page
	 * tree for it.
	 */
	prng_get_rand(&rnd, sizeof(rnd));
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

vaddr_t
mm_map_segment(int segtype, paddr_t pa, size_t elfsz, size_t elfalign)
{
	size_t i, npages, size, pagesz, offset;
	vaddr_t randva;
	char pad;

	if (elfsz <= PAGE_SIZE) {
		pagesz = NBPD_L1;
	} else {
		pagesz = NBPD_L2;
	}

	/* Create the page tree */
	size = roundup(elfsz, pagesz);
	randva = mm_randva_kregion(size, pagesz);

	/* Enter the segment */
	npages = size / PAGE_SIZE;
	for (i = 0; i < npages; i++) {
		mm_enter_pa(pa + i * PAGE_SIZE,
		    randva + i * PAGE_SIZE, MM_PROT_READ|MM_PROT_WRITE);
	}

	/* Shift the segment in memory */
	offset = mm_shift_segment(randva, pagesz, elfsz, elfalign);
	ASSERT(offset + elfsz <= size);

	/* Fill the paddings */
	pad = pads[segtype];
	memset((void *)randva, pad, offset);
	memset((void *)(randva + offset + elfsz), pad, size - elfsz - offset);

	/* Register the bootspace information */
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
	bootspace.smodule = (vaddr_t)iom_base + IOM_SIZE;
	bootspace.emodule = bootspace.boot.va + NKL2_KIMG_ENTRIES * NBPD_L2;
}

/*
 * The bootloader has set up the following layout of physical memory:
 * +------------+-----------------+---------------+------------------+-------+
 * | ELF HEADER | SECTION HEADERS | KERN SECTIONS | SYM+REL SECTIONS | EXTRA |
 * +------------+-----------------+---------------+------------------+-------+
 * Which we abstract into several "regions":
 * +------------------------------+---------------+--------------------------+
 * |         Head region          | Several segs  |       Boot region        |
 * +------------------------------+---------------+--------------------------+
 * See loadfile_elf32.c:loadfile_dynamic() for the details.
 *
 * There is a variable number of independent regions we create: one head,
 * several kernel segments, one boot. They are all mapped at random VAs.
 *
 * Head contains the ELF Header and ELF Section Headers, and we use them to
 * map the rest of the regions. Head must be placed in both virtual memory
 * and physical memory *before* the rest.
 *
 * The Kernel Sections are mapped at random VAs using individual segments
 * in bootspace.
 *
 * Boot contains various information, including the ELF Sym+Rel sections,
 * plus extra memory the prekern has used so far; it is a region that the
 * kernel will eventually use for module_map. Boot is placed *after* the
 * other regions in physical memory. In virtual memory however there is no
 * constraint, so its VA is randomly selected in the main KASLR window.
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
