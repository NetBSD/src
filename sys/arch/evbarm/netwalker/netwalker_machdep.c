/*	$NetBSD: netwalker_machdep.c,v 1.1.2.2 2010/11/15 14:38:23 uebayasi Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2005, 2010  Genetec Corporation. 
 * All rights reserved.
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
 * Machine dependant functions for kernel setup for Sharp Netwalker.
 * Based on iq80310_machhdep.c
 */
/*
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netwalker_machdep.c,v 1.1.2.2 2010/11/15 14:38:23 uebayasi Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_pmap_debug.h"
#include "opt_md.h"
#include "opt_com.h"
#include "md.h"
#include "imxuart.h"
#include "opt_imxuart.h"
#include "opt_imx.h"

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

#include <sys/conf.h>
#include <dev/cons.h>
#include <dev/md.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/pte.h>
#include <arm/arm32/machdep.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxwdogreg.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>
#include <arm/imx/imx51_iomuxreg.h>
#include <evbarm/netwalker/netwalker_reg.h>

/* Kernel text starts 1MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00100000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x0C000000


/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = 0;

/* Define various stack sizes in pages */
#define FIQ_STACK_SIZE	1
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

/*int debug_flags;*/
#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

/* Physical and virtual addresses for some global pages */
pv_addr_t fiqstack;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;
extern char KERNEL_BASE_phys[];
extern char KERNEL_BASE_virt[];
extern char etext[], __data_start[], _edata[], __bss_start[], __bss_end__[];
extern char _end[];
extern int cpu_do_powersave;

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL+KERNEL_PT_KERNEL_NUM)
				        /* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS ((paddr_t)&KERNEL_BASE_phys)
#define KERNEL_BASE_VIRT ((vaddr_t)&KERNEL_BASE_virt)
#define KERN_VTOPHYS(va) \
	((paddr_t)((vaddr_t)va - KERNEL_BASE_VIRT + KERNEL_BASE_PHYS))
#define KERN_PHYSTOV(pa) \
	((vaddr_t)((paddr_t)pa - KERNEL_BASE_PHYS + KERNEL_BASE_VIRT))


/* Prototypes */

void consinit(void);
#if 0
void	process_kernel_args(char *);
#endif

#ifdef KGDB
void	kgdb_port_init(void);
#endif
void	change_clock(uint32_t v);

static void init_clocks(void);
static void setup_ioports(void);
#ifdef DEBUG_IOPORTS
void dump_registers(void);
#endif

bs_protos(bs_notimpl);

#ifndef CONSPEED
#define CONSPEED B115200	/* What RedBoot uses */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

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
		pmf_system_shutdown(boothowto);
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		cpu_reset();
		/*NOTREACHED*/
	}

	/* Disable console buffering */
/*	cnpollc(1);*/

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

	pmf_system_shutdown(boothowto);

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
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in netwalker_start() so that we
 * can use them while booting the kernel, and stay at the same address
 * throughout whole kernel's life time.
 *
 * We use this table twice; once with bootstrap page table, and once
 * with kernel's page table which we build up in initarm().
 */

#define _A(a)   ((a) & ~L1_S_OFFSET)
#define _S(s)   (((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap netwalker_devmap[] = {
	{
		/* for UART1, IOMUXC */
		NETWALKER_IO_VBASE0,
		_A(NETWALKER_IO_PBASE0),
		L1_S_SIZE * 4,
		VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE
	},
	{0, 0, 0, 0, 0 }
};

#ifndef MEMSTART
#define MEMSTART	0x90000000
#endif
#ifndef MEMSIZE
#define MEMSIZE		512
#endif

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
	int loop;
	int loop1;
	vaddr_t l1pagetable;

#ifdef	RBFLAGS
	boothowto |= RBFLAGS;
#endif

	disable_interrupts(I32_bit|F32_bit);
	/* XXX move to netwalker_start.S */

	/* Register devmap for devices we mapped in start */
	pmap_devmap_register(netwalker_devmap);

	setup_ioports();

	consinit();

#ifdef	DEBUG_IOPORTS
	dump_registers();
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

#ifdef	NO_POWERSAVE
	cpu_do_powersave=0;
#endif

	init_clocks();

#ifdef KGDB
	kgdb_port_init();
#endif

	/* Talk to the user */
	printf("\nNetBSD/evbarm (netwalker) booting ...\n");

	/*
	 * Ok we have the following memory map
	 *
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 *
	 * 0x90000000 - 0x97FFFFFF    DDR SDRAM (128MByte)
	 *
	 * The initarm() has the responsibility for creating the kernel
	 * page tables.
	 * It must also set up various memory pointers that are used
	 * by pmap etc.
	 */

#if 0
	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
	process_kernel_args((char *)nwbootinfo.bt_args);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif
	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = MEMSTART;
	bootconfig.dram[0].pages = (MEMSIZE * 1024 * 1024)/ PAGE_SIZE;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0x80100000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the bottom of SDRAM, we
	 * will panic.  We will update physical_freestart and
	 * physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = 0x90000000UL;	/* top of loadaddres */
	physical_freeend =   0x90100000UL;	/* base of kernel */

	physmem = (physical_end - physical_start) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * Okay, the kernel starts 1MB in from the bottom of physical
	 * memory.  We are going to allocate our bootstrap pages downwards
	 * from there.
	 *
	 * We need to allocate some fixed page tables to get the kernel
	 * going.  We allocate one page directory and a number of page
	 * tables and store the physical addresses in the kernel_pt_table
	 * array.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 4K boundaries.  What we do is allocate the
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
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(systempage, 1);
	systempage.pv_va = ARM_VECTORS_HIGH;

	/* Allocate stacks for all modes */
	valloc_pages(fiqstack, FIQ_STACK_SIZE);
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

#ifdef VERBOSE_INIT_ARM
	printf("FIQ stack: p0x%08lx v0x%08lx\n", fiqstack.pv_pa,
	    fiqstack.pv_va);
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
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table at p0x%08lx v0x%08lx\n",
		kernel_l1pt.pv_pa, kernel_l1pt.pv_va);
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
#define round_L_page(x) (((x) + L2_L_OFFSET) & L2_L_FRAME)
	{
		size_t textsize = round_L_page((size_t)etext - KERNEL_TEXT_BASE);
		size_t totalsize = round_L_page((size_t)_end - KERNEL_TEXT_BASE);
		u_int logical;


#ifdef VERBOSE_INIT_ARM
		printf("%s: etext %lx, _end %lx\n",
		       __func__, (uintptr_t)etext, (uintptr_t)_end);
		printf("%s: textsize %#lx, totalsize %#lx\n",
		       __func__, textsize, totalsize);
#endif
		logical = 0x00100000;	/* offset of kernel in RAM */

		/* Map text section read-only. */
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
					  physical_start + logical, textsize,
					  VM_PROT_READ|VM_PROT_EXECUTE, PTE_CACHE);

		/* Map data and bss sections read-write. */
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
					  physical_start + logical, totalsize - textsize,
					  VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	}

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	pmap_map_chunk(l1pagetable, fiqstack.pv_va, fiqstack.pv_pa,
	    FIQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		pmap_map_chunk(l1pagetable, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
	}

	/* Map the vector page. */
#if 0
	/* MULTI-ICE requires that page 0 is NC/NB so that it can download the
	 * cache-clean code there.  */
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
		       VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);
#else
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
		       VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
#endif

	/*
	 * map integrated peripherals at same address in l1pagetable
	 * so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1pagetable, netwalker_devmap);

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/*
	 * Update the physical_freestart/physical_freeend/free_pages
	 * variables.
	 */
	physical_freestart = physical_start +
		(((((uintptr_t) _end) + PGOFSET) & ~PGOFSET) - KERNEL_BASE);
	physical_freeend = physical_end;
	free_pages =
		(physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about where all the bits and pieces live. */
	printf("%22s       Physical              Virtual        Num\n", " ");
	printf("%22s Starting    Ending    Starting    Ending   Pages\n", " ");

	static const char mem_fmt[] =
	    "%20s: 0x%08lx 0x%08lx 0x%08lx 0x%08lx %d\n";
	static const char mem_fmt_nov[] =
	    "%20s: 0x%08lx 0x%08lx                       %d\n";

	printf(mem_fmt, "SDRAM", physical_start, physical_end-1,
	    KERN_PHYSTOV(physical_start), KERN_PHYSTOV(physical_end-1),
	    physmem);
	printf(mem_fmt, "text section",
	       (paddr_t)KERNEL_BASE_phys, KERN_VTOPHYS(etext-1),
	       (vaddr_t)KERNEL_BASE_virt, (vaddr_t)etext-1,
	       (int)(round_L_page((size_t)etext - KERNEL_TEXT_BASE) / PAGE_SIZE));
	printf(mem_fmt, "data section",
	       KERN_VTOPHYS(__data_start), KERN_VTOPHYS(_edata),
	       (vaddr_t)__data_start, (vaddr_t)_edata,
	       (int)((round_page((vaddr_t)_edata)
		      - trunc_page((vaddr_t)__data_start)) / PAGE_SIZE));
	printf(mem_fmt, "bss section",
	       KERN_VTOPHYS(__bss_start), KERN_VTOPHYS(__bss_end__),
	       (vaddr_t)__bss_start, (vaddr_t)__bss_end__,
	       (int)((round_page((vaddr_t)__bss_end__)
		      - trunc_page((vaddr_t)__bss_start)) / PAGE_SIZE));
	printf(mem_fmt, "L1 page directory",
	    kernel_l1pt.pv_pa, kernel_l1pt.pv_pa + L1_TABLE_SIZE - 1,
	    kernel_l1pt.pv_va, kernel_l1pt.pv_va + L1_TABLE_SIZE - 1,
	    L1_TABLE_SIZE / PAGE_SIZE);
	printf(mem_fmt, "Exception Vectors",
	    systempage.pv_pa, systempage.pv_pa + PAGE_SIZE - 1,
	    systempage.pv_va, systempage.pv_va + PAGE_SIZE - 1,
	    1);
	printf(mem_fmt, "FIQ stack",
	    fiqstack.pv_pa, fiqstack.pv_pa + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    fiqstack.pv_va, fiqstack.pv_va + (FIQ_STACK_SIZE * PAGE_SIZE) - 1,
	    FIQ_STACK_SIZE);
	printf(mem_fmt, "IRQ stack",
	    irqstack.pv_pa, irqstack.pv_pa + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    irqstack.pv_va, irqstack.pv_va + (IRQ_STACK_SIZE * PAGE_SIZE) - 1,
	    IRQ_STACK_SIZE);
	printf(mem_fmt, "ABT stack",
	    abtstack.pv_pa, abtstack.pv_pa + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    abtstack.pv_va, abtstack.pv_va + (ABT_STACK_SIZE * PAGE_SIZE) - 1,
	    ABT_STACK_SIZE);
	printf(mem_fmt, "UND stack",
	    undstack.pv_pa, undstack.pv_pa + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    undstack.pv_va, undstack.pv_va + (UND_STACK_SIZE * PAGE_SIZE) - 1,
	    UND_STACK_SIZE);
	printf(mem_fmt, "SVC stack",
	    kernelstack.pv_pa, kernelstack.pv_pa + (UPAGES * PAGE_SIZE) - 1,
	    kernelstack.pv_va, kernelstack.pv_va + (UPAGES * PAGE_SIZE) - 1,
	    UPAGES);
	printf(mem_fmt_nov, "Message Buffer",
	    msgbufphys, msgbufphys + round_page(MSGBUFSIZE) - 1, round_page(MSGBUFSIZE) / PAGE_SIZE);
	printf(mem_fmt, "Free Memory", physical_freestart, physical_freeend-1,
	    KERN_PHYSTOV(physical_freestart), KERN_PHYSTOV(physical_freeend-1),
	    free_pages);
#endif

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
	       physical_freestart, free_pages, free_pages);
	printf("switching to new L1 page table  @%#lx...", kernel_l1pt.pv_pa);
#endif

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
	set_stackptr(PSR_FIQ32_MODE, fiqstack.pv_va + FIQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

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
	uvm_setpagesize();        /* initialize PAGE_SIZE-dependent variables */
	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

	/* disable power down counter in watch dog,
	   This must be done within 16 seconds of start-up. */
	ioreg16_write(NETWALKER_WDOG_VBASE + IMX_WDOG_WMCR, 0);

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#ifdef KGDB
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#ifdef DDB
#ifdef VERBOSE_INIT_ARM
	printf("ddb ");
#endif
	db_machine_init();

	/* Firmware doesn't load symbols. */
	ddb_init(0, NULL, NULL);

	if (boothowto & RB_KDB)
		Debugger();
#endif



	printf("initarm done.\n");

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

#if 0
void
process_kernel_args(char *args)
{

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

static void
init_clocks(void)
{
	extern void cortexa8_pmc_ccnt_init(void);

	cortexa8_pmc_ccnt_init();
}

struct iomux_setup {
	size_t  	pad_ctl_reg;
	uint32_t	pad_ctl_val;
	size_t  	mux_ctl_reg;
	uint32_t	mux_ctl_val;
};

#define	IOMUX_DATA(padname, mux, pad)	\
	IOMUX_DATA2(__CONCAT(IOMUXC_SW_MUX_CTL_PAD_,padname), mux, \
		    __CONCAT(IOMUXC_SW_PAD_CTL_PAD_,padname), pad)
		    
	
#define	IOMUX_DATA2(muxreg, muxval, padreg, padval)	\
	{						\
		.pad_ctl_reg = (padreg),		\
		.pad_ctl_val = (padval),		\
		.mux_ctl_reg = (muxreg),		\
		.mux_ctl_val = (muxval)			\
	}


const struct iomux_setup iomux_setup_data[] = {

	/* left buttons */
	IOMUX_DATA(EIM_EB2, IOMUX_CONFIG_ALT1,
		   PAD_CTL_HYS_ENABLE),
	/* right buttons */
	IOMUX_DATA(EIM_EB3, IOMUX_CONFIG_ALT1,
		   PAD_CTL_HYS_ENABLE),
		   

#if 0
	/* UART1 */
	IOMUX_DATA(UART1_RXD, IOMUX_CONFIG_ALT0,
		   (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
		    PAD_CTL_PUE_PULL	 | PAD_CTL_DSE_HIGH   |
		    PAD_CTL_SRE_FAST)),
	IOMUX_DATA(UART1_TXD, IOMUX_CONFIG_ALT0,
		   (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
		    PAD_CTL_PUE_PULL	 | PAD_CTL_DSE_HIGH   |
		    PAD_CTL_SRE_FAST)),
	IOMUX_DATA(UART1_RTS, IOMUX_CONFIG_ALT0,
		   (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
		    PAD_CTL_PUE_PULL	 | PAD_CTL_DSE_HIGH)),
	IOMUX_DATA(UART1_CTS, IOMUX_CONFIG_ALT0,
		   (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
		    PAD_CTL_PUE_PULL	 | PAD_CTL_DSE_HIGH)),
#else
	/* UART1 */
#if 1
	IOMUX_DATA(UART1_RXD, IOMUX_CONFIG_ALT0,
		   PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
#else
	IOMUX_DATA(UART1_RXD, IOMUX_CONFIG_ALT3,	/* gpio4[28] */
		   PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
#endif
	IOMUX_DATA(UART1_TXD, IOMUX_CONFIG_ALT0,
		   PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
	IOMUX_DATA(UART1_RTS, IOMUX_CONFIG_ALT0,
		   PAD_CTL_DSE_HIGH),
	IOMUX_DATA(UART1_CTS, IOMUX_CONFIG_ALT0,
		   PAD_CTL_DSE_HIGH),
#endif

};

static void
setup_ioports(void)
{
	int i;
	const struct iomux_setup *p;

#if 0	/* These are all done already by Netwalker's bootloader. */
	/* set IO multiplexor for UART1 */
	uint32_t reg;
	uint32_t addr;

	/* input */
	addr = NETWALKER_IOMUXC_VBASE + MUX_IN_UART1_IPP_UART_RXD_MUX;
	reg = INPUT_DAISY_0;
	ioreg_write(addr, reg);
	addr = NETWALKER_IOMUXC_VBASE + MUX_IN_UART1_IPP_UART_RTS_B;
	reg = INPUT_DAISY_0;
	ioreg_write(addr, reg);
#endif

	for (i=0; i < __arraycount(iomux_setup_data); ++i) {
		p = iomux_setup_data + i;

		ioreg_write(NETWALKER_IOMUXC_VBASE + 
			    p->pad_ctl_reg,
			    p->pad_ctl_val);
		ioreg_write(NETWALKER_IOMUXC_VBASE + 
			    p->mux_ctl_reg,
			    p->mux_ctl_val);
	}


#if 0	/* already done by bootloader */
	/* GPIO2[22,23]: input (left/right button)
	   GPIO2[21]: input (power button) */
	ioreg_write(NETWALKER_GPIO_VBASE(2) + GPIO_DIR,
		    ~__BITS(21,23) &
		    ioreg_read(NETWALKER_GPIO_VBASE(2) + GPIO_DIR));
#endif

#if 0	/* already done by bootloader */
	/* GPIO4[12]: input  (cover switch) */
	ioreg_write(NETWALKER_GPIO_VBASE(4) + GPIO_DIR,
		    ~__BIT(12) &
		    ioreg_read(NETWALKER_GPIO_VBASE(4) + GPIO_DIR));
#endif
}


#ifdef	CONSDEVNAME
const char consdevname[] = CONSDEVNAME;

#ifndef	CONMODE
#define	CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef	CONSPEED
#define	CONSPEED	115200
#endif

int consmode = CONMODE;
int consrate = CONSPEED;

#endif	/* CONSDEVNAME */

#ifndef	IMXUART_FREQ
#define	IMXUART_FREQ	66355200
#endif

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called)
		return;

	consinit_called = 1;

#ifdef	CONSDEVNAME

#if NIMXUART > 0
	imxuart_set_frequency(IMXUART_FREQ, 2);
#endif

#if (NIMXUART > 0) && defined(IMXUARTCONSOLE)
	if (strcmp(consdevname, "imxuart") == 0) {
		paddr_t consaddr;
#ifdef	CONADDR
		consaddr = CONADDR;
#else
		consaddr = IMX51_UART1_BASE;
#endif
		imxuart_cons_attach(&imx_bs_tag, consaddr, consrate, consmode);
	    return;
	}
#endif

#endif

#if (NWSDISPLAY > 0) && defined(IMXLCDCONSOLE)
	{
		extern void netwalker_cnattach(void);
		netwalker_cnattach();
	}
#endif
}

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "imxuart"
#endif
#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

const char kgdb_devname[20] = KGDB_DEVNAME;
int kgdb_mode = KGDB_DEVMODE;
int kgdb_addr = KGDB_DEVADDR;
extern int kgdb_rate;	/* defined in kgdb_stub.c */

void
kgdb_port_init(void)
{
#if (NIMXUART > 0)
	if (strcmp(kgdb_devname, "imxuart") == 0) {
		imxuart_kgdb_attach(&imx_bs_tag, kgdb_addr,
		kgdb_rate, kgdb_mode);
	    return;
	}

#endif
}
#endif


#ifdef DEBUG_IOPORTS
static void dump_sub(paddr_t addr, size_t size)
{
	paddr_t end = addr + size;

	for (; addr < end; addr += 4) {
		if (addr % 16 == 0)
			printf("%08x: ", (u_int)addr);
		printf("%08x ", ioreg_read(addr));

		if (addr % 16 == 12)
			printf("\n");
	}
	printf("\n");
}

void
dump_registers(void)
{
	paddr_t pa;
	int i;

	dump_sub(IOMUXC_BASE, IOMUXC_USBOH3_IPP_IND_UH3_STP_SELECT_INPUT + 4);

	for (i = 1; i <= 4; ++i) {
		dump_sub(GPIO_BASE(i), GPIO_SIZE);
	}

	printf("\nwatchdog: ");
	for (pa = WDOG1_BASE; pa <= WDOG1_BASE + IMX_WDOG_WMCR;
	     pa += 2) {
		printf("%04x ", *(volatile uint16_t *)pa);
	}
	printf("\n");

#if 0
	/* disable power down counter in watch dog,
	   This must be done within 16 seconds of start-up. */
	ioreg16_write(NETWALKER_WDOG_VBASE + IMX_WDOG_WMCR, 0);

	/* read left/right buttons */
	for (;;) {
		uint32_t reg;

		reg = ioreg_read(GPIO_BASE(2) + GPIO_DR);
		printf("\r%08x", reg);
		reg = ioreg_read(GPIO_BASE(4) + GPIO_DR);
		printf("  %08x", reg);

#if 0
		ioreg16_write(WDOG1_BASE + IMX_WDOG_WSR, WSR_MAGIC1);
		ioreg16_write(WDOG1_BASE + IMX_WDOG_WSR, WSR_MAGIC2);
#endif

	}
#endif

}
#endif
