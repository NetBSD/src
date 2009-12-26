/*	$NetBSD: initarm_common.c,v 1.10 2009/12/26 16:01:23 uebayasi Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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
 */
/*
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: initarm_common.c,v 1.10 2009/12/26 16:01:23 uebayasi Exp $");

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <evbarm/evbarm/initarmvar.h>


/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#define UND_STACK_SIZE	1

vm_offset_t msgbufphys;
vm_offset_t physical_start;
vm_offset_t physical_end;
pv_addr_t systempage;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;


vaddr_t
initarm_common(const struct initarm_config *ic)
{
#ifdef DIAGNOSTIC
	extern vsize_t xscale_minidata_clean_size;
#endif
	extern char etext[], _end[];
	const BootConfig *bc;
	int loop;
	vaddr_t l1pagetable;
	pv_addr_t kernel_l1pt;
	pv_addr_t *kernel_pt_table;
	pv_addr_t irqstack;
	pv_addr_t undstack;
	pv_addr_t abtstack;
	pv_addr_t kernelstack;
	pv_addr_t minidataclean;
	vm_offset_t physical_freestart;
	vm_offset_t physical_freeend;
	vaddr_t avail;
	vaddr_t pt_vstart;
	paddr_t pt_pstart;
	vsize_t pt_size;
	u_int ptcount_total;
	u_int ptcount_kernel;
	u_int ptcount_fixed_io;
	u_int ptcount_vmdata;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.
	 */
	bc = ic->ic_bootconf;
	avail = round_page((vaddr_t)(uintptr_t)&_end[0]);
	physical_start = bc->dram[0].address;
	physical_freestart = (avail - KERNEL_BASE) + ic->ic_kernel_base_pa;

	for (loop = 0; loop < bc->dramblocks; loop++) {
		paddr_t blk_end;

		blk_end = bc->dram[loop].address +
		    (bc->dram[loop].pages * PAGE_SIZE);

		if (ic->ic_kernel_base_pa >= bc->dram[loop].address &&
		    ic->ic_kernel_base_pa < blk_end)
			physical_freeend = blk_end;
			
		physmem += bc->dram[loop].pages;
	}

	loop--;
	physical_end = bc->dram[loop].address +
	    (bc->dram[loop].pages * PAGE_SIZE);

	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);

	/*
	 * Okay, the kernel starts near the bottom of physical memory
	 * and extends to (avail - KERNEL_BASE) + ic->ic_kernel_base_pa.
	 * We are going to allocate our bootstrap pages upwards
	 * from there.
	 *
	 * We need to allocate some fixed page tables to get the kernel
	 * going.  We allocate one page directory and a number of page
	 * tables and store the physical addresses in the kernel_pt_table
	 * array.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 1K boundaries.  What we do is allocate the
	 * page directory on the first 16K boundary that we encounter, and
	 * the page tables on 1K boundaries otherwise.  Since we allocate
	 * at least 12 L2 page tables, we are guaranteed to encounter at
	 * least one 16K aligned region.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, avail = 0x%08lx\n",
	       physical_freestart, avail);
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_l2(var, nl2)			\
	alloc_l2((var).pv_pa, (nl2));		\
	(var).pv_va = avail;			\
	avail += ((nl2) * L2_TABLE_SIZE_REAL);
#define alloc_l2(var, nl2)					\
	if (physical_freestart >= physical_freeend)		\
		panic("initarm: out of memory");		\
	(var) = physical_freestart;				\
	physical_freestart += ((nl2) * L2_TABLE_SIZE_REAL);	\
	memset((char *)(var), 0, ((nl2) * L2_TABLE_SIZE_REAL));

#define	valloc_pages(var, np)	\
	    valloc_l2(var, (np) * (PAGE_SIZE / L2_TABLE_SIZE_REAL))
#define	alloc_pages(var, np)	\
	    alloc_l2(var, (np) * (PAGE_SIZE / L2_TABLE_SIZE_REAL))

	/*
	 * Burn some memory at the end of the kernel to hold ~85
	 * pv_addr_t structures. This is more than sufficient to
	 * track the page tables we'll be allocating here.
	 */
	kernel_pt_table = (pv_addr_t *)avail;
	avail += L2_TABLE_SIZE_REAL;
	physical_freestart += L2_TABLE_SIZE_REAL;

	/*
	 * Figure out how much space to allocate for page tables
	 */
#define round_sec(x)	(((x) + L1_S_OFFSET) & L1_S_FRAME)

	ptcount_kernel   = round_sec(avail - KERNEL_BASE) / L1_S_SIZE;
	ptcount_vmdata   = 16;	/* 16MB of KVM, initially */
	ptcount_fixed_io = round_sec(ic->ic_iosize) / L1_S_SIZE;

	ptcount_total =
		1 +			/* The System Page */
		ptcount_kernel +	/* text/data */
		ptcount_vmdata +	/* Initial kernel VM size */
		ptcount_fixed_io;	/* Fixed I/O mappings */

	kernel_l1pt.pv_pa = 0;
	pt_pstart = physical_freestart;
	pt_vstart = avail;
	for (loop = 0; loop < ptcount_total; ) {
		/* Are we 16KB aligned for an L1 ? */
		if ((physical_freestart & (L1_TABLE_SIZE - 1)) == 0
		    && kernel_l1pt.pv_pa == 0) {
			valloc_l2(kernel_l1pt,
			    L1_TABLE_SIZE / L2_TABLE_SIZE_REAL);
		} else {
			valloc_l2(kernel_pt_table[loop], 1);
			++loop;
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory");

	/*
	 * Re-align physical_freestart to a page boundary
	 */
	physical_freestart = round_page(physical_freestart);
	avail = round_page(avail);
	pt_size = physical_freestart - pt_pstart;

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap PTs: VA: 0x%08lx, PA: 0x%08lx, Size: 0x%08lx\n",
	       pt_vstart, pt_pstart, pt_size);
#endif

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

	/* Allocate enough pages for cleaning the Mini-Data cache. */
	KASSERT(xscale_minidata_clean_size <= PAGE_SIZE);
	valloc_pages(minidataclean, 1);

	/*
	 * Allocate physical pages for the kernel message buffer
	 */
	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

	/*
	 * Allocate a page for the system page.
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	alloc_pages(systempage.pv_pa, 1);

#ifdef VERBOSE_INIT_ARM
	printf("IRQ stack: p0x%08lx v0x%08lx\n", irqstack.pv_pa,
	    irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n", abtstack.pv_pa,
	    abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n", undstack.pv_pa,
	    undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n", kernelstack.pv_pa,
	    kernelstack.pv_va); 
	printf("System Pg: p0x%08lx v0x%08lx\n", systempage.pv_pa,
	    ic->ic_vecbase);
#endif

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at 0x%08lx\n", kernel_l1pt.pv_pa);
#endif

	/*
	 * Now we start construction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_va;

	pmap_link_l2pt(l1pagetable, ic->ic_vecbase, kernel_pt_table++);

	for (loop = 0; loop < (ptcount_kernel + ptcount_vmdata); loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_BASE + loop * L1_S_SIZE,
		    kernel_pt_table++);

	for (loop = 0; loop < ptcount_fixed_io; loop++)
		pmap_link_l2pt(l1pagetable, ic->ic_iobase, kernel_pt_table++);

	/*
	 * Update the top of the kernel VM.
	 *
	 * Note that we round up 'avail' to a 1MB boundary since that's
	 * what pmap_growkernel() expects.
	 */
	avail = round_sec(avail);
	pmap_curmaxkvaddr = avail + (ptcount_vmdata * L1_S_SIZE);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	{
		size_t textsize = (uintptr_t) etext - KERNEL_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_BASE;
		u_int logical;

		textsize = round_page(textsize);
		totalsize = round_page(totalsize);

		logical = pmap_map_chunk(l1pagetable, KERNEL_BASE,
		    physical_start, textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

		(void) pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, totalsize - textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	}

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	pmap_map_chunk(l1pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the Mini-Data cache clean area. */
	xscale_setup_minidata(l1pagetable, minidataclean.pv_va,
	    minidataclean.pv_pa);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, ic->ic_vecbase, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map page tables */
	pmap_map_chunk(l1pagetable, pt_vstart, pt_pstart,
	    pt_size, VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	/*
	 * Map fixed I/O space
	 */
	for (loop = 0; loop < ic->ic_nio; loop++) {
#ifdef VERBOSE_INIT_ARM
		printf("Fixed I/O: 0x%08lx -> 0x%08lx @ 0x%08lx (0x%x, %d)\n",
		    ic->ic_io[loop].ii_pa,
		    ic->ic_io[loop].ii_pa + ic->ic_io[loop].ii_size - 1,
		    ic->ic_io[loop].ii_kva, ic->ic_io[loop].ii_prot,
		    ic->ic_io[loop].ii_cache);
#endif
#ifdef DEBUG
		if (ic->ic_io[loop].ii_kva < ic->ic_iobase ||
		    (ic->ic_io[loop].ii_kva + ic->ic_io[loop].ii_kva) >
		    ic->ic_iosize)
			panic("initarm_common: bad fixed i/o range: %d", loop);
#endif

		pmap_map_chunk(l1pagetable, ic->ic_io[loop].ii_kva,
		    ic->ic_io[loop].ii_pa, ic->ic_io[loop].ii_size,
		    ic->ic_io[loop].ii_prot, ic->ic_io[loop].ii_cache);
	}

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(kernel_l1pt.pv_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

#ifdef VERBOSE_INIT_ARM
	printf("done!\n");
#endif

	/*
	 * Fix up the vector table
	 */
	arm32_vector_init(ic->ic_vecbase, ARM_VEC_ALL);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("init subsystems: stacks ");
#endif

	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */

	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);

	for (loop = 1; loop < bc->dramblocks; loop++) {
		paddr_t blk_start;
		paddr_t blk_end;

		blk_start = bc->dram[loop].address;
		blk_end = blk_start + (bc->dram[loop].pages * PAGE_SIZE);

		/*
		 * XXX: Support different free lists
		 */
		uvm_page_physload(atop(blk_start), atop(blk_end),
		    atop(blk_start), atop(blk_end),
		    VM_FREELIST_DEFAULT);
	}

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap((pd_entry_t *)l1pagetable, avail);

#ifdef VERBOSE_INIT_ARM
	printf("Done.\n");
#endif

	return (kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}
