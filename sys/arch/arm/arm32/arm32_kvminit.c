/*	$NetBSD: arm32_kvminit.c,v 1.14.2.4 2013/01/23 00:05:39 yamt Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 2007 Microsoft
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm32_kvminit.c,v 1.14.2.4 2013/01/23 00:05:39 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/bus.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/undefined.h>
#include <arm/bootconfig.h>
#include <arm/arm32/machdep.h>

#include "ksyms.h"

struct bootmem_info bootmem_info;

paddr_t msgbufphys;
paddr_t physical_start;
paddr_t physical_end;

extern char etext[];
extern char __data_start[], _edata[];
extern char __bss_start[], __bss_end__[];
extern char _end[];

/* Page tables for mapping kernel VM */
#define KERNEL_L2PT_VMDATA_NUM	8	/* start with 32MB of KVM */

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERN_VTOPHYS(bmi, va) \
	((paddr_t)((vaddr_t)(va) - KERNEL_BASE + (bmi)->bmi_start))
#define KERN_PHYSTOV(bmi, pa) \
	((vaddr_t)((paddr_t)(pa) - (bmi)->bmi_start + KERNEL_BASE))

void
arm32_bootmem_init(paddr_t memstart, psize_t memsize, vsize_t kernelstart)
{
	struct bootmem_info * const bmi = &bootmem_info;
	pv_addr_t *pv = bmi->bmi_freeblocks;

#ifdef VERBOSE_INIT_ARM
	printf("%s: memstart=%#lx, memsize=%#lx, kernelstart=%#lx\n",
	    __func__, memstart, memsize, kernelstart);
#endif

	physical_start = bmi->bmi_start = memstart;
	physical_end = bmi->bmi_end = memstart + memsize;
	physmem = memsize / PAGE_SIZE;

	/*
	 * Let's record where the kernel lives.
	 */
	bmi->bmi_kernelstart = kernelstart;
	bmi->bmi_kernelend = KERN_VTOPHYS(bmi, round_page((vaddr_t)_end));

#ifdef VERBOSE_INIT_ARM
	printf("%s: kernelend=%#lx\n", __func__, bmi->bmi_kernelend);
#endif

	/*
	 * Now the rest of the free memory must be after the kernel.
	 */
	pv->pv_pa = bmi->bmi_kernelend;
	pv->pv_va = KERN_PHYSTOV(bmi, pv->pv_pa);
	pv->pv_size = bmi->bmi_end - bmi->bmi_kernelend;
	bmi->bmi_freepages += pv->pv_size / PAGE_SIZE;
#ifdef VERBOSE_INIT_ARM
	printf("%s: adding %lu free pages: [%#lx..%#lx] (VA %#lx)\n",
	    __func__, pv->pv_size / PAGE_SIZE, pv->pv_pa,
	    pv->pv_pa + pv->pv_size - 1, pv->pv_va);
#endif
	pv++;

	/*
	 * Add a free block for any memory before the kernel.
	 */
	if (bmi->bmi_start < bmi->bmi_kernelstart) {
		pv->pv_pa = bmi->bmi_start;
		pv->pv_va = KERNEL_BASE;
		pv->pv_size = bmi->bmi_kernelstart - bmi->bmi_start;
		bmi->bmi_freepages += pv->pv_size / PAGE_SIZE;
#ifdef VERBOSE_INIT_ARM
		printf("%s: adding %lu free pages: [%#lx..%#lx] (VA %#lx)\n",
		    __func__, pv->pv_size / PAGE_SIZE, pv->pv_pa,
		    pv->pv_pa + pv->pv_size - 1, pv->pv_va);
#endif
		pv++;
	}

	bmi->bmi_nfreeblocks = pv - bmi->bmi_freeblocks;
	
	SLIST_INIT(&bmi->bmi_freechunks);
	SLIST_INIT(&bmi->bmi_chunks);
}

static bool
concat_pvaddr(pv_addr_t *acc_pv, pv_addr_t *pv)
{
	if (acc_pv->pv_pa + acc_pv->pv_size == pv->pv_pa
	    && acc_pv->pv_va + acc_pv->pv_size == pv->pv_va
	    && acc_pv->pv_prot == pv->pv_prot
	    && acc_pv->pv_cache == pv->pv_cache) {
#ifdef VERBOSE_INIT_ARMX
		printf("%s: appending pv %p (%#lx..%#lx) to %#lx..%#lx\n",
		    __func__, pv, pv->pv_pa, pv->pv_pa + pv->pv_size + 1,
		    acc_pv->pv_pa, acc_pv->pv_pa + acc_pv->pv_size + 1);
#endif
		acc_pv->pv_size += pv->pv_size;
		return true;
	}

	return false;
}

static void
add_pages(struct bootmem_info *bmi, pv_addr_t *pv)
{
	pv_addr_t **pvp = &SLIST_FIRST(&bmi->bmi_chunks);
	while ((*pvp) != NULL && (*pvp)->pv_va <= pv->pv_va) {
		pv_addr_t * const pv0 = (*pvp);
		KASSERT(SLIST_NEXT(pv0, pv_list) == NULL || pv0->pv_pa < SLIST_NEXT(pv0, pv_list)->pv_pa);
		if (concat_pvaddr(pv0, pv)) {
#ifdef VERBOSE_INIT_ARM
			printf("%s: %s pv %p (%#lx..%#lx) to %#lx..%#lx\n",
			    __func__, "appending", pv,
			    pv->pv_pa, pv->pv_pa + pv->pv_size - 1,
			    pv0->pv_pa, pv0->pv_pa + pv0->pv_size - pv->pv_size - 1);
#endif
			pv = SLIST_NEXT(pv0, pv_list);
			if (pv != NULL && concat_pvaddr(pv0, pv)) {
#ifdef VERBOSE_INIT_ARM
				printf("%s: %s pv %p (%#lx..%#lx) to %#lx..%#lx\n",
				    __func__, "merging", pv,
				    pv->pv_pa, pv->pv_pa + pv->pv_size - 1,
				    pv0->pv_pa,
				    pv0->pv_pa + pv0->pv_size - pv->pv_size - 1);
#endif
				SLIST_REMOVE_AFTER(pv0, pv_list);
				SLIST_INSERT_HEAD(&bmi->bmi_freechunks, pv, pv_list);
			}
			return;
		}
		KASSERT(pv->pv_va != (*pvp)->pv_va);
		pvp = &SLIST_NEXT(*pvp, pv_list);
	}
	KASSERT((*pvp) == NULL || pv->pv_va < (*pvp)->pv_va);
	pv_addr_t * const new_pv = SLIST_FIRST(&bmi->bmi_freechunks);
	KASSERT(new_pv != NULL);
	SLIST_REMOVE_HEAD(&bmi->bmi_freechunks, pv_list);
	*new_pv = *pv;
	SLIST_NEXT(new_pv, pv_list) = *pvp;
	(*pvp) = new_pv;
#ifdef VERBOSE_INIT_ARM
	printf("%s: adding pv %p (pa %#lx, va %#lx, %lu pages) ",
	    __func__, new_pv, new_pv->pv_pa, new_pv->pv_va,
	    new_pv->pv_size / PAGE_SIZE);
	if (SLIST_NEXT(new_pv, pv_list))
		printf("before pa %#lx\n", SLIST_NEXT(new_pv, pv_list)->pv_pa);
	else
		printf("at tail\n");
#endif
}

static void
valloc_pages(struct bootmem_info *bmi, pv_addr_t *pv, size_t npages,
	int prot, int cache)
{
	size_t nbytes = npages * PAGE_SIZE;
	pv_addr_t *free_pv = bmi->bmi_freeblocks;
	size_t free_idx = 0;
	static bool l1pt_found;

	/*
	 * If we haven't allocated the kernel L1 page table and we are aligned
	 * at a L1 table boundary, alloc the memory for it.
	 */
	if (!l1pt_found
	    && (free_pv->pv_pa & (L1_TABLE_SIZE - 1)) == 0
	    && free_pv->pv_size >= L1_TABLE_SIZE) {
		l1pt_found = true;
		valloc_pages(bmi, &kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
		add_pages(bmi, &kernel_l1pt);
	}

	while (nbytes > free_pv->pv_size) {
		free_pv++;
		free_idx++;
		if (free_idx == bmi->bmi_nfreeblocks) {
			panic("%s: could not allocate %zu bytes",
			    __func__, nbytes);
		}
	}

	/*
	 * As we allocate the memory, make sure that we don't walk over
	 * our current first level translation table.
	 */
	KASSERT((armreg_ttbr_read() & ~(L1_TABLE_SIZE - 1)) != free_pv->pv_pa);

	pv->pv_pa = free_pv->pv_pa;
	pv->pv_va = free_pv->pv_va;
	pv->pv_size = nbytes;
	pv->pv_prot = prot;
	pv->pv_cache = cache;

	/*
	 * If PTE_PAGETABLE uses the same cache modes as PTE_CACHE
	 * just use PTE_CACHE.
	 */
	if (cache == PTE_PAGETABLE
	    && pte_l1_s_cache_mode == pte_l1_s_cache_mode_pt
	    && pte_l2_l_cache_mode == pte_l2_l_cache_mode_pt
	    && pte_l2_s_cache_mode == pte_l2_s_cache_mode_pt)
		pv->pv_cache = PTE_CACHE;

	free_pv->pv_pa += nbytes;
	free_pv->pv_va += nbytes;
	free_pv->pv_size -= nbytes;
	if (free_pv->pv_size == 0) {
		--bmi->bmi_nfreeblocks;
		for (; free_idx < bmi->bmi_nfreeblocks; free_idx++) {
			free_pv[0] = free_pv[1];
		}
	}

	bmi->bmi_freepages -= npages;

	memset((void *)pv->pv_va, 0, nbytes);
}

void
arm32_kernel_vm_init(vaddr_t kernel_vm_base, vaddr_t vectors, vaddr_t iovbase,
	const struct pmap_devmap *devmap, bool mapallmem_p)
{
	struct bootmem_info * const bmi = &bootmem_info;
#ifdef MULTIPROCESSOR
	const size_t cpu_num = arm_cpu_max + 1;
#else
	const size_t cpu_num = 1;
#endif

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	KASSERT(mapallmem_p);
#endif

	/*
	 * Calculate the number of L2 pages needed for mapping the
	 * kernel + data + stuff.  Assume 2 L2 pages for kernel, 1 for vectors,
	 * and 1 for IO
	 */
	size_t kernel_size = bmi->bmi_kernelend;
	kernel_size -= (bmi->bmi_kernelstart & -L2_S_SEGSIZE);
	kernel_size += L1_TABLE_SIZE;
	kernel_size += L2_TABLE_SIZE * (2 + 1 + KERNEL_L2PT_VMDATA_NUM + 1);
	kernel_size +=
	    cpu_num * (ABT_STACK_SIZE + FIQ_STACK_SIZE + IRQ_STACK_SIZE
	    + UND_STACK_SIZE + UPAGES) * PAGE_SIZE;
	kernel_size += round_page(MSGBUFSIZE);
	kernel_size += 0x10000;	/* slop */
	kernel_size += PAGE_SIZE * (kernel_size + L2_S_SEGSIZE - 1) / L2_S_SEGSIZE;
	kernel_size = round_page(kernel_size);

	/*
	 * Now we know how many L2 pages it will take.
	 */
	const size_t KERNEL_L2PT_KERNEL_NUM =
	    (kernel_size + L2_S_SEGSIZE - 1) / L2_S_SEGSIZE;

#ifdef VERBOSE_INIT_ARM
	printf("%s: %zu L2 pages are needed to map %#zx kernel bytes\n",
	    __func__, KERNEL_L2PT_KERNEL_NUM, kernel_size);
#endif

	KASSERT(KERNEL_L2PT_KERNEL_NUM + KERNEL_L2PT_VMDATA_NUM < __arraycount(bmi->bmi_l2pts));
	pv_addr_t * const kernel_l2pt = bmi->bmi_l2pts;
	pv_addr_t * const vmdata_l2pt = kernel_l2pt + KERNEL_L2PT_KERNEL_NUM;
	pv_addr_t msgbuf;
	pv_addr_t text;
	pv_addr_t data;
	pv_addr_t chunks[KERNEL_L2PT_KERNEL_NUM+KERNEL_L2PT_VMDATA_NUM+11];
#if ARM_MMU_XSCALE == 1
	pv_addr_t minidataclean;
#endif

	/*
	 * We need to allocate some fixed page tables to get the kernel going.
	 *
	 * We are going to allocate our bootstrap pages from the beginning of
	 * the free space that we just calculated.  We allocate one page
	 * directory and a number of page tables and store the physical
	 * addresses in the bmi_l2pts array in bootmem_info.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 4K boundaries.  What we do is allocate the
	 * page directory on the first 16K boundary that we encounter, and
	 * the page tables on 4K boundaries otherwise.  Since we allocate
	 * at least 3 L2 page tables, we are guaranteed to encounter at
	 * least one 16K aligned region.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("%s: allocating page tables for", __func__);
#endif
	for (size_t i = 0; i < __arraycount(chunks); i++) {
		SLIST_INSERT_HEAD(&bmi->bmi_freechunks, &chunks[i], pv_list);
	}

	kernel_l1pt.pv_pa = 0;
	kernel_l1pt.pv_va = 0;

	/*
	 * Allocate the L2 pages, but if we get to a page that is aligned for
	 * an L1 page table, we will allocate the pages for it first and then
	 * allocate the L2 page.
	 */

	/*
	 * First allocate L2 page for the vectors.
	 */
#ifdef VERBOSE_INIT_ARM
	printf(" vector");
#endif
	valloc_pages(bmi, &bmi->bmi_vector_l2pt, L2_TABLE_SIZE / PAGE_SIZE,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
	add_pages(bmi, &bmi->bmi_vector_l2pt);

	/*
	 * Now allocate L2 pages for the kernel
	 */
#ifdef VERBOSE_INIT_ARM
	printf(" kernel");
#endif
	for (size_t idx = 0; idx < KERNEL_L2PT_KERNEL_NUM; ++idx) {
		valloc_pages(bmi, &kernel_l2pt[idx], L2_TABLE_SIZE / PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
		add_pages(bmi, &kernel_l2pt[idx]);
	}

	/*
	 * Now allocate L2 pages for the initial kernel VA space.
	 */
#ifdef VERBOSE_INIT_ARM
	printf(" vm");
#endif
	for (size_t idx = 0; idx < KERNEL_L2PT_VMDATA_NUM; ++idx) {
		valloc_pages(bmi, &vmdata_l2pt[idx], L2_TABLE_SIZE / PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
		add_pages(bmi, &vmdata_l2pt[idx]);
	}

	/*
	 * If someone wanted a L2 page for I/O, allocate it now.
	 */
	if (iovbase != 0) {
#ifdef VERBOSE_INIT_ARM
		printf(" io");
#endif
		valloc_pages(bmi, &bmi->bmi_io_l2pt, L2_TABLE_SIZE / PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
		add_pages(bmi, &bmi->bmi_io_l2pt);
	}

#ifdef VERBOSE_ARM_INIT
	printf("%s: allocating stacks\n", __func__);
#endif

	/* Allocate stacks for all modes and CPUs */
	valloc_pages(bmi, &abtstack, ABT_STACK_SIZE * cpu_num,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &abtstack);
	valloc_pages(bmi, &fiqstack, FIQ_STACK_SIZE * cpu_num,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &fiqstack);
	valloc_pages(bmi, &irqstack, IRQ_STACK_SIZE * cpu_num,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &irqstack);
	valloc_pages(bmi, &undstack, UND_STACK_SIZE * cpu_num,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &undstack);
	valloc_pages(bmi, &idlestack, UPAGES * cpu_num,		/* SVC32 */
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &idlestack);
	valloc_pages(bmi, &kernelstack, UPAGES,			/* SVC32 */
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &kernelstack);

	/* Allocate the message buffer from the end of memory. */
	const size_t msgbuf_pgs = round_page(MSGBUFSIZE) / PAGE_SIZE;
	valloc_pages(bmi, &msgbuf, msgbuf_pgs,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	add_pages(bmi, &msgbuf);
	msgbufphys = msgbuf.pv_pa;

	/*
	 * Allocate a page for the system vector page.
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(bmi, &systempage, 1, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	systempage.pv_va = vectors;

	/*
	 * If the caller needed a few extra pages for some reason, allocate
	 * them now.
	 */
#if ARM_MMU_XSCALE == 1
#if (ARM_NMMUS > 1)
	if (xscale_use_minidata)
#endif          
		valloc_pages(bmi, extrapv, nextrapages,
		    VM_PROT_READ|VM_PROT_WRITE, 0);
#endif

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables and stacks.  Let's just confirm that.
	 */
	if (kernel_l1pt.pv_va == 0
	    && (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE - 1)) != 0))
		panic("%s: Failed to allocate or align the kernel "
		    "page directory", __func__);


#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at 0x%08lx\n", kernel_l1pt.pv_pa);
#endif

	/*
	 * Now we start construction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	vaddr_t l1pt_va = kernel_l1pt.pv_va;
	paddr_t l1pt_pa = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pt_va, systempage.pv_va & -L2_S_SEGSIZE,
	    &bmi->bmi_vector_l2pt);
#ifdef VERBOSE_INIT_ARM
	printf("%s: adding L2 pt (VA %#lx, PA %#lx) for VA %#lx\n (vectors)",
	    __func__, bmi->bmi_vector_l2pt.pv_va, bmi->bmi_vector_l2pt.pv_pa,
	    systempage.pv_va);
#endif

	const vaddr_t kernel_base =
	    KERN_PHYSTOV(bmi, bmi->bmi_kernelstart & -L2_S_SEGSIZE);
	for (size_t idx = 0; idx < KERNEL_L2PT_KERNEL_NUM; idx++) {
		pmap_link_l2pt(l1pt_va, kernel_base + idx * L2_S_SEGSIZE,
		    &kernel_l2pt[idx]);
#ifdef VERBOSE_INIT_ARM
		printf("%s: adding L2 pt (VA %#lx, PA %#lx) for VA %#lx (kernel)\n",
		    __func__, kernel_l2pt[idx].pv_va, kernel_l2pt[idx].pv_pa,
		    kernel_base + idx * L2_S_SEGSIZE);
#endif
	}

	for (size_t idx = 0; idx < KERNEL_L2PT_VMDATA_NUM; idx++) {
		pmap_link_l2pt(l1pt_va, kernel_vm_base + idx * L2_S_SEGSIZE,
		    &vmdata_l2pt[idx]);
#ifdef VERBOSE_INIT_ARM
		printf("%s: adding L2 pt (VA %#lx, PA %#lx) for VA %#lx (vm)\n",
		    __func__, vmdata_l2pt[idx].pv_va, vmdata_l2pt[idx].pv_pa,
		    kernel_vm_base + idx * L2_S_SEGSIZE);
#endif
	}
	if (iovbase) {
		pmap_link_l2pt(l1pt_va, iovbase & -L2_S_SEGSIZE, &bmi->bmi_io_l2pt);
#ifdef VERBOSE_INIT_ARM
		printf("%s: adding L2 pt (VA %#lx, PA %#lx) for VA %#lx (io)\n",
		    __func__, bmi->bmi_io_l2pt.pv_va, bmi->bmi_io_l2pt.pv_pa,
		    iovbase & -L2_S_SEGSIZE);
#endif
	}

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    kernel_vm_base + (KERNEL_L2PT_VMDATA_NUM * L2_S_SEGSIZE);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	extern char etext[], _end[];
	size_t totalsize = bmi->bmi_kernelend - bmi->bmi_kernelstart;
	size_t textsize = KERN_VTOPHYS(bmi, (uintptr_t)etext) - bmi->bmi_kernelstart;

	textsize = (textsize + PGOFSET) & ~PGOFSET;

	/* start at offset of kernel in RAM */

	text.pv_pa = bmi->bmi_kernelstart;
	text.pv_va = KERN_PHYSTOV(bmi, bmi->bmi_kernelstart);
	text.pv_size = textsize;
	text.pv_prot = VM_PROT_READ|VM_PROT_WRITE; /* XXX VM_PROT_EXECUTE */
	text.pv_cache = PTE_CACHE;

#ifdef VERBOSE_INIT_ARM
	printf("%s: adding chunk for kernel text %#lx..%#lx (VA %#lx)\n",
	    __func__, text.pv_pa, text.pv_pa + text.pv_size - 1, text.pv_va);
#endif

	add_pages(bmi, &text);

	data.pv_pa = text.pv_pa + textsize;
	data.pv_va = text.pv_va + textsize;
	data.pv_size = totalsize - textsize;
	data.pv_prot = VM_PROT_READ|VM_PROT_WRITE;
	data.pv_cache = PTE_CACHE;

#ifdef VERBOSE_INIT_ARM
	printf("%s: adding chunk for kernel data/bss %#lx..%#lx (VA %#lx)\n",
	    __func__, data.pv_pa, data.pv_pa + data.pv_size - 1, data.pv_va);
#endif

	add_pages(bmi, &data);

#ifdef VERBOSE_INIT_ARM
	printf("Listing Chunks\n");
	{
		pv_addr_t *pv;
		SLIST_FOREACH(pv, &bmi->bmi_chunks, pv_list) {
			printf("%s: pv %p: chunk VA %#lx..%#lx "
			    "(PA %#lx, prot %d, cache %d)\n",
			    __func__, pv, pv->pv_va, pv->pv_va + pv->pv_size - 1,
			    pv->pv_pa, pv->pv_prot, pv->pv_cache);
		}
	}
	printf("\nMapping Chunks\n");
#endif

	pv_addr_t cur_pv;
	pv_addr_t *pv = SLIST_FIRST(&bmi->bmi_chunks);
	if (!mapallmem_p || pv->pv_pa == bmi->bmi_start) {
		cur_pv = *pv;
		pv = SLIST_NEXT(pv, pv_list);
	} else {
		cur_pv.pv_va = KERNEL_BASE;
		cur_pv.pv_pa = bmi->bmi_start;
		cur_pv.pv_size = pv->pv_pa - bmi->bmi_start;
		cur_pv.pv_prot = VM_PROT_READ | VM_PROT_WRITE;
		cur_pv.pv_cache = PTE_CACHE;
	}
	while (pv != NULL) {
		if (mapallmem_p) {
			if (concat_pvaddr(&cur_pv, pv)) {
				pv = SLIST_NEXT(pv, pv_list);
				continue;
			}
			if (cur_pv.pv_pa + cur_pv.pv_size < pv->pv_pa) {
				/*
				 * See if we can extend the current pv to emcompass the
				 * hole, and if so do it and retry the concatenation.
				 */
				if (cur_pv.pv_prot == (VM_PROT_READ|VM_PROT_WRITE)
				    && cur_pv.pv_cache == PTE_CACHE) {
					cur_pv.pv_size = pv->pv_pa - cur_pv.pv_va;
					continue;
				}

				/*
				 * We couldn't so emit the current chunk and then
				 */
#ifdef VERBOSE_INIT_ARM
				printf("%s: mapping chunk VA %#lx..%#lx "
				    "(PA %#lx, prot %d, cache %d)\n",
				    __func__,
				    cur_pv.pv_va, cur_pv.pv_va + cur_pv.pv_size - 1,
				    cur_pv.pv_pa, cur_pv.pv_prot, cur_pv.pv_cache);
#endif
				pmap_map_chunk(l1pt_va, cur_pv.pv_va, cur_pv.pv_pa,
				    cur_pv.pv_size, cur_pv.pv_prot, cur_pv.pv_cache);

				/*
				 * set the current chunk to the hole and try again.
				 */
				cur_pv.pv_pa += cur_pv.pv_size;
				cur_pv.pv_va += cur_pv.pv_size;
				cur_pv.pv_size = pv->pv_pa - cur_pv.pv_va;
				cur_pv.pv_prot = VM_PROT_READ | VM_PROT_WRITE;
				cur_pv.pv_cache = PTE_CACHE;
				continue;
			}
		}

		/*
		 * The new pv didn't concatenate so emit the current one
		 * and use the new pv as the current pv.
		 */
#ifdef VERBOSE_INIT_ARM
		printf("%s: mapping chunk VA %#lx..%#lx "
		    "(PA %#lx, prot %d, cache %d)\n",
		    __func__, cur_pv.pv_va, cur_pv.pv_va + cur_pv.pv_size - 1,
		    cur_pv.pv_pa, cur_pv.pv_prot, cur_pv.pv_cache);
#endif
		pmap_map_chunk(l1pt_va, cur_pv.pv_va, cur_pv.pv_pa,
		    cur_pv.pv_size, cur_pv.pv_prot, cur_pv.pv_cache);
		cur_pv = *pv;
		pv = SLIST_NEXT(pv, pv_list);
	}

	/*
	 * If we are mapping all of memory, let's map the rest of memory.
	 */
	if (mapallmem_p && cur_pv.pv_pa + cur_pv.pv_size < bmi->bmi_end) {
		if (cur_pv.pv_prot == (VM_PROT_READ | VM_PROT_WRITE)
		    && cur_pv.pv_cache == PTE_CACHE) {
			cur_pv.pv_size = bmi->bmi_end - cur_pv.pv_pa;
		} else {
#ifdef VERBOSE_INIT_ARM
			printf("%s: mapping chunk VA %#lx..%#lx "
			    "(PA %#lx, prot %d, cache %d)\n",
			    __func__, cur_pv.pv_va, cur_pv.pv_va + cur_pv.pv_size - 1,
			    cur_pv.pv_pa, cur_pv.pv_prot, cur_pv.pv_cache);
#endif
			pmap_map_chunk(l1pt_va, cur_pv.pv_va, cur_pv.pv_pa,
			    cur_pv.pv_size, cur_pv.pv_prot, cur_pv.pv_cache);
			cur_pv.pv_pa += cur_pv.pv_size;
			cur_pv.pv_va += cur_pv.pv_size;
			cur_pv.pv_size = bmi->bmi_end - cur_pv.pv_pa;
			cur_pv.pv_prot = VM_PROT_READ | VM_PROT_WRITE;
			cur_pv.pv_cache = PTE_CACHE;
		}
	}

	/*
	 * Now we map the final chunk.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("%s: mapping last chunk VA %#lx..%#lx (PA %#lx, prot %d, cache %d)\n",
	    __func__, cur_pv.pv_va, cur_pv.pv_va + cur_pv.pv_size - 1,
	    cur_pv.pv_pa, cur_pv.pv_prot, cur_pv.pv_cache);
#endif
	pmap_map_chunk(l1pt_va, cur_pv.pv_va, cur_pv.pv_pa,
	    cur_pv.pv_size, cur_pv.pv_prot, cur_pv.pv_cache);

	/*
	 * Now we map the stuff that isn't directly after the kernel
	 */

	/* Map the vector page. */
	pmap_map_entry(l1pt_va, systempage.pv_va, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the Mini-Data cache clean area. */ 
#if ARM_MMU_XSCALE == 1
#if (ARM_NMMUS > 1)
	if (xscale_use_minidata)
#endif          
		xscale_setup_minidata(l1_va, minidataclean.pv_va,
		    minidataclean.pv_pa);      
#endif

	/*
	 * Map integrated peripherals at same address in first level page
	 * table so that we can continue to use console.
	 */
	if (devmap)
		pmap_devmap_bootstrap(l1pt_va, devmap);

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about where all the bits and pieces live. */
	printf("%22s       Physical              Virtual        Num\n", " ");
	printf("%22s Starting    Ending    Starting    Ending   Pages\n", " ");

	static const char mem_fmt[] =
	    "%20s: 0x%08lx 0x%08lx 0x%08lx 0x%08lx %u\n";
	static const char mem_fmt_nov[] =
	    "%20s: 0x%08lx 0x%08lx                       %zu\n";

	printf(mem_fmt, "SDRAM", bmi->bmi_start, bmi->bmi_end - 1,
	    KERN_PHYSTOV(bmi, bmi->bmi_start), KERN_PHYSTOV(bmi, bmi->bmi_end - 1),
	    physmem);
	printf(mem_fmt, "text section",
	       text.pv_pa, text.pv_pa + text.pv_size - 1,
	       text.pv_va, text.pv_va + text.pv_size - 1,
	       (int)(text.pv_size / PAGE_SIZE));
	printf(mem_fmt, "data section",
	       KERN_VTOPHYS(bmi, __data_start), KERN_VTOPHYS(bmi, _edata),
	       (vaddr_t)__data_start, (vaddr_t)_edata,
	       (int)((round_page((vaddr_t)_edata)
		      - trunc_page((vaddr_t)__data_start)) / PAGE_SIZE));
	printf(mem_fmt, "bss section",
	       KERN_VTOPHYS(bmi, __bss_start), KERN_VTOPHYS(bmi, __bss_end__),
	       (vaddr_t)__bss_start, (vaddr_t)__bss_end__,
	       (int)((round_page((vaddr_t)__bss_end__)
		      - trunc_page((vaddr_t)__bss_start)) / PAGE_SIZE));
	printf(mem_fmt, "L1 page directory",
	    kernel_l1pt.pv_pa, kernel_l1pt.pv_pa + L1_TABLE_SIZE - 1,
	    kernel_l1pt.pv_va, kernel_l1pt.pv_va + L1_TABLE_SIZE - 1,
	    L1_TABLE_SIZE / PAGE_SIZE);
	printf(mem_fmt, "ABT stack (CPU 0)",
	    abtstack.pv_pa, abtstack.pv_pa + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    abtstack.pv_va, abtstack.pv_va + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    ABT_STACK_SIZE);
	printf(mem_fmt, "FIQ stack (CPU 0)",
	    fiqstack.pv_pa, fiqstack.pv_pa + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    fiqstack.pv_va, fiqstack.pv_va + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    FIQ_STACK_SIZE);
	printf(mem_fmt, "IRQ stack (CPU 0)",
	    irqstack.pv_pa, irqstack.pv_pa + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    irqstack.pv_va, irqstack.pv_va + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    IRQ_STACK_SIZE);
	printf(mem_fmt, "UND stack (CPU 0)",
	    undstack.pv_pa, undstack.pv_pa + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    undstack.pv_va, undstack.pv_va + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    UND_STACK_SIZE);
	printf(mem_fmt, "IDLE stack (CPU 0)",
	    idlestack.pv_pa, idlestack.pv_pa + (UPAGES * PAGE_SIZE) - 1,
	    idlestack.pv_va, idlestack.pv_va + (UPAGES * PAGE_SIZE) - 1,
	    UPAGES);
	printf(mem_fmt, "SVC stack",
	    kernelstack.pv_pa, kernelstack.pv_pa + (UPAGES * PAGE_SIZE) - 1,
	    kernelstack.pv_va, kernelstack.pv_va + (UPAGES * PAGE_SIZE) - 1,
	    UPAGES);
	printf(mem_fmt, "Message Buffer",
	    msgbuf.pv_pa, msgbuf.pv_pa + (msgbuf_pgs * PAGE_SIZE) - 1,
	    msgbuf.pv_va, msgbuf.pv_va + (msgbuf_pgs * PAGE_SIZE) - 1,
	    (int)msgbuf_pgs);
	printf(mem_fmt, "Exception Vectors",
	    systempage.pv_pa, systempage.pv_pa + PAGE_SIZE - 1,
	    systempage.pv_va, systempage.pv_va + PAGE_SIZE - 1,
	    1);
	for (size_t i = 0; i < bmi->bmi_nfreeblocks; i++) {
		pv = &bmi->bmi_freeblocks[i];

		printf(mem_fmt_nov, "Free Memory",
		    pv->pv_pa, pv->pv_pa + pv->pv_size - 1,
		    pv->pv_size / PAGE_SIZE);
	}
#endif
	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

#if defined(VERBOSE_INIT_ARM) && 0
	printf("TTBR0=%#x", armreg_ttbr_read());
#ifdef _ARM_ARCH_6
	printf(" TTBR1=%#x TTBCR=%#x",
	    armreg_ttbr1_read(), armreg_ttbcr_read());
#endif
	printf("\n");
#endif

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table @%#lx...", l1pt_pa);
#endif

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_idcache_wbinv_all();
	cpu_setttb(l1pt_pa, true);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

#ifdef VERBOSE_INIT_ARM
	printf("TTBR0=%#x OK\n", armreg_ttbr_read());
#endif
}
