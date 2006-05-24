/*	$NetBSD: nslu2_machdep.c,v 1.2.2.1 2006/05/24 15:47:54 tron Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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

/*
 * Machine dependant functions for kernel setup for Linksys NSLU2
 * using RedBoot firmware.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nslu2_machdep.c,v 1.2.2.1 2006/05/24 15:47:54 tron Exp $");

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

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>

#include <evbarm/nslu2/nslu2reg.h>

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "opt_ipkdb.h"
#include "ksyms.h"

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define	KERNEL_VM_SIZE		0x0C000000


/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
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
extern int end;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* L2 table for mapping zero page */

#define KERNEL_PT_KERNEL	1	/* L2 table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
#define	KERNEL_PT_IO		(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
					/* L2 tables for mapping kernel VM */ 
#define KERNEL_PT_VMDATA	(KERNEL_PT_IO + 1)
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

void	consinit(void);
u_int	cpu_get_control   __P((void));

/*
 * Define the default console speed for the board.  This is generally
 * what the firmware provided with the board defaults to.
 */
#ifndef CONSPEED
#define CONSPEED B115200
#endif /* ! CONSPEED */

#ifndef CONUNIT
#define	CONUNIT	0
#endif

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB)) | CS8) /* 8N1 */
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

	if ((howto & (RB_HALT | RB_POWERDOWN)) == RB_HALT) {
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

 reset:
	/*
	 * Make really really sure that all interrupts are disabled,
	 */
	(void) disable_interrupts(I32_bit | F32_bit);

	if (howto & RB_POWERDOWN) {
		uint32_t reg;

		printf("powering down...\n\r");
		/* Delay to allow the UART's Tx FIFO to drain */
		delay(50000);

#define	GPRD(r)		*((volatile uint32_t *)(IXP425_GPIO_VBASE+(r)))
#define	GPWR(r,v)	*((volatile uint32_t *)(IXP425_GPIO_VBASE+(r))) = (v)

		/*
		 * Power-down pin requires a short pulse
		 */
		reg = GPRD(IXP425_GPIO_GPOUTR);
		reg |= 1u << GPIO_POWER_OFF;
		GPWR(IXP425_GPIO_GPOUTR, reg);

		delay(1000);

		reg = GPRD(IXP425_GPIO_GPOUTR);
		reg &= ~(1u << GPIO_POWER_OFF);
		GPWR(IXP425_GPIO_GPOUTR, reg);

		delay(500000);
		printf("POWER OFF FAILED! TRYING TO REBOOT INSTEAD\n\r");
	}

	printf("rebooting...\n\r");

#define	WDWR(r,v)	*((volatile uint32_t *)(IXP425_TIMER_VBASE+(r))) = (v)
	/* Force a watchdog reset */
	WDWR(IXP425_OST_WDOG_KEY, OST_WDOG_KEY_MAJICK);
	WDWR(IXP425_OST_WDOG_ENAB, OST_WDOG_ENAB_RST_ENA);
	WDWR(IXP425_OST_WDOG, 0x1000);
	WDWR(IXP425_OST_WDOG_ENAB,
	    OST_WDOG_ENAB_RST_ENA | OST_WDOG_ENAB_CNT_ENA);

	delay(500000);

	/* ...and if that didn't work, just croak. */
	printf("RESET FAILED!\n");

	for (;;);
}

/* Static device mappings. */
static const struct pmap_devmap nslu2_devmap[] = {
	/* Physical/Virtual address for I/O space */
	{
		IXP425_IO_VBASE,
		IXP425_IO_HWBASE,
		IXP425_IO_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	/* Expansion Bus */
	{
		IXP425_EXP_VBASE,
		IXP425_EXP_HWBASE,
		IXP425_EXP_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	/* IXP425 PCI Configuration */
	{
		IXP425_PCI_VBASE,
		IXP425_PCI_HWBASE,
		IXP425_PCI_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	/* SDRAM Controller */
	{
		IXP425_MCU_VBASE,
		IXP425_MCU_HWBASE,
		IXP425_MCU_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	/* PCI Memory Space */
	{
		IXP425_PCI_MEM_VBASE,
		IXP425_PCI_MEM_HWBASE,
		IXP425_PCI_MEM_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	/* Flash memory */
	{
		NSLU2_FLASH_VBASE,
		NSLU2_FLASH_HWBASE,
		NSLU2_FLASH_SIZE,
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
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg)
{
	extern vaddr_t xscale_cache_clean_addr;
#ifdef DIAGNOSTIC
	extern vsize_t xscale_minidata_clean_size;
#endif
	int loop;
	int loop1;
	u_int kerneldatasize;
	u_int l1pagetable;
	u_int freemempos;
	pv_addr_t kernel_l1pt;
	uint32_t reg;

	/*
	 * Make sure the power-down GPIO pin is configured correctly, as
	 * cpu_reboot() may be called early on (e.g. from within ddb(9)).
	 */
	/* Pin is active-high, so make sure it's driven low */
	reg = GPRD(IXP425_GPIO_GPOUTR);
	reg &= ~(1u << GPIO_POWER_OFF);
	GPWR(IXP425_GPIO_GPOUTR, reg);

	/* Set as output */
	reg = GPRD(IXP425_GPIO_GPOER);
	reg &= ~(1u << GPIO_POWER_OFF);
	GPWR(IXP425_GPIO_GPOER, reg);

	/*
	 * Since we map v0xf0000000 == p0xc8000000, it's possible for
	 * us to initialize the console now.
	 */
	consinit();

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (Linksys NSLU2) booting ...\n");
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* XXX overwrite bootconfig to hardcoded values */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = 0x10000000;
	bootconfig.dram[0].pages = ixp425_sdram_size() / PAGE_SIZE;

	kerneldatasize = (u_int32_t)&end - (u_int32_t)KERNEL_TEXT_BASE;

#ifdef VERBOSE_INIT_ARM
        printf("kernsize=0x%x\n", kerneldatasize);
#endif
        kerneldatasize = ((kerneldatasize - 1) & ~(PAGE_SIZE * 4 - 1)) + PAGE_SIZE * 8;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0x10200000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the L1 table that we set up, we
	 * will panic.  We will update physical_freestart and
	 * physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = physical_start
                + (KERNEL_TEXT_BASE - KERNEL_BASE) + kerneldatasize;
        physical_freeend = physical_end;

	physmem = (physical_end - physical_start) / PAGE_SIZE;

	/* Tell the user about the memory */
#ifdef VERBOSE_INIT_ARM
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);

	printf("Allocating page tables\n");
#endif
	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

	freemempos = 0x10000000;

#ifdef VERBOSE_INIT_ARM
        printf("physical_start = 0x%08lx, physical_end = 0x%08lx\n",
                physical_start, physical_end);
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)				\
	alloc_pages((var).pv_pa, (np));			\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#if 0
#define alloc_pages(var, np)				\
	physical_freeend -= ((np) * PAGE_SIZE);		\
	if (physical_freeend < physical_freestart)	\
		panic("initarm: out of memory");	\
	(var) = physical_freeend;			\
	free_pages -= (np);				\
	memset((char *)(var), 0, ((np) * PAGE_SIZE));
#else
#define alloc_pages(var, np)				\
        (var) = freemempos;                             \
        memset((char *)(var), 0, ((np) * PAGE_SIZE));   \
        freemempos += (np) * PAGE_SIZE;
#endif

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
	 * Allocate a page for the system page.
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

	/*
	 * XXX Defer this to later so that we can reclaim the memory
	 * XXX used by the RedBoot page tables.
	 */
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
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; loop++)
		pmap_link_l2pt(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

	pmap_link_l2pt(l1pagetable, IXP425_IO_VBASE,
	    &kernel_pt_table[KERNEL_PT_IO]);

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

	/* Map the Mini-Data cache clean area. */
	xscale_setup_minidata(l1pagetable, minidataclean.pv_va,
	    minidataclean.pv_pa);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

        /*
         * Map the IXP425 registers
         */
	pmap_devmap_bootstrap(l1pagetable, nslu2_devmap);

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

		physical_freestart = physical_start +
		    (((((uintptr_t) _end) + PGOFSET) & ~PGOFSET) -
		     KERNEL_BASE);
		physical_freeend = physical_end;
		free_pages =
		    (physical_freeend - physical_freestart) / PAGE_SIZE;
	}

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
	ixp425_intr_init();
#ifdef VERBOSE_INIT_ARM
	printf("\nAll initialize done!\nNow Starting NetBSD, Hear we go!\n");
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

/*
 * consinit
 */
void
consinit(void)
{
	static int consinit_called;
	static const bus_addr_t addrs[2] = {
		IXP425_UART0_HWBASE, IXP425_UART1_HWBASE
	};

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	pmap_devmap_register(nslu2_devmap);

	if (comcnattach(&ixp425_a4x_bs_tag, addrs[comcnunit],
	    comcnspeed, IXP425_UART_FREQ, COM_TYPE_PXA2x0, comcnmode))
		panic("can't init serial console (UART%d)", comcnunit);
}
