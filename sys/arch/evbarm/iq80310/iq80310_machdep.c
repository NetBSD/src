/*	$NetBSD: iq80310_machdep.c,v 1.20.2.2 2002/02/11 20:07:45 jdolecek Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
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
 * Machine dependant functions for kernel setup for Intel IQ80310 evaluation
 * boards using RedBoot firmware.
 */

#include "opt_ddb.h"
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

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/xscale/i80312reg.h>
#include <arm/xscale/i80312var.h>

#include <dev/pci/ppbreg.h>

#include <evbarm/iq80310/iq80310reg.h>
#include <evbarm/iq80310/iq80310var.h>
#include <evbarm/iq80310/obiovar.h>

#include "opt_ipkdb.h"

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = 0;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#ifdef IPKDB
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif

BootConfig bootconfig;		/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_end;
u_int free_pages;
vm_offset_t pagetables_start;
int physmem = 0;

/*int debug_flags;*/
#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;
pv_addr_t minidataclean;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define	KERNEL_PT_IOPXS		2	/* Page table for mapping i80312 */
#define KERNEL_PT_VMDATA	3	/* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	(KERNEL_VM_SIZE >> (PDSHIFT + 2))
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pt_entry_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

void	consinit(void);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifndef CONSPEED
#define CONSPEED B115200	/* What RedBoot uses */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef CONUNIT
#define	CONUNIT	0
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
int comcnunit = CONUNIT;

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
#ifdef DIAGNOSTIC
	/* info */
	printf("boot: howto=%08x curproc=%p\n", howto, curproc);
#endif

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		cpu_reset();
		/*NOTREACHED*/
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
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	/*NOTREACHED*/
}

/*
 * Mapping table for core kernel memory. This memory is mapped at init
 * time with section mappings.
 */
struct l1_sec_map {
	vaddr_t	va;
	vaddr_t	pa;
	vsize_t	size;
	int flags;
} l1_sec_table[] = {
    /*
     * Map the on-board devices VA == PA so that we can access them
     * with the MMU on or off.
     */
    {
	IQ80310_OBIO_BASE,
	IQ80310_OBIO_BASE,
	IQ80310_OBIO_SIZE,
	0,
    },

    {
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
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg)
{
	extern vaddr_t xscale_cache_clean_addr, xscale_minidata_clean_addr;
	extern vsize_t xscale_minidata_clean_size;
	int loop;
	int loop1;
	u_int l1pagetable;
	u_int l2pagetable;
	extern char page0[], page0_end[];
	pv_addr_t kernel_l1pt;
	pv_addr_t kernel_ptpt;
	paddr_t memstart;
	psize_t memsize;

	/*
	 * Clear out the 7-segment display.  Whee, the first visual
	 * indication that we're running kernel code.
	 */
	iq80310_7seg(' ', ' ');

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* Calibrate the delay loop. */
	iq80310_calibrate_delay();

	/*
	 * Since we map the on-board devices VA==PA, and the kernel
	 * is running VA==PA, it's possible for us to initialize
	 * the console now.
	 */
	consinit();

	/* Talk to the user */
	printf("\nNetBSD/evbarm (IQ80310) booting ...\n");

	/*
	 * Reset the secondary PCI bus.  RedBoot doesn't stop devices
	 * on the PCI bus before handing us control, so we have to
	 * do this.
	 *
	 * XXX This is arguably a bug in RedBoot, and doing this reset
	 * XXX could be problematic in the future if we encounter an
	 * XXX application where the PPB in the i80312 is used as a
	 * XXX PPB.
	 */
	{
		uint32_t reg;

		printf("Resetting secondary PCI bus...\n");
		reg = bus_space_read_4(&obio_bs_tag,
		    I80312_PMMR_BASE + I80312_PPB_BASE, PPB_REG_BRIDGECONTROL);
		bus_space_write_4(&obio_bs_tag,
		    I80312_PMMR_BASE + I80312_PPB_BASE, PPB_REG_BRIDGECONTROL,
		    reg | PPB_BC_SECONDARY_RESET);
		delay(10 * 1000);	/* 10ms enough? */
		bus_space_write_4(&obio_bs_tag,
		    I80312_PMMR_BASE + I80312_PPB_BASE, PPB_REG_BRIDGECONTROL,
		    reg);
	}

	/*
	 * Okay, RedBoot has provided us with the following memory map:
	 *
	 * Physical Address Range     Description 
	 * -----------------------    ---------------------------------- 
	 * 0x00000000 - 0x00000fff    flash Memory 
	 * 0x00001000 - 0x00001fff    80312 Internal Registers 
	 * 0x00002000 - 0x007fffff    flash Memory 
	 * 0x00800000 - 0x7fffffff    PCI ATU Outbound Direct Window 
	 * 0x80000000 - 0x83ffffff    Primary PCI 32-bit Memory 
	 * 0x84000000 - 0x87ffffff    Primary PCI 64-bit Memory 
	 * 0x88000000 - 0x8bffffff    Secondary PCI 32-bit Memory 
	 * 0x8c000000 - 0x8fffffff    Secondary PCI 64-bit Memory 
	 * 0x90000000 - 0x9000ffff    Primary PCI IO Space 
	 * 0x90010000 - 0x9001ffff    Secondary PCI IO Space 
	 * 0x90020000 - 0x9fffffff    Unused 
	 * 0xa0000000 - 0xbfffffff    SDRAM 
	 * 0xc0000000 - 0xefffffff    Unused 
	 * 0xf0000000 - 0xffffffff    80200 Internal Registers
	 *
	 *
	 * Virtual Address Range    C B  Description 
	 * -----------------------  - -  ---------------------------------- 
	 * 0x00000000 - 0x00000fff  Y Y  SDRAM 
	 * 0x00001000 - 0x00001fff  N N  80312 Internal Registers 
	 * 0x00002000 - 0x007fffff  Y N  flash Memory 
	 * 0x00800000 - 0x7fffffff  N N  PCI ATU Outbound Direct Window 
	 * 0x80000000 - 0x83ffffff  N N  Primary PCI 32-bit Memory 
	 * 0x84000000 - 0x87ffffff  N N  Primary PCI 64-bit Memory 
	 * 0x88000000 - 0x8bffffff  N N  Secondary PCI 32-bit Memory 
	 * 0x8c000000 - 0x8fffffff  N N  Secondary PCI 64-bit Memory 
	 * 0x90000000 - 0x9000ffff  N N  Primary PCI IO Space 
	 * 0x90010000 - 0x9001ffff  N N  Secondary PCI IO Space 
	 * 0xa0000000 - 0xa0000fff  Y N  flash 
	 * 0xa0001000 - 0xbfffffff  Y Y  SDRAM 
	 * 0xc0000000 - 0xcfffffff  Y Y  Cache Flush Region 
	 * 0xf0000000 - 0xffffffff  N N  80200 Internal Registers 
	 *
	 * The first level page table is at 0xa0004000.  There are also
	 * 2 second-level tables at 0xa0008000 and 0xa0008400.
	 *
	 * This corresponds roughly to the physical memory map, i.e.
	 * we are quite nearly running VA==PA.
	 */

	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
#if 0
	process_kernel_args((char *)nwbootinfo.bt_args);
#endif

	/*
	 * Fetch the SDRAM start/size from the i80312 SDRAM configration
	 * registers.
	 */
	i80312_sdram_bounds(&obio_bs_tag, I80312_PMMR_BASE + I80312_MEM_BASE,
	    &memstart, &memsize);

	printf("initarm: Configuring system ...\n");

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independant */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / NBPG;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0xa0200000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the page tables that RedBoot
	 * set up, we will panic.  We will update physical_freestart
	 * and physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + (bootconfig.dram[0].pages * NBPG);

	physical_freestart = 0xa0009000UL;
	physical_freeend = 0xa0200000UL;

	physmem = (physical_end - physical_start) / NBPG;

	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);

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

	free_pages = (physical_freeend - physical_freestart) / NBPG;

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	       physical_freestart, free_pages, free_pages);
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)				\
	alloc_pages((var).pv_pa, (np));			\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)				\
	physical_freeend -= ((np) * NBPG);		\
	if (physical_freeend < physical_freestart)	\
		panic("initarm: out of memory");	\
	(var) = physical_freeend;			\
	free_pages -= (np);				\
	memset((char *)(var), 0, ((np) * NBPG));

	loop1 = 0;
	kernel_l1pt.pv_pa = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if (((physical_freeend - PD_SIZE) & (PD_SIZE - 1)) == 0
		    && kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, PD_SIZE / NBPG);
		} else {
			alloc_pages(kernel_pt_table[loop1], PT_SIZE / NBPG);
			++loop1;
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (PD_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory\n");

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	alloc_pages(systempage.pv_pa, 1);

	/* Allocate a page for the page table to map kernel page tables. */
	valloc_pages(kernel_ptpt, PT_SIZE / NBPG);

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

	/* Allocate enough pages for cleaning the Mini-Data cache. */
	KASSERT(xscale_minidata_clean_size <= NBPG);
	valloc_pages(minidataclean, 1);
	xscale_minidata_clean_addr = minidataclean.pv_va;

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

	/*
	 * XXX Defer this to later so that we can reclaim the memory
	 * XXX used by the RedBoot page tables.
	 */
	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / NBPG);

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at 0x%08lx\n", kernel_l1pt.pv_pa);
#endif

	/*
	 * Now we start consturction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	map_pagetable(l1pagetable, 0x00000000,
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_pagetable(l1pagetable, KERNEL_BASE,
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_pagetable(l1pagetable, IQ80310_IOPXS_VBASE,
	    kernel_pt_table[KERNEL_PT_IOPXS]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		map_pagetable(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	map_pagetable(l1pagetable, PROCESS_PAGE_TBLS_BASE,
	    kernel_ptpt.pv_pa);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL];

	{
		extern char etext[], _end[];
		size_t textsize = (uintptr_t) etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;
		
		logical = 0x00200000;	/* offset of kernel in RAM */

		/*
		 * This maps the kernel text/data/bss VA==PA.
		 */
		logical += map_chunk(l1pagetable, l2pagetable,
		    KERNEL_BASE + logical,
		    physical_start + logical, textsize,
		    AP_KRW, PT_CACHEABLE);
		logical += map_chunk(l1pagetable, l2pagetable,
		    KERNEL_BASE + logical,
		    physical_start + logical, totalsize - textsize,
		    AP_KRW, PT_CACHEABLE);

#if 0 /* XXX No symbols yet. */
		logical += map_chunk(l1pagetable, l2pagetable,
		    KERNEL_BASE + logical,
		    physical_start + logical, kernexec->a_syms + sizeof(int)
		    + *(u_int *)((int)end + kernexec->a_syms + sizeof(int)),
		    AP_KRW, PT_CACHEABLE);
#endif
	}

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	map_chunk(0, l2pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    PD_SIZE, AP_KRW, 0);

	/* Map the Mini-Data cache clean area. */
	map_chunk(0, l2pagetable, minidataclean.pv_va, minidataclean.pv_pa,
	    NBPG, AP_KRW, PT_CACHEABLE);

	/* Map the page table that maps the kernel pages */
	map_entry_nc(l2pagetable, kernel_ptpt.pv_pa, kernel_ptpt.pv_pa);

	/*
	 * Map entries in the page table used to map PTE's
	 * Basically every kernel page table gets mapped here
	 */
	/* The -2 is slightly bogus, it should be -log2(sizeof(pt_entry_t)) */
	l2pagetable = kernel_ptpt.pv_pa;
	map_entry_nc(l2pagetable, (KERNEL_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_entry_nc(l2pagetable, (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT-2)),
	    kernel_ptpt.pv_pa);
	map_entry_nc(l2pagetable, (0x00000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		map_entry_nc(l2pagetable, ((KERNEL_VM_BASE +
		    (loop * 0x00400000)) >> (PGSHIFT-2)),
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/*
	 * Map the system page in the kernel page table for the bottom 1Meg
	 * of the virtual memory map.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_SYS];
	map_entry(l2pagetable, 0x00000000, systempage.pv_pa);

	/*
	 * Map devices we can map w/ section mappings.
	 */
	loop = 0;
	while (l1_sec_table[loop].size) {
		vm_size_t sz;

#ifdef VERBOSE_INIT_ARM
		printf("%08lx -> %08lx @ %08lx\n", l1_sec_table[loop].pa,
		    l1_sec_table[loop].pa + l1_sec_table[loop].size - 1,
		    l1_sec_table[loop].va);
#endif
		for (sz = 0; sz < l1_sec_table[loop].size; sz += L1_SEC_SIZE)
			map_section(l1pagetable, l1_sec_table[loop].va + sz,
			    l1_sec_table[loop].pa + sz,
			    l1_sec_table[loop].flags);
		++loop;
	}

	/*
	 * Map the PCI I/O spaces and i80312 registers.  These are too
	 * small to be mapped w/ section mappings.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_IOPXS];
#ifdef VERBOSE_INIT_ARM
	printf("Mapping PIOW 0x%08lx -> 0x%08lx @ 0x%08lx\n",
	    I80312_PCI_XLATE_PIOW_BASE,
	    I80312_PCI_XLATE_PIOW_BASE + I80312_PCI_XLATE_IOSIZE - 1,
	    IQ80310_PIOW_VBASE);
#endif
	map_chunk(0, l2pagetable, IQ80310_PIOW_VBASE,
	    I80312_PCI_XLATE_PIOW_BASE, I80312_PCI_XLATE_IOSIZE, AP_KRW, 0);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping SIOW 0x%08lx -> 0x%08lx @ 0x%08lx\n",
	    I80312_PCI_XLATE_SIOW_BASE,
	    I80312_PCI_XLATE_SIOW_BASE + I80312_PCI_XLATE_IOSIZE - 1,
	    IQ80310_SIOW_VBASE);
#endif
	map_chunk(0, l2pagetable, IQ80310_SIOW_VBASE,
	    I80312_PCI_XLATE_SIOW_BASE, I80312_PCI_XLATE_IOSIZE, AP_KRW, 0);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping 80312 0x%08lx -> 0x%08lx @ 0x%08lx\n",
	    I80312_PMMR_BASE,
	    I80312_PMMR_BASE + I80312_PMMR_SIZE - 1,
	    IQ80310_80312_VBASE);
#endif
	map_chunk(0, l2pagetable, IQ80310_80312_VBASE,
	    I80312_PMMR_BASE, I80312_PMMR_SIZE, AP_KRW, 0);

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

	/*
	 * Update the physical_freestart/physical_freeend/free_pages
	 * variables.
	 */
	{
		extern char _end[];

		physical_freestart = (((uintptr_t) _end) + PGOFSET) & ~PGOFSET;
		physical_freeend = physical_end;
		free_pages = (physical_freeend - physical_freestart) / NBPG;
	}

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
	       physical_freestart, free_pages, free_pages);
	printf("switching to new L1 page table  @%#lx...", kernel_l1pt.pv_pa);
#endif
	setttb(kernel_l1pt.pv_pa);

#ifdef VERBOSE_INIT_ARM
	printf("done!\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

	/* Right, set up the vectors at the bottom of page 0 */
	memcpy((char *)0x00000000, page0, page0_end - page0);

	/* We have modified a text page so sync the icache */
	cpu_icache_sync_all();

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
	printf("init subsystems: stacks ");

	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + IRQ_STACK_SIZE * NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + ABT_STACK_SIZE * NBPG);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + UND_STACK_SIZE * NBPG);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slighly better one.
	 */
	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* At last !
	 * We now have the kernel in physical memory from the bottom upwards.
	 * Kernel page tables are physically above this.
	 * The kernel is mapped to KERNEL_TEXT_BASE
	 * The kernel data PTs will handle the mapping of 0xf1000000-0xf3ffffff
	 * The page tables are mapped to 0xefc00000
	 */

	/* Initialise the undefined instruction handlers */
	printf("undefined ");
	undefined_init();

	/* Boot strap pmap telling it where the kernel page table is */
	printf("pmap ");
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, kernel_ptpt);

	/* Setup the IRQ system */
	printf("irq ");
	iq80310_intr_init();
	printf("done.\n");

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#ifdef DDB
	db_machine_init();

	/* Firmware doesn't load symbols. */
	ddb_init(0, NULL, NULL);

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

#if 0
void
process_kernel_args(char *args)
{
	static char bootargs[MAX_BOOT_STRING + 1];

	boothowto = 0;

	/* Make a local copy of the bootargs */
	strncpy(bootargs, args, MAX_BOOT_STRING);

	args = bootargs;
	boot_file = bootargs;

	/* Skip the kernel image filename */
	while (*args != ' ' && *args != 0)
		++args;

	if (*args != 0)
		*args++ = 0;

	while (*args == ' ')
		++args;

	boot_args = args;

	printf("bootfile: %s\n", boot_file);
	printf("bootargs: %s\n", boot_args);

	parse_mi_bootargs(boot_args);
}
#endif

void
consinit(void)
{
	static const bus_addr_t comcnaddrs[] = {
		IQ80310_UART2,		/* com0 (J9) */
		IQ80310_UART1,		/* com1 (J10) */
	};
	static int consinit_called;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0
	if (comcnattach(&obio_bs_tag, comcnaddrs[comcnunit], comcnspeed,
	    COM_FREQ, comcnmode))
			panic("can't init serial console @%lx", IQ80310_UART2);
#else
	panic("serial console @%lx not configured", IQ80310_UART2);
#endif
}
