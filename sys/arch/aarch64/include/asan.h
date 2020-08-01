/*	$NetBSD: asan.h,v 1.9 2020/08/01 06:35:00 maxv Exp $	*/

/*
 * Copyright (c) 2018-2020 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <sys/atomic.h>
#include <sys/ksyms.h>

#include <aarch64/pmap.h>
#include <aarch64/vmparam.h>
#include <aarch64/cpufunc.h>
#include <aarch64/armreg.h>
#include <aarch64/machdep.h>

#define __MD_VIRTUAL_SHIFT	48	/* 49bit address space, cut half */
#define __MD_KERNMEM_BASE	0xFFFF000000000000 /* kern mem base address */

#define __MD_SHADOW_SIZE	(1ULL << (__MD_VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define KASAN_MD_SHADOW_START	(AARCH64_KSEG_END)
#define KASAN_MD_SHADOW_END	(KASAN_MD_SHADOW_START + __MD_SHADOW_SIZE)

static bool __md_early __read_mostly = true;

static inline int8_t *
kasan_md_addr_to_shad(const void *addr)
{
	vaddr_t va = (vaddr_t)addr;
	return (int8_t *)(KASAN_MD_SHADOW_START +
	    ((va - __MD_KERNMEM_BASE) >> KASAN_SHADOW_SCALE_SHIFT));
}

static inline bool
kasan_md_unsupported(vaddr_t addr)
{
	return (addr < VM_MIN_KERNEL_ADDRESS) ||
	    (addr >= VM_KERNEL_IO_ADDRESS);
}

static paddr_t
__md_palloc(void)
{
	paddr_t pa;

	if (__predict_false(__md_early))
		pa = (paddr_t)pmapboot_pagealloc();
	else
		pa = pmap_alloc_pdp(pmap_kernel(), NULL, 0, false);

	/* The page is zeroed. */
	return pa;
}

static inline paddr_t
__md_palloc_large(void)
{
	struct pglist pglist;
	int ret;

	if (!uvm.page_init_done)
		return 0;

	ret = uvm_pglistalloc(L2_SIZE, 0, ~0UL, L2_SIZE, 0,
	    &pglist, 1, 0);
	if (ret != 0)
		return 0;

	/* The page may not be zeroed. */
	return VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
}

static void
kasan_md_shadow_map_page(vaddr_t va)
{
	pd_entry_t *l0, *l1, *l2, *l3;
	paddr_t l0pa, pa;
	pd_entry_t pde;
	size_t idx;

	l0pa = reg_ttbr1_el1_read();
	if (__predict_false(__md_early)) {
		l0 = (void *)KERN_PHYSTOV(l0pa);
	} else {
		l0 = (void *)AARCH64_PA_TO_KVA(l0pa);
	}

	idx = l0pde_index(va);
	pde = l0[idx];
	if (!l0pde_valid(pde)) {
		pa = __md_palloc();
		atomic_swap_64(&l0[idx], pa | L0_TABLE);
	} else {
		pa = l0pde_pa(pde);
	}
	if (__predict_false(__md_early)) {
		l1 = (void *)KERN_PHYSTOV(pa);
	} else {
		l1 = (void *)AARCH64_PA_TO_KVA(pa);
	}

	idx = l1pde_index(va);
	pde = l1[idx];
	if (!l1pde_valid(pde)) {
		pa = __md_palloc();
		atomic_swap_64(&l1[idx], pa | L1_TABLE);
	} else {
		pa = l1pde_pa(pde);
	}
	if (__predict_false(__md_early)) {
		l2 = (void *)KERN_PHYSTOV(pa);
	} else {
		l2 = (void *)AARCH64_PA_TO_KVA(pa);
	}

	idx = l2pde_index(va);
	pde = l2[idx];
	if (!l2pde_valid(pde)) {
		/* If possible, use L2_BLOCK to map it in advance. */
		if ((pa = __md_palloc_large()) != 0) {
			atomic_swap_64(&l2[idx], pa | L2_BLOCK |
			    LX_BLKPAG_UXN | LX_BLKPAG_PXN | LX_BLKPAG_AF |
			    LX_BLKPAG_SH_IS | LX_BLKPAG_AP_RW);
			aarch64_tlbi_by_va(va);
			__builtin_memset((void *)va, 0, L2_SIZE);
			return;
		}
		pa = __md_palloc();
		atomic_swap_64(&l2[idx], pa | L2_TABLE);
	} else if (l2pde_is_block(pde)) {
		/* This VA is already mapped as a block. */
		return;
	} else {
		pa = l2pde_pa(pde);
	}
	if (__predict_false(__md_early)) {
		l3 = (void *)KERN_PHYSTOV(pa);
	} else {
		l3 = (void *)AARCH64_PA_TO_KVA(pa);
	}

	idx = l3pte_index(va);
	pde = l3[idx];
	if (!l3pte_valid(pde)) {
		pa = __md_palloc();
		atomic_swap_64(&l3[idx], pa | L3_PAGE | LX_BLKPAG_UXN |
		    LX_BLKPAG_PXN | LX_BLKPAG_AF | LX_BLKPAG_SH_IS |
		    LX_BLKPAG_AP_RW);
		aarch64_tlbi_by_va(va);
	}
}

static void
kasan_md_early_init(void *stack)
{
	kasan_shadow_map(stack, USPACE);
	__md_early = false;
}

static void
kasan_md_init(void)
{
	vaddr_t eva, dummy;

	CTASSERT((__MD_SHADOW_SIZE / L0_SIZE) == 64);

	/* The VAs we've created until now. */
	pmap_virtual_space(&eva, &dummy);
	kasan_shadow_map((void *)VM_MIN_KERNEL_ADDRESS,
	    eva - VM_MIN_KERNEL_ADDRESS);
}

static inline bool
__md_unwind_end(const char *name)
{
	if (!strncmp(name, "el0_trap", 8) ||
	    !strncmp(name, "el1_trap", 8)) {
		return true;
	}

	return false;
}

static void
kasan_md_unwind(void)
{
	uint64_t lr, *fp;
	const char *mod;
	const char *sym;
	size_t nsym;
	int error;

	fp = (uint64_t *)__builtin_frame_address(0);
	nsym = 0;

	while (1) {
		/*
		 * normal stack frame
		 *  fp[0]  saved fp(x29) value
		 *  fp[1]  saved lr(x30) value
		 */
		lr = fp[1];

		if (lr < VM_MIN_KERNEL_ADDRESS) {
			break;
		}
		error = ksyms_getname(&mod, &sym, (vaddr_t)lr, KSYMS_PROC);
		if (error) {
			break;
		}
		printf("#%zu %p in %s <%s>\n", nsym, (void *)lr, sym, mod);
		if (__md_unwind_end(sym)) {
			break;
		}

		fp = (uint64_t *)fp[0];
		if (fp == NULL) {
			break;
		}
		nsym++;

		if (nsym >= 15) {
			break;
		}
	}
}
