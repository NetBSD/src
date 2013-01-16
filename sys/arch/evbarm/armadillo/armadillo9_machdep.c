/*	$NetBSD: armadillo9_machdep.c,v 1.21.2.2 2013/01/16 05:32:52 yamt Exp $	*/

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
 * Machine dependent functions for kernel setup for Armadillo.
 */

/*	Armadillo-9 physical memory map
	0000 0000 - 0fff ffff	reserved
	1000 0000 - 1000 000f	I/O Control Register
	1000 0010 - 11dd ffff	reserved
	1200 0000 - 1200 ffff	PC/104 I/O space (8bit)
	1201 0000 - 12ff ffff	reserved
	1300 0000 - 13ff ffff	PC/104 Memory space (8bit)
	1400 0000 - 1fff ffff	reserved
	2000 0000 - 21ff ffff	reserved
	2200 0000 - 2200 ffff	PC/104 I/O space (16bit)
	2201 0000 - 22ff ffff	reserved
	2300 0000 - 23ff ffff	PC/104 Memory space (16bit)
	2400 0000 - 2fff ffff	reserved
	3000 0000 - 3fff ffff	reserved
	4000 0000 - 43ff ffff	Compact Flash I/O space
	4400 0000 - 47ff ffff	reserved
	4800 0000 - 4bff ffff	Compact Flash Attribute space
	4c00 0000 - 4fff ffff	Compact Flash memory space
	5000 0000 - 5fff ffff	reserved
	6000 0000 - 607f ffff	Flash Memory (8MByte)
	6080 0000 - 6fff ffff	reserved
	7000 0000 - 7fff ffff	reserved
	8000 0000 - 8008 ffff	EP9315 Internal Register (AHB)
	8009 0000 - 8009 3fff	Internal Boot ROM (16kByte)
	8009 4000 - 8009 ffff	reserved
	800a 0000 - 800f ffff	EP9315 Internal Register (AHB)
	8010 0000 - 807f ffff	reserved
	8080 0000 - 8094 ffff	EP9315 Internal Register (APB)
	8095 0000 - 8fff ffff	reserved
	9000 0000 - bfff ffff	reserved
	c000 0000 - c1ff ffff	SDRAM (32MByte)
	c200 0000 - c3ff ffff	reserved
	c400 0000 - c5ff ffff	SDRAM (32MByte)
	c600 0000 - cfff ffff	reserved
	d000 0000 - ffff ffff	reserved
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadillo9_machdep.c,v 1.21.2.2 2013/01/16 05:32:52 yamt Exp $");

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

#include <net/if.h>
#include <net/if_ether.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#define	DRAM_BLOCKS	4
#include <machine/bootconfig.h>
#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	8
#define ABT_STACK_SIZE	8
#define UND_STACK_SIZE	8

#include <arm/arm32/machdep.h>

#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/ep93xxvar.h>

#include "epwdog.h"
#if NEPWDOG > 0
#include <arm/ep93xx/epwdogvar.h>
#endif
#include <arm/ep93xx/epwdogreg.h>

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

#include <evbarm/armadillo/armadillo9reg.h>
#include <evbarm/armadillo/armadillo9var.h>

struct armadillo_model_t *armadillo_model = 0;
static struct armadillo_model_t armadillo_model_table[] = {
	{ DEVCFG_ARMADILLO9, "Armadillo-9" },
	{ DEVCFG_ARMADILLO210, "Armadillo-210" },
	{ 0, "Armadillo(unknown model)" } };

#include "ksyms.h"

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xf0000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x0c000000


BootConfig bootconfig;	/* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_freeend_low;
vm_offset_t physical_end;
u_int free_pages;

vm_offset_t msgbufphys;

static struct arm32_dma_range armadillo9_dma_ranges[4];

#if NISA > 0
extern void isa_armadillo9_init(u_int, u_int); 
#endif

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

/* Prototypes */

void	consinit(void);
/*
 * Define the default console speed for the machine.
 */
#if NEPCOM > 0
#ifndef CONSPEED
#define CONSPEED B115200
#endif /* ! CONSPEED */

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

#ifndef CONUNIT
#define	CONUNIT	0
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
const unsigned long comaddr[] = {
	EP93XX_APB_UART1, EP93XX_APB_UART2 };
#endif

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
 * MAC address for the built-in Ethernet.
 */
uint8_t	armadillo9_ethaddr[ETHER_ADDR_LEN];

static void
armadillo9_device_register(device_t dev, void *aux)
{

	/* MAC address for the built-in Ethernet. */
	if (device_is_a(dev, "epe")) {
		prop_data_t pd = prop_data_create_data_nocopy(
		    armadillo9_ethaddr, ETHER_ADDR_LEN);
		KASSERT(pd != NULL);
		if (prop_dictionary_set(device_properties(dev),
					"mac-address", pd) == false) {
			printf("WARNING: unable to set mac-addr property "
			    "for %s\n", device_xname(dev));
		}
		prop_object_release(pd);
	}
}

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
		pmf_system_shutdown(boothowto);
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

	pmf_system_shutdown(boothowto);

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
#if NEPWDOG > 0
	epwdog_reset();
#else
	{
	uint32_t ctrl = EP93XX_APB_VBASE + EP93XX_APB_WDOG + EP93XX_WDOG_Ctrl;
	uint32_t val = EP93XX_WDOG_ENABLE;
	__asm volatile (
		"str %1, [%0]\n"
		:
		: "r" (ctrl), "r" (val)
	);
	}
#endif
	for (;;);
}

/* Static device mappings. */
static const struct pmap_devmap armadillo9_devmap[] = {
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

    {
	EP93XX_PCMCIA0_VBASE,
	EP93XX_PCMCIA0_HWBASE,
	EP93XX_PCMCIA_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

	/*
	 * IO8 and IO16 space *must* be mapped contiguously with
	 * IO8_VA == IO16_VA - 64 Mbytes.  ISA busmap driver depends
	 * on that!
	 */
    {
	ARMADILLO9_IO8_VBASE,
	ARMADILLO9_IO8_HWBASE,
	ARMADILLO9_IO8_SIZE,
	VM_PROT_READ|VM_PROT_WRITE,
	PTE_NOCACHE,
    },

    {
	ARMADILLO9_IO16_VBASE,
	ARMADILLO9_IO16_HWBASE,
	ARMADILLO9_IO16_SIZE,
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
	int loop;
	int loop1;
	u_int l1pagetable;
	struct bootparam_tag *bootparam_p;
	unsigned long devcfg;

	/*
	 * Since we map the on-board devices VA==PA, and the kernel
	 * is running VA==PA, it's possible for us to initialize
	 * the console now.
	 */
	consinit();

	/* identify model */
	devcfg = *((volatile unsigned long*)(EP93XX_APB_HWBASE 
					     + EP93XX_APB_SYSCON
					     + EP93XX_SYSCON_DeviceCfg));
	for (armadillo_model = &armadillo_model_table[0];
				armadillo_model->devcfg; armadillo_model++)
		if (devcfg == armadillo_model->devcfg)
			break;

	/* Talk to the user */
	printf("\nNetBSD/%s booting ...\n", armadillo_model->name);

	/* set some informations from bootloader */
	bootparam_p = (struct bootparam_tag *)bootparam;
	bootconfig.dramblocks = 0;
	while (bootparam_p->hdr.tag != BOOTPARAM_TAG_NONE) {
		switch (bootparam_p->hdr.tag) {
		case BOOTPARAM_TAG_MEM:
			if (bootconfig.dramblocks < DRAM_BLOCKS) {
#ifdef VERBOSE_INIT_ARM
			printf("dram[%d]: address=0x%08lx, size=0x%08lx\n",
						bootconfig.dramblocks,
						bootparam_p->u.mem.start,
						bootparam_p->u.mem.size);
#endif
				bootconfig.dram[bootconfig.dramblocks].address =
					bootparam_p->u.mem.start;
				bootconfig.dram[bootconfig.dramblocks].pages =
					bootparam_p->u.mem.size / PAGE_SIZE;
				bootconfig.dramblocks++;
			}
			break;
		case BOOTPARAM_TAG_CMDLINE:
#ifdef VERBOSE_INIT_ARM
			printf("cmdline: %s\n", bootparam_p->u.cmdline.cmdline);
#endif
			parse_mi_bootargs(bootparam_p->u.cmdline.cmdline);
			break;
		}
		bootparam_p = bootparam_tag_next(bootparam_p);
	}

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif
	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0xc0200000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the L1 table that we set up, we
	 * will panic.  We will update physical_freestart and
	 * physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = bootconfig.dram[0].address
			+ (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = 0xc0018000UL;
	physical_freeend = 0xc0200000UL;

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
	pmap_devmap_bootstrap(l1pagetable, armadillo9_devmap);

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
	cpu_setttb(kernel_l1pt.pv_pa, true);
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
	uvm_page_physload(atop(0xc0000000), atop(physical_freeend_low),
	    atop(0xc0000000), atop(physical_freeend_low),
	    VM_FREELIST_DEFAULT);
	physmem = bootconfig.dram[0].pages;
	for (loop = 1; loop < bootconfig.dramblocks; ++loop) {
		size_t start = bootconfig.dram[loop].address;
		size_t size = bootconfig.dram[loop].pages * PAGE_SIZE;
		uvm_page_physload(atop(start), atop(start + size),
				  atop(start), atop(start + size),
				  VM_FREELIST_DEFAULT);
		physmem += bootconfig.dram[loop].pages;
	}

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

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
	isa_armadillo9_init(ARMADILLO9_IO16_VBASE + ARMADILLO9_ISAIO,
		ARMADILLO9_IO16_VBASE + ARMADILLO9_ISAMEM);	
#endif

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef BOOTHOWTO
	boothowto = BOOTHOWTO;
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We have our own device_register() */
	evbarm_device_register = armadillo9_device_register;

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
consinit(void)
{
	static int consinit_called;
#if NEPCOM > 0
	bus_space_handle_t ioh;
#endif

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	/*
	 * Console devices are already mapped in VA.  Our devmap reflects
	 * this, so register it now so drivers can map the console
	 * device.
	 */
	pmap_devmap_register(armadillo9_devmap);

#if NEPCOM > 0
	bus_space_map(&ep93xx_bs_tag, EP93XX_APB_HWBASE + comaddr[CONUNIT], 
		EP93XX_APB_UART_SIZE, 0, &ioh);
        if (epcomcnattach(&ep93xx_bs_tag, EP93XX_APB_HWBASE + comaddr[CONUNIT], 
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
		armadillo9_dma_ranges[i].dr_sysbase = bootconfig.dram[i].address;
		armadillo9_dma_ranges[i].dr_busbase = bootconfig.dram[i].address;
		armadillo9_dma_ranges[i].dr_len = bootconfig.dram[i].pages * 
			PAGE_SIZE;
	}

	dmat = dma_tag_template;

	dmat->_ranges = armadillo9_dma_ranges;
	dmat->_nranges = bootconfig.dramblocks;

	return dmat;
}
