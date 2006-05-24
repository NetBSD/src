/*	$NetBSD: tsarm_machdep.c,v 1.3.12.1 2006/05/24 15:47:54 tron Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Based on code written by Jason R. Thorpe and Steve C. Woodford for
 * Wasabi Systems, Inc.
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
 *
 * Machine dependant functions for kernel setup for Iyonix.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tsarm_machdep.c,v 1.3.12.1 2006/05/24 15:47:54 tron Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <acorn32/include/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/ep93xxvar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include "epcom.h"
#if NEPCOM > 0
#include <arm/ep93xx/epcomvar.h>
#endif

#include "isa.h"
#if NISA > 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#include <machine/isa_machdep.h>

#include <evbarm/tsarm/tsarmreg.h>

#include "opt_ipkdb.h"
#include "ksyms.h"

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xf0000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x0C000000

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = 0x00000000;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	8
#define ABT_STACK_SIZE	8
#ifdef IPKDB
#define UND_STACK_SIZE	16
#else
#define UND_STACK_SIZE	8
#endif

struct bootconfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_freeend_low;
vm_offset_t physical_end;
u_int free_pages;
int physmem = 0;

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

vm_offset_t msgbufphys;

static struct arm32_dma_range tsarm_dma_ranges[4];

#if NISA > 0
extern void isa_tsarm_init(u_int, u_int); 
#endif

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* L2 table for mapping vectors page */

#define KERNEL_PT_KERNEL	1	/* L2 table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
					/* L2 tables for mapping kernel VM */ 
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)

#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

void	consinit(void);
/*
 * Define the default console speed for the machine.
 */
#ifndef CONSPEED
#define CONSPEED B115200
#endif /* ! CONSPEED */

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

#if KGDB
#ifndef KGDB_DEVNAME
#error Must define KGDB_DEVNAME
#endif
const char kgdb_devname[] = KGDB_DEVNAME;

#ifndef KGDB_DEVADDR
#error Must define KGDB_DEVADDR
#endif
unsigned long kgdb_devaddr = KGDB_DEVADDR;

#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE	CONSPEED
#endif
int kgdb_devrate = KGDB_DEVRATE;

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE	CONMODE
#endif
int kgdb_devmode = KGDB_DEVMODE;
#endif /* KGDB */

/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */
void
cpu_reboot(int howto, char *bootstr)
{

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("\r\n");
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
		printf("\r\nrebooting...\r\n");
		goto reset;
	}

	/* Disable console buffering */

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();
	
	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("\r\n");
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
	}

	printf("\r\nrebooting...\r\n");
 reset:
	/*
	 * Make really really sure that all interrupts are disabled,
	 * and poke the Internal Bus and Peripheral Bus reset lines.
	 */
	(void) disable_interrupts(I32_bit|F32_bit);

	{
		u_int32_t feed, ctrl;

		feed = TS7XXX_IO16_VBASE + TS7XXX_WDOGFEED;
		ctrl = TS7XXX_IO16_VBASE + TS7XXX_WDOGCTRL;

		__asm volatile (
			"mov r0, #0x5\n"
			"mov r1, #0x1\n"
			"strh r0, [%0]\n"
			"strh r1, [%1]\n"
			: 
			: "r" (feed), "r" (ctrl)
			: "r0", "r1"
		);
	}

	for (;;);
}

/* Static device mappings. */
static const struct pmap_devmap tsarm_devmap[] = {
    {
	EP93XX_AHB_VBASE,
	EP93XX_AHB_HWBASE,
	EP93XX_AHB_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

    {
	EP93XX_APB_VBASE,
	EP93XX_APB_HWBASE,
	EP93XX_APB_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

	/*
	 * IO8 and IO16 space *must* be mapped contiguously with
	 * IO8_VA == IO16_VA - 64 Mbytes.  ISA busmap driver depends
	 * on that!
	 */
    {
	TS7XXX_IO8_VBASE,
	TS7XXX_IO8_HWBASE,
	TS7XXX_IO8_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

    {
	TS7XXX_IO16_VBASE,
	TS7XXX_IO16_HWBASE,
	TS7XXX_IO16_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

   {
	0,
	0,
	0,
	0,
	0,
    }
};

/*
 * u_int initarm(...)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 *   Initialising interrupt controllers to a sane default state
 */
u_int
initarm(void *arg)
{
#ifdef FIXME
	struct bootconfig *passed_bootconfig = arg;
	extern char _end[];
#endif
	int loop;
	int loop1;
	u_int l1pagetable;
	pv_addr_t kernel_l1pt;
	paddr_t memstart;
	psize_t memsize;

#ifdef FIXME
	/* Calibrate the delay loop. */
	i80321_calibrate_delay();
#endif

	/*
	 * Since we map the on-board devices VA==PA, and the kernel
	 * is running VA==PA, it's possible for us to initialize
	 * the console now.
	 */
	consinit();

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/tsarm booting ...\n");
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * We are currently running with the MMU enabled
	 */

#ifdef FIXME 
	/*
	 * Fetch the SDRAM start/size from the i80321 SDRAM configuration
	 * registers.
	 */
	i80321_sdram_bounds(&obio_bs_tag, VERDE_PMMR_BASE + VERDE_MCU_BASE,
	    &memstart, &memsize);
#else
	memstart = 0x0;
	memsize = 0x2000000;
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independant */
	bootconfig.dramblocks = 4;
	bootconfig.dram[0].address = 0x0UL;
	bootconfig.dram[0].pages = 0x800000UL / PAGE_SIZE;
	bootconfig.dram[1].address = 0x1000000UL;
	bootconfig.dram[1].pages = 0x800000UL / PAGE_SIZE;
	bootconfig.dram[2].address = 0x4000000UL;
	bootconfig.dram[2].pages = 0x800000UL / PAGE_SIZE;
	bootconfig.dram[3].address = 0x5000000UL;
	bootconfig.dram[3].pages = 0x800000UL / PAGE_SIZE;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0x00200000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the L1 table that we set up, we
	 * will panic.  We will update physical_freestart and
	 * physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = bootconfig.dram[0].address + 
		(bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = 0x00009000UL;
	physical_freeend = 0x00200000UL;

	physmem = (physical_end - physical_start) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * Okay, the kernel starts 2MB in from the bottom of physical
	 * memory.  We are going to allocate our bootstrap pages downwards
	 * from there.
	 *
	 * We need to allocate some fixed page tables to get the kernel
	 * going.  We allocate one page directory and a number of page
	 * tables and store the physical addresses in the kernel_pt_table
	 * array.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 4K bounaries.  What we do is allocate the
	 * page directory on the first 16K boundary that we encounter, and
	 * the page tables on 4K boundaries otherwise.  Since we allocate
	 * at least 3 L2 page tables, we are guaranteed to encounter at
	 * least one 16K aligned region.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	       physical_freestart, free_pages, free_pages);
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)				\
	alloc_pages((var).pv_pa, (np));			\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)				\
	physical_freeend -= ((np) * PAGE_SIZE);		\
	if (physical_freeend < physical_freestart)	\
		panic("initarm: out of memory");	\
	(var) = physical_freeend;			\
	free_pages -= (np);				\
	memset((char *)(var), 0, ((np) * PAGE_SIZE));

	loop1 = 0;
	kernel_l1pt.pv_pa = 0;
	kernel_l1pt.pv_va = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if (((physical_freeend - L1_TABLE_SIZE) & (L1_TABLE_SIZE - 1)) == 0
		    && kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
		} else {
			valloc_pages(kernel_pt_table[loop1],
			    L2_TABLE_SIZE / PAGE_SIZE);
			++loop1;
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory");

	/*
	 * Allocate a page for the system vectors page
	 */
	alloc_pages(systempage.pv_pa, 1);

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

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables.  Save physical_freeend for when we give whats left 
	 * of memory below 2Mbyte to UVM.
	 */

	physical_freeend_low = physical_freeend;

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at 0x%08lx\n", kernel_l1pt.pv_pa);
#endif

	/*
	 * Now we start construction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, ARM_VECTORS_HIGH & ~(0x00400000 - 1),
	    &kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	{
		extern char etext[], _end[];
		size_t textsize = (uintptr_t) etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;
		
		logical = 0x00200000;	/* offset of kernel in RAM */
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
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

	pmap_map_chunk(l1pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		pmap_map_chunk(l1pagetable, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
	}

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the statically mapped devices. */
	pmap_devmap_bootstrap(l1pagetable, tsarm_devmap);

	/*
	 * Update the physical_freestart/physical_freeend/free_pages
	 * variables.
	 */
	{
		extern char _end[];

		physical_freestart = physical_start +
		    (((((uintptr_t) _end) + PGOFSET) & ~PGOFSET) -
		     KERNEL_BASE);
		physical_freeend = physical_end;
		free_pages =
		    (physical_freeend - physical_freestart) / PAGE_SIZE;
	}

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
	       physical_freestart, free_pages, free_pages);
	printf("switching to new L1 page table  @%#lx...", kernel_l1pt.pv_pa);
#endif
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	setttb(kernel_l1pt.pv_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	proc0paddr = (struct user *)kernelstack.pv_va;
	lwp0.l_addr = proc0paddr;

#ifdef VERBOSE_INIT_ARM
	printf("done!\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

	arm32_vector_init(ARM_VECTORS_HIGH, ARM_VEC_ALL);

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
	uvm_page_physload(0, atop(physical_freeend_low),
	    0, atop(physical_freeend_low),
	    VM_FREELIST_DEFAULT);
	/*
	 * There is 32 Mb of memory on the TS-7200 in 4 8Mb chunks, so far 
	 * we've only been working with the first one mapped at 0x0.  Tell 
	 * UVM about the others.	
	 */
	uvm_page_physload(atop(0x1000000), atop(0x1800000),
	    atop(0x1000000), atop(0x1800000),
	    VM_FREELIST_DEFAULT);
	uvm_page_physload(atop(0x4000000), atop(0x4800000),
	    atop(0x4000000), atop(0x4800000),
	    VM_FREELIST_DEFAULT);
	uvm_page_physload(atop(0x5000000), atop(0x5800000),
	    atop(0x5000000), atop(0x5800000),
	    VM_FREELIST_DEFAULT);

	physmem = 0x2000000 / PAGE_SIZE;
	

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, KERNEL_VM_BASE,
	    KERNEL_VM_BASE + KERNEL_VM_SIZE);

	/* Setup the IRQ system */
#ifdef VERBOSE_INIT_ARM
	printf("irq ");
#endif
	ep93xx_intr_init();
#if NISA > 0
	isa_intr_init();

#ifdef VERBOSE_INIT_ARM
	printf("isa ");
#endif
	isa_tsarm_init(TS7XXX_IO16_VBASE + TS7XXX_ISAIO,
		TS7XXX_IO16_VBASE + TS7XXX_ISAMEM);	
#endif

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef BOOTHOWTO
	boothowto = BOOTHOWTO;
#endif

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#if NKSYMS || defined(DDB) || defined(LKM)
	/* Firmware doesn't load symbols. */
	ksyms_init(0, NULL, NULL);
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
consinit(void)
{
	static int consinit_called;
	bus_space_handle_t ioh;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	/*
	 * Console devices are already mapped in VA.  Our devmap reflects
	 * this, so register it now so drivers can map the console
	 * device.
	 */
	pmap_devmap_register(tsarm_devmap);
#if 0
	isa_tsarm_init(TS7XXX_IO16_VBASE + TS7XXX_ISAIO,
		TS7XXX_IO16_VBASE + TS7XXX_ISAMEM);	

        if (comcnattach(&isa_io_bs_tag, 0x3e8, comcnspeed,
            COM_FREQ, COM_TYPE_NORMAL, comcnmode))
        {
                panic("can't init serial console");
        }
#endif

#if NEPCOM > 0
	bus_space_map(&ep93xx_bs_tag, EP93XX_APB_HWBASE + EP93XX_APB_UART1, 
		EP93XX_APB_UART_SIZE, 0, &ioh);
        if (epcomcnattach(&ep93xx_bs_tag, EP93XX_APB_HWBASE + EP93XX_APB_UART1, 
		ioh, comcnspeed, comcnmode))
	{
		panic("can't init serial console");
	}
#else
	panic("serial console not configured");
#endif
#if KGDB
#if NEPCOM > 0
	if (strcmp(kgdb_devname, "epcom") == 0) {
		com_kgdb_attach(&ep93xx_bs_tag, kgdb_devaddr, kgdb_devrate,
			kgdb_devmode);
	}
#endif	/* NEPCOM > 0 */
#endif	/* KGDB */
}


bus_dma_tag_t
ep93xx_bus_dma_init(struct arm32_bus_dma_tag *dma_tag_template)
{
	int i;
	struct arm32_bus_dma_tag *dmat;

	for (i = 0; i < bootconfig.dramblocks; i++) {
		tsarm_dma_ranges[i].dr_sysbase = bootconfig.dram[i].address;
		tsarm_dma_ranges[i].dr_busbase = bootconfig.dram[i].address;
		tsarm_dma_ranges[i].dr_len = bootconfig.dram[i].pages * 
			PAGE_SIZE;
	}

	dmat = dma_tag_template;

	dmat->_ranges = tsarm_dma_ranges;
	dmat->_nranges = bootconfig.dramblocks;

	return dmat;
}
