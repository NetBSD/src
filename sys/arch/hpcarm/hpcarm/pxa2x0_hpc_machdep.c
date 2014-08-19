/*	$NetBSD: pxa2x0_hpc_machdep.c,v 1.18.2.2 2014/08/20 00:03:02 tls Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *      This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Machine dependent functions for kernel setup.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_hpc_machdep.c,v 1.18.2.2 2014/08/20 00:03:02 tls Exp $");

#include "opt_ddb.h"
#include "opt_dram_pages.h"
#include "opt_modular.h"
#include "opt_pmap_debug.h"
#include "ksyms.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/exec.h>
#include <sys/ksyms.h>
#include <sys/conf.h>	/* XXX for consinit related hacks */
#include <sys/device.h>
#include <sys/bus.h>

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE	DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <uvm/uvm.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/locore.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/io.h>
#include <machine/platid.h>
#include <machine/rtc.h>
#include <machine/signal.h>

#include <dev/hpc/apm/apmvar.h>
#include <dev/ic/comreg.h>

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#ifndef	KERNEL_VM_BASE
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)
#endif

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define	KERNEL_VM_SIZE		0x0c000000

extern BootConfig bootconfig;		/* Boot config storage */

extern paddr_t physical_start;
extern paddr_t physical_freestart;
extern paddr_t physical_freeend;
extern paddr_t physical_end;

extern vaddr_t msgbufphys;

extern int end;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif /* PMAP_DEBUG */

#define	KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define	KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
#define	KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
					/* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define	NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

pv_addr_t minidataclean;

/* Prototypes */
void data_abort_handler(trapframe_t *);
void prefetch_abort_handler(trapframe_t *);
void undefinedinstruction_bounce(trapframe_t *);
u_int cpu_get_control(void);

u_int init_pxa2x0(int, char **, struct bootinfo *);

/* Machine dependent initialize function */
extern void pxa2x0_machdep_init(void);

/* Mode dependent sleep function holder */
extern void (*__sleep_func)(void *);
extern void *__sleep_ctx;

#ifdef DEBUG_BEFOREMMU
static void	fakecninit(void);
#endif

/* Number of DRAM pages which are installed */
/* Units are 4K pages, so 8192 is 32 MB of memory */
#ifndef DRAM_PAGES
#define DRAM_PAGES	8192
#endif

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel and stay at the same address
 * throughout whole kernel's life time.
 */
#define	PXA2X0_GPIO_VBASE	0xfd000000
#define	PXA2X0_CLKMAN_VBASE	0xfd100000
#define	PXA2X0_INTCTL_VBASE	0xfd200000
#define	PXA2X0_MEMCTL_VBASE	0xfd300000
#define	PXA2X0_FFUART_VBASE	0xfd400000
#define	PXA2X0_BTUART_VBASE	0xfd500000
#define	PXA2X0_STUART_VBASE	0xfd600000

#define	_A(a)	((a) & L1_S_FRAME)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & L1_S_FRAME)
const struct pmap_devmap pxa2x0_devmap[] = {
    {
	    PXA2X0_GPIO_VBASE,
	    _A(PXA2X0_GPIO_BASE),
	    _S(PXA2X0_GPIO_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    PXA2X0_CLKMAN_VBASE,
	    _A(PXA2X0_CLKMAN_BASE),
	    _S(PXA2X0_CLKMAN_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    PXA2X0_INTCTL_VBASE,
	    _A(PXA2X0_INTCTL_BASE),
	    _S(PXA2X0_INTCTL_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    PXA2X0_MEMCTL_VBASE,
	    _A(PXA2X0_MEMCTL_BASE),
	    _S(PXA2X0_MEMCTL_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    PXA2X0_FFUART_VBASE,
	    _A(PXA2X0_FFUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    PXA2X0_BTUART_VBASE,
	    _A(PXA2X0_BTUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    PXA2X0_STUART_VBASE,
	    _A(PXA2X0_STUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },

    { 0, 0, 0, 0, 0, }
};
#undef	_A
#undef	_S
extern const struct pmap_devmap machdep_devmap[];


static inline pd_entry_t *
read_ttb(void)
{
	u_long ttb;

	__asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1 << 14) - 1));
}

/*
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes:
 *   Initializing the physical console so characters can be printed.
 *   Setting up page tables for the kernel.
 */
u_int
init_pxa2x0(int argc, char **argv, struct bootinfo *bi)
{
#ifdef DIAGNOSTIC
	extern vsize_t xscale_minidata_clean_size; /* used in KASSERT */
#endif
	extern vaddr_t xscale_cache_clean_addr;
	u_int kerneldatasize, symbolsize;
	u_int l1pagetable;
	vaddr_t freemempos;
	int loop;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	Elf_Shdr *sh;
#endif

	pmap_devmap_bootstrap((vaddr_t)read_ttb(), pxa2x0_devmap);
	pxa2x0_memctl_bootstrap(PXA2X0_MEMCTL_VBASE);
	pxa2x0_intr_bootstrap(PXA2X0_INTCTL_VBASE);
	pxa2x0_clkman_bootstrap(PXA2X0_CLKMAN_VBASE);
	pxa2x0_gpio_bootstrap(PXA2X0_GPIO_VBASE);

	/*
	 * XXX for now, overwrite bootconfig to hardcoded values in
	 * XXX pxa2x0_machdep_init().
	 * XXX kill bootconfig and directly call uvm_physload
	 */
	bootconfig.dram[0].address = 0xa0000000;
	bootconfig.dram[0].pages = DRAM_PAGES;
	bootconfig.dramblocks = 1;

	pxa2x0_machdep_init();

#ifdef DEBUG_BEFOREMMU
	/*
	 * At this point, we cannot call real consinit().
	 * Just call a faked up version of consinit(), which does the thing
	 * with MMU disabled.
	 */
	fakecninit();
#endif

	kerneldatasize = (uint32_t)&end - (uint32_t)KERNEL_TEXT_BASE;
	symbolsize = 0;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (!memcmp(&end, "\177ELF", 4)) {
		sh = (Elf_Shdr *)((char *)&end + ((Elf_Ehdr *)&end)->e_shoff);
		loop = ((Elf_Ehdr *)&end)->e_shnum;
		for (; loop; loop--, sh++)
			if (sh->sh_offset > 0 &&
			    (sh->sh_offset + sh->sh_size) > symbolsize)
				symbolsize = sh->sh_offset + sh->sh_size;
	}
#endif

	printf("kernsize=0x%x\n", kerneldatasize);
	kerneldatasize += symbolsize;
	kerneldatasize = ((kerneldatasize - 1) & ~(PAGE_SIZE * 4 - 1)) +
	    PAGE_SIZE * 8;

	/*
	 * hpcboot has loaded me with MMU disabled.
	 * So create kernel page tables and enable MMU.
	 */

	/*
	 * Set up the variables that define the availability of physcial
	 * memory.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_freestart = physical_start
	    + (KERNEL_TEXT_BASE - KERNEL_BASE) + kerneldatasize;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address
	    + bootconfig.dram[bootconfig.dramblocks - 1].pages * PAGE_SIZE;
	physical_freeend = physical_end;
    
	for (loop = 0; loop < bootconfig.dramblocks; ++loop)
		physmem += bootconfig.dram[loop].pages;
    
	/* XXX handle UMA framebuffer memory */

	freemempos = 0xa0009000UL;
	memset((void *)freemempos, 0, KERNEL_TEXT_BASE - KERNEL_BASE - 0x9000);

	/*
	 * Right. We have the bottom meg of memory mapped to 0x00000000
	 * so was can get at it. The kernel will occupy the start of it.
	 * After the kernel/args we allocate some of the fixed page tables
	 * we need to get the system going.
	 * We allocate one page directory and NUM_KERNEL_PTS page tables
	 * and store the physical addresses in the kernel_pt_table array.
	 * Must remember that neither the page L1 or L2 page tables are the
	 * same size as a page !
	 *
	 * Ok, the next bit of physical allocate may look complex but it is
	 * simple really. I have done it like this so that no memory gets
	 * wasted during the allocate of various pages and tables that are
	 * all different sizes.
	 * The start address will be page aligned.
	 * We allocate the kernel page directory on the first free 16KB
	 * boundary we find.
	 * We allocate the kernel page tables on the first 1KB boundary we
	 * find.  We allocate at least 9 PT's (12 currently).  This means
	 * that in the process we KNOW that we will encounter at least one
	 * 16KB boundary.
	 *
	 * Eventually if the top end of the memory gets used for process L1
	 * page tables the kernel L1 page table may be moved up there.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)			\
	alloc_pages((var).pv_pa, (np));		\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;
#define	alloc_pages(var, np)			\
	(var) = freemempos;			\
	freemempos += (np) * PAGE_SIZE;

	{
		int loop1 = 0;
		kernel_l1pt.pv_pa = 0;
		kernel_l1pt.pv_va = 0;
		for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
			/* Are we 16KB aligned for an L1 ? */
			if (((freemempos - L1_TABLE_SIZE) & (L1_TABLE_SIZE - 1)) == 0
			    && kernel_l1pt.pv_pa == 0) {
				valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
			} else {
				valloc_pages(kernel_pt_table[loop1],
				    L2_TABLE_SIZE / PAGE_SIZE);
				++loop1;
			}
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory");

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(systempage, 1);

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

#ifdef VERBOSE_INIT_ARM
	printf("IRQ stack: p0x%08lx v0x%08lx\n", irqstack.pv_pa,
	    irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n", abtstack.pv_pa,
	    abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n", undstack.pv_pa,
	    undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n", kernelstack.pv_pa,
	    kernelstack.pv_va); 
#endif

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

	/* Allocate enough pages for cleaning the Mini-Data cache. */
	KASSERT(xscale_minidata_clean_size <= PAGE_SIZE);
	valloc_pages(minidataclean, 1);
#ifdef VERBOSE_INIT_ARM
	printf("minidataclean: p0x%08lx v0x%08lx, size = %ld\n",
	    minidataclean.pv_pa, minidataclean.pv_va,
	    xscale_minidata_clean_size);
#endif

	/*
	 * Ok, we have allocated physical pages for the primary kernel
	 * page tables.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table\n");
#endif

	/*
	 * Now we start construction of the L1 page table.
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary.
	 */
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, 0x00000000,
	    &kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; ++loop)
		pmap_link_l2pt(l1pagetable, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		pmap_link_l2pt(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel code/data */

	/*
	 * XXX there is no ELF header to find RO region.
	 * XXX What should we do?
	 */
#if 0
	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
		logical = pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    VM_PROT_READ, PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable,
		    KERNEL_TEXT_BASE + logical, physical_start + logical,
		    kerneldatasize - kernexec->a_text,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	} else
#endif
		pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    KERNEL_TEXT_BASE - KERNEL_BASE + physical_start,
		    kerneldatasize, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

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

	pmap_map_chunk(l1pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	/* Map page tables */
	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		pmap_map_chunk(l1pagetable, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
	}

	/* Map the Mini-Data cache clean area. */
	xscale_setup_minidata(l1pagetable, minidataclean.pv_va,
	    minidataclean.pv_pa);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/*
	 * map integrated peripherals at same address in l1pagetable
	 * so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1pagetable, pxa2x0_devmap);
	pmap_devmap_bootstrap(l1pagetable, machdep_devmap);

	/*
	 * Give the XScale global cache clean code an appropriately
	 * sized chunk of unmapped VA space starting at 0xff000000
	 * (our device mappings end before this address).
	 */
	xscale_cache_clean_addr = 0xff000000U;

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

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
#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("kstack V%08lx P%08lx\n", kernelstack.pv_va,
		    kernelstack.pv_pa);
#endif /* PMAP_DEBUG */

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler. Until then we will use a handler that just panics but
	 * tells us why.
	 * Initialization of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
#ifdef DEBUG
	printf("%08x %08x %08x\n", data_abort_handler_address,
	    prefetch_abort_handler_address, undefined_handler_address); 
#endif

	/* Initialize the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined\n");
#endif
	undefined_init();

	/* Set the page table address. */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table  @%#lx...\n", kernel_l1pt.pv_pa);
#endif
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(kernel_l1pt.pv_pa, true);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init.
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL);

	consinit();

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("freemempos=%08lx\n", freemempos);
	printf("MMU enabled. control=%08x\n", cpu_get_control());
#endif

	/* Load memory into UVM. */
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	for (loop = 0; loop < bootconfig.dramblocks; loop++) {
		paddr_t dblk_start = (paddr_t)bootconfig.dram[loop].address;
		paddr_t dblk_end = dblk_start
			+ (bootconfig.dram[loop].pages * PAGE_SIZE);

		if (dblk_start < physical_freestart)
			dblk_start = physical_freestart;
		if (dblk_end > physical_freeend)
			dblk_end = physical_freeend;

		uvm_page_physload(atop(dblk_start), atop(dblk_end),
		    atop(dblk_start), atop(dblk_end), VM_FREELIST_DEFAULT);
	}

	/* Boot strap pmap telling it where the kernel page table is */
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

#ifdef DDB
	db_machine_init();
#endif
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf(symbolsize, ((int *)&end), ((char *)&end) + symbolsize);
#endif

	printf("kernsize=0x%x", kerneldatasize);
	printf(" (including 0x%x symbols)\n", symbolsize);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif /* DDB */

	/* We return the new stack pointer address */
	return (kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

#ifdef DEBUG_BEFOREMMU
static void
fakecninit(void)
{
#if (NCOM > 0) && defined(COM_PXA2X0)
	comcnattach(&pxa2x0_a4x_bs_tag, comcnaddr, comcnspeed,
	    PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode);
#endif
}
#endif
