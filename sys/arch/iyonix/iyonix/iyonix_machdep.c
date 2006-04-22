/*	$NetBSD: iyonix_machdep.c,v 1.4.6.1 2006/04/22 11:37:40 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: iyonix_machdep.c,v 1.4.6.1 2006/04/22 11:37:40 simonb Exp $");

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

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <acorn32/include/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <iyonix/iyonix/iyonixreg.h>
#include <iyonix/iyonix/obiovar.h>

#include <dev/pci/ppbreg.h>
#include <dev/ic/i8259reg.h>

#include "opt_ipkdb.h"
#include "ksyms.h"

#define	KERNEL_TEXT_BASE	KERNEL_BASE
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
 *
 * XXX Not actually used on Iyonix -- clean up the generic
 * ARM code.
 */

u_int cpu_reset_address = 0x00000000;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#ifdef IPKDB
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif

struct bootconfig bootconfig;		/* Boot config storage */
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

#define KERNEL_PT_SYS		0	/* L2 table for mapping zero page */

#define KERNEL_PT_KERNEL	1	/* L2 table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4

					/* L2 table for mapping i80321 */
#define	KERNEL_PT_IOPXS		(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)

					/* L2 tables for mapping kernel VM */ 
#define KERNEL_PT_VMDATA	(KERNEL_PT_IOPXS + 1)
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

char iyonix_macaddr[ETHER_ADDR_LEN];

/* Prototypes */

void	consinit(void);
void	iyonix_pic_init(void);
void	iyonix_read_machineid(void);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Define the default console speed for the machine.
 */
#ifndef CONSPEED
#define CONSPEED B9600
#endif /* ! CONSPEED */

#ifndef CONUNIT
#define	CONUNIT	0
#endif

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
int comcnunit = CONUNIT;

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
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
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
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n\r");
 reset:
	/*
	 * Make really really sure that all interrupts are disabled,
	 * and poke the Internal Bus and Peripheral Bus reset lines.
	 */
	(void) disable_interrupts(I32_bit|F32_bit);
	*(volatile uint32_t *)(IYONIX_80321_VBASE + VERDE_ATU_BASE +
	    ATU_PCSR) = PCSR_RIB | PCSR_RPB;

	/* ...and if that didn't work, just croak. */
	printf("RESET FAILED!\n");
	for (;;);
}

/* Static device mappings. */
static const struct pmap_devmap iyonix_devmap[] = {
    /*
     * Map the on-board devices VA == PA so that we can access them
     * with the MMU on or off.
     */
    {
	IYONIX_OBIO_BASE,
	IYONIX_OBIO_BASE,
	IYONIX_OBIO_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

    {
	IYONIX_IOW_VBASE,
	VERDE_OUT_XLATE_IO_WIN0_BASE,
	VERDE_OUT_XLATE_IO_WIN_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
   },

   {
	IYONIX_80321_VBASE,
	VERDE_PMMR_BASE,
	VERDE_PMMR_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
   },

   {
	IYONIX_FLASH_BASE,
	IYONIX_FLASH_BASE,
	IYONIX_FLASH_SIZE,
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

/* Read out the Machine ID from the flash, and stash it away for later use. */

void
iyonix_read_machineid(void)
{
	volatile uint32_t *flashbase = (uint32_t *)IYONIX_FLASH_BASE;
	volatile uint16_t *flashword = (uint16_t *)IYONIX_FLASH_BASE;
	union {
		uint32_t w[2];
		uint8_t  b[8];
	} machid;

	/* Enter SecSi Sector Region */
	flashword[0x555] = 0xAA;
	flashword[0x2AA] = 0x55;
	flashword[0x555] = 0x88;

	machid.w[0] = flashbase[0];
	machid.w[1] = flashbase[1];

	iyonix_macaddr[0] = machid.b[6];
	iyonix_macaddr[1] = machid.b[5];
	iyonix_macaddr[2] = machid.b[4];
	iyonix_macaddr[3] = machid.b[3];
	iyonix_macaddr[4] = machid.b[2];
	iyonix_macaddr[5] = machid.b[1];

	/* Exit SecSi Sector Region */
	flashword[0x555] = 0xAA;
	flashword[0x2AA] = 0x55;
	flashword[0x555] = 0x90;
	flashword[0x555] = 0x00;
}

#define IYONIX_PIC_WRITE(a,v) (*((char *)IYONIX_OBIO_BASE + (a)) = (v))

void
iyonix_pic_init(void)
{
	IYONIX_PIC_WRITE(IYONIX_MASTER_PIC + PIC_ICW1, ICW1_IC4|ICW1_SELECT);
	IYONIX_PIC_WRITE(IYONIX_MASTER_PIC + PIC_ICW2, ICW2_IRL(0));
	IYONIX_PIC_WRITE(IYONIX_MASTER_PIC + PIC_ICW3, ICW3_CASCADE(2));
	IYONIX_PIC_WRITE(IYONIX_MASTER_PIC + PIC_ICW4, ICW4_8086);
	IYONIX_PIC_WRITE(IYONIX_MASTER_PIC + PIC_OCW1, 0x0); /* Unmask */

	IYONIX_PIC_WRITE(IYONIX_SLAVE_PIC + PIC_ICW1, ICW1_IC4|ICW1_SELECT);
	IYONIX_PIC_WRITE(IYONIX_SLAVE_PIC + PIC_ICW2, ICW2_IRL(0));
	IYONIX_PIC_WRITE(IYONIX_SLAVE_PIC + PIC_ICW3, ICW3_CASCADE(1));
	IYONIX_PIC_WRITE(IYONIX_SLAVE_PIC + PIC_ICW4, ICW4_8086);
	IYONIX_PIC_WRITE(IYONIX_SLAVE_PIC + PIC_OCW1, 0x0); /* Unmask */

}

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
	struct bootconfig *passed_bootconfig = arg;
	extern vaddr_t xscale_cache_clean_addr;
#ifdef DIAGNOSTIC
	extern vsize_t xscale_minidata_clean_size;
#endif
	extern char _end[];
	int loop;
	int loop1;
	u_int l1pagetable;
	pv_addr_t kernel_l1pt;
	paddr_t memstart;
	psize_t memsize;

	/* Calibrate the delay loop. */
	i80321_calibrate_delay();

	/*
	 * Since we map the on-board devices VA==PA, and the kernel
	 * is running VA==PA, it's possible for us to initialize
	 * the console now.
	 */
	consinit();

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/iyonix booting ...\n");
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * We are currently running with the MMU enabled and the
	 * entire address space mapped VA==PA.
	 */

	/*
	 * Fetch the SDRAM start/size from the i80321 SDRAM configuration
	 * registers.
	 */
	i80321_sdram_bounds(&obio_bs_tag, VERDE_PMMR_BASE + VERDE_MCU_BASE,
	    &memstart, &memsize);

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Ensure bootconfig has valid magic */
	if (passed_bootconfig->magic != BOOTCONFIG_MAGIC)
		printf("Bad bootconfig magic: %x\n", bootconfig.magic);

	bootconfig = *passed_bootconfig;

	/* Fake bootconfig structure for anything that still needs it */
	/* XXX must make the memory description h/w independent */
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;
	bootconfig.dramblocks = 1;

	/*
	 * Set up the variables that define the availability of
	 * physical memory.
	 */
	physical_start = memstart;
	physical_end = physical_start + memsize;

	physical_freestart = physical_start +
	    (((uintptr_t) _end - KERNEL_TEXT_BASE + PGOFSET) & ~PGOFSET);
	physical_freeend = physical_end;

	physmem = (physical_end - physical_start) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * The kernel is loaded at the base of physical memory. We allocate
	 * pages upwards from the top of the kernel.
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
	(var) = physical_freestart;			\
	physical_freestart += ((np) * PAGE_SIZE);	\
	if (physical_freeend < physical_freestart)	\
		panic("initarm: out of memory");	\
	free_pages -= (np);				\
	memset((char *)(var), 0, ((np) * PAGE_SIZE));

	loop1 = 0;
	kernel_l1pt.pv_pa = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if ((physical_freestart & (L1_TABLE_SIZE - 1)) == 0
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
	alloc_pages(systempage.pv_pa, 1);

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

	/* Allocate enough pages for cleaning the Mini-Data cache. */
	KASSERT(xscale_minidata_clean_size <= PAGE_SIZE);
	valloc_pages(minidataclean, 1);

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
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, ARM_VECTORS_HIGH & ~(0x00400000 - 1),
	    &kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);
	pmap_link_l2pt(l1pagetable, IYONIX_IOPXS_VBASE,
	    &kernel_pt_table[KERNEL_PT_IOPXS]);
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
		
		logical = 0;	/* offset of kernel in RAM */
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

	/* Map the Mini-Data cache clean area. */
	xscale_setup_minidata(l1pagetable, minidataclean.pv_va,
	    minidataclean.pv_pa);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the statically mapped devices. */
	pmap_devmap_bootstrap(l1pagetable, iyonix_devmap);

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

	iyonix_read_machineid();

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
	i80321_intr_init();

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

	iyonix_pic_init();

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
consinit(void)
{
	static const bus_addr_t comcnaddrs[] = {
		IYONIX_UART1,		/* com0 */
	};
	static int consinit_called;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	/*
	 * Console devices are mapped VA==PA.  Our devmap reflects
	 * this, so register it now so drivers can map the console
	 * device.
	 */
	pmap_devmap_register(iyonix_devmap);

#if NCOM > 0
	if (comcnattach(&obio_bs_tag, comcnaddrs[comcnunit], comcnspeed,
	    COM_FREQ, COM_TYPE_NORMAL, comcnmode))
	{
		panic("can't init serial console @%lx", comcnaddrs[comcnunit]);
	}
#else
	panic("serial console @%lx not configured", comcnaddrs[comcnunit]);
#endif
#if KGDB
#if NCOM > 0
	if (strcmp(kgdb_devname, "com") == 0) {
		com_kgdb_attach(&obio_bs_tag, kgdb_devaddr, kgdb_devrate,
				COM_FREQ, COM_TYPE_NORMAL, kgdb_devmode);
	}
#endif	/* NCOM > 0 */
#endif	/* KGDB */
}
