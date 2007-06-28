/*	$NetBSD: machdep.c,v 1.5 2007/06/28 16:07:12 nonaka Exp $	*/
/*	$OpenBSD: zaurus_machdep.c,v 1.25 2006/06/20 18:24:04 todd Exp $	*/

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
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
 * Machine dependant functions for kernel setup for 
 * Intel DBPXA250 evaluation board (a.k.a. Lubbock).
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.5 2007/06/28 16:07:12 nonaka Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_pmap_debug.h"
#include "opt_md.h"
#include "opt_com.h"
#include "md.h"
#include "ksyms.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <sys/conf.h>
#include <sys/queue.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <arch/zaurus/zaurus/zaurus_reg.h>
#include <arch/zaurus/zaurus/zaurus_var.h>

#include <zaurus/dev/scoopreg.h>

#include <dev/ic/comreg.h>

#if 0	/* XXX */
#include "apm.h"
#endif	/* XXX */
#if NAPM > 0
#include <zaurus/dev/zapmvar.h>
#endif

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x04000000)

/*
 * The range 0xc4000000 - 0xcfffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x0c000000

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

int zaurusmod;			/* Zaurus model */

BootConfig bootconfig;		/* Boot config storage */
char *boot_file = NULL;
char *boot_args = NULL;

paddr_t physical_start;
paddr_t physical_freestart;
paddr_t physical_freeend;
paddr_t physical_end;
u_int free_pages;
int physmem = 0;

#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
extern pv_addr_t kernelstack;
pv_addr_t minidataclean;

paddr_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	32
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
				        /* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	8	/* start with 32MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

const char *console = "glass";
int glass_console = 0;

char bootargs[MAX_BOOT_STRING];

/* Prototypes */
void	consinit(void);
void	dumpsys(void);
void	process_kernel_args(char *);
#ifdef KGDB
void	kgdb_port_init(void);
#endif

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
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if (!(howto & RB_NOSYNC)) {
		bootsync();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Wait 3s */
	delay(3 * 1000 * 1000);

	/* Say NO to interrupts */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
#if NAPM > 0
		if (howto & RB_POWERDOWN) {
			printf("\nAttempting to power down...\n");
			zapm_poweroff();
		}
#endif
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	delay(1 * 1000 * 1000);
	zaurus_restart();

	printf("REBOOT FAILED!!!\n");
	for (;;)
		continue;
	/*NOTREACHED*/
}

/*
 * Do a GPIO reset, immediately causing the processor to begin the normal
 * boot sequence.  See 2.7 Reset in the PXA27x Developer's Manual for the
 * summary of effects of this kind of reset.
 */
void
zaurus_restart(void)
{
	uint32_t rv;

	rv = pxa2x0_memctl_read(MEMCTL_MSC0);
	if ((rv & 0xffff0000) == 0x7ff00000) {
		pxa2x0_memctl_write(MEMCTL_MSC0, (rv & 0xffff) | 0x7ee00000);
	}

	/* External reset circuit presumably asserts nRESET_GPIO. */
	pxa2x0_gpio_set_function(89, GPIO_OUT | GPIO_SET);
	delay(1 * 1000* 1000);	/* wait 1s */
}

static inline pd_entry_t *
read_ttb(void)
{
	u_long ttb;

	__asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1 << 14) - 1));
}

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel, and stay at the same address
 * throughout whole kernel's life time.
 *
 * We use this table twice; once with bootstrap page table, and once
 * with kernel's page table which we build up in initarm().
 *
 * Since we map these registers into the bootstrap page table using
 * pmap_devmap_bootstrap() which calls pmap_map_chunk(), we map
 * registers segment-aligned and segment-rounded in order to avoid
 * using the 2nd page tables.
 */
#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap zaurus_devmap[] = {
    {
	    ZAURUS_GPIO_VBASE,
	    _A(PXA2X0_GPIO_BASE),
	    _S(PXA2X0_GPIO_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_CLKMAN_VBASE,
	    _A(PXA2X0_CLKMAN_BASE),
	    _S(PXA2X0_CLKMAN_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_INTCTL_VBASE,
	    _A(PXA2X0_INTCTL_BASE),
	    _S(PXA2X0_INTCTL_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_MEMCTL_VBASE,
	    _A(PXA2X0_MEMCTL_BASE),
	    _S(PXA2X0_MEMCTL_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_SCOOP0_VBASE,
	    _A(C3000_SCOOP0_BASE),
	    _S(SCOOP_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_SCOOP1_VBASE,
	    _A(C3000_SCOOP1_BASE),
	    _S(SCOOP_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_FFUART_VBASE,
	    _A(PXA2X0_FFUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_BTUART_VBASE,
	    _A(PXA2X0_BTUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    ZAURUS_STUART_VBASE,
	    _A(PXA2X0_STUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },

    {0, 0, 0, 0, 0,}
};

#undef	_A
#undef	_S

void green_on(int virt);
void
green_on(int virt)
{
	/* clobber green led p */
	volatile uint16_t *p;

	if (virt) {
		p = (volatile uint16_t *)(ZAURUS_SCOOP0_VBASE + SCOOP_GPWR);
	} else {
		p = (volatile uint16_t *)(C3000_SCOOP0_BASE + SCOOP_GPWR);
	}

	*p |= (1 << SCOOP0_LED_GREEN);
}

void irda_on(int virt);
void
irda_on(int virt)
{
	/* clobber IrDA led p */
	volatile uint16_t *p;

	if (virt) {
		/* XXX scoop1 registers are not page-aligned! */
		int ofs = C3000_SCOOP1_BASE - trunc_page(C3000_SCOOP1_BASE);
		p = (volatile uint16_t *)(ZAURUS_SCOOP1_VBASE + ofs + SCOOP_GPWR);
	} else {
		p = (volatile uint16_t *)(C3000_SCOOP1_BASE + SCOOP_GPWR);
	}

	*p &= ~(1 << SCOOP1_IR_ON);
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
 *   Relocating the kernel to the bottom of physical memory
 */
u_int
initarm(void *arg)
{
#ifdef DIAGNOSTIC
	extern vsize_t xscale_minidata_clean_size; /* used in KASSERT */
#endif
	extern vaddr_t xscale_cache_clean_addr;
	int loop;
	int loop1;
	u_int l1pagetable;
	pv_addr_t kernel_l1pt;
	paddr_t memstart;
	psize_t memsize;

	/* Get ready for zaurus_restart() */
	pxa2x0_memctl_bootstrap(PXA2X0_MEMCTL_BASE);

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* Get ready for splfoo() */
	pxa2x0_intr_bootstrap(PXA2X0_INTCTL_BASE);

	/* map some peripheral registers at static I/O area */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), zaurus_devmap);

	/* set new memctl register address so that zaurus_restart() doesn't
	   touch illegal address. */
	pxa2x0_memctl_bootstrap(ZAURUS_MEMCTL_VBASE);

	/* set new intc register address so that splfoo() doesn't
	   touch illegal address.  */
	pxa2x0_intr_bootstrap(ZAURUS_INTCTL_VBASE);

	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
	/* XXX should really be done after setting up the console, but we
	 * XXX need to parse the console selection flags right now. */
	process_kernel_args((char *)0xa0200000 - MAX_BOOT_STRING - 1);
#ifdef RAMDISK_HOOKS
        boothowto |= RB_DFLTROOT;
#endif /* RAMDISK_HOOKS */

	/*
	 * This test will work for now but has to be revised when support
	 * for other models is added.
	 */
	if ((cputype & ~CPU_ID_XSCALE_COREREV_MASK) == CPU_ID_PXA27X)
		zaurusmod = ZAURUS_C3000;
	else
		zaurusmod = ZAURUS_C860;

	/* setup a serial console for very early boot */
	pxa2x0_gpio_bootstrap(ZAURUS_GPIO_VBASE);
	pxa2x0_clkman_bootstrap(ZAURUS_CLKMAN_VBASE);
	if (strcmp(console, "glass") != 0)
		consinit();
#ifdef KGDB
	kgdb_port_init();
#endif

	/* Talk to the user */
	printf("\nNetBSD/zaurus booting ...\n");

	{
		/* XXX - all Zaurus have this for now, fix memory sizing */
		memstart = 0xa0000000;
		memsize =  0x04000000; /* 64MB */
	}

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independant */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;

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
	physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = 0xa0009000UL;
	physical_freeend = 0xa0200000UL;

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
	printf("minidataclean: p0x%08lx v0x%08lx, size = %ld\n",
	    minidataclean.pv_pa, minidataclean.pv_va,
	    xscale_minidata_clean_size);
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
	pmap_link_l2pt(l1pagetable, 0x00000000,
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

	/* Now we fill in the L2 pagetable for the kernel static code/data
	 * and the symbol table. */
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
		pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
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
#if 0
	/* MULTI-ICE requires that page 0 is NC/NB so that it can download the
	 * cache-clean code there.  */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);
#else
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
#endif

	/*
	 * map integrated peripherals at same address in l1pagetable
	 * so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1pagetable, zaurus_devmap);

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
		    ((((uintptr_t) _end + PGOFSET) & ~PGOFSET) - KERNEL_BASE);
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

	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL);

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
	 * This just fills in a slighly better one.
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
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, KERNEL_VM_BASE,
	    KERNEL_VM_BASE + KERNEL_VM_SIZE);

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

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
	db_machine_init();

	/* Firmware doesn't load symbols. */
	ddb_init(0, NULL, NULL);

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return (kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
process_kernel_args(char *args)
{
	char *cp = args;

	if (cp == NULL || *(u_int *)cp != BOOTARGS_MAGIC) {
		boothowto = RB_AUTOBOOT;
		return;
	}

	/* Eat the cookie */
	*(u_int *)cp = 0;
	cp += sizeof(u_int);

	boothowto = 0;

	/* Make a local copy of the bootargs */
	strncpy(bootargs, cp, MAX_BOOT_STRING - sizeof(u_int));

	cp = bootargs;
	boot_file = bootargs;

	for (;;) {
		/* Skip white-space */
		while (*cp == ' ')
			++cp;

		if (*cp == '\0')
			break;

		if (*cp != '-') {
			/* kernel image filename */
			if (boot_file == NULL)
				boot_file = cp;

			/* Skip the kernel image filename */
			while (*cp != ' ' && *cp != '\0')
				++cp;
			if (*cp == '\0')
				break;

			*cp++ = '\0';
			continue;
		}

		/* options */
		if (*++cp != '\0') {
			int fl = 0;

			switch (*cp) {
			case 'a':
				fl |= RB_ASKNAME;
				break;
			case 'c':
				fl |= RB_USERCONF;
				break;
			case 'd':
				fl |= RB_KDB;
				break;
			case 's':
				fl |= RB_SINGLE;
				break;
			/* XXX undocumented console switching flags */
			case '0':
				console = "ffuart";
				break;
			case '1':
				console = "btuart";
				break;
			case '2':
				console = "stuart";
				break;
			default:
				printf("unknown option `%c'\n", *cp);
				break;
			}
			boothowto |= fl;
		}
		++cp;
	}
}

/*
 * Console
 */
#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comvar.h>
#endif

#include "lcd.h"
#include "wsdisplay.h"

#ifndef CONSPEED
#define CONSPEED B9600
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME	"ffuart"
#endif
const char kgdb_devname[] = KGDB_DEVNAME;

#if (NCOM > 0)
#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comkgdbmode = KGDB_DEVMODE;
#endif /* NCOM */
#endif /* KGDB */

void
consinit(void)
{
	static int consinit_called = 0;
#if (NCOM > 0) && defined(COM_PXA2X0)
	paddr_t paddr;
	u_int cken = 0;
#endif

	if (consinit_called)
		return;
	consinit_called = 1;

#if (NCOM > 0) && defined(COM_PXA2X0)
#ifdef KGDB
	if (strcmp(kgdb_devname, console) == 0) {
		/* port is reserved for kgdb */
	} else
#endif
	if (strcmp(console, "ffuart") == 0) {
		paddr = PXA2X0_FFUART_BASE;
		cken = CKEN_FFUART;
		pxa2x0_gpio_set_function(34, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(39, GPIO_ALT_FN_2_OUT);
		pxa2x0_gpio_set_function(35, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(40, GPIO_ALT_FN_2_OUT);
		pxa2x0_gpio_set_function(41, GPIO_ALT_FN_2_OUT);
	} else if (strcmp(console, "btuart") == 0) {
		paddr = PXA2X0_BTUART_BASE;
		cken = CKEN_BTUART;
		pxa2x0_gpio_set_function(42, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(43, GPIO_ALT_FN_2_OUT);
		pxa2x0_gpio_set_function(44, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(45, GPIO_ALT_FN_2_OUT);
	} else if (strcmp(console, "stuart") == 0) {
		paddr = PXA2X0_STUART_BASE;
		cken = CKEN_STUART;
		pxa2x0_gpio_set_function(46, GPIO_ALT_FN_2_IN);
		pxa2x0_gpio_set_function(47, GPIO_ALT_FN_1_OUT);
		irda_on(0);
	} else
#endif
	if (strcmp(console, "glass") == 0) {
#if (NLCD > 0) && (NWSDISPLAY > 0)
		extern void lcd_cnattach(void);

		glass_console = 1;
		lcd_cnattach();
#endif
	}

#if (NCOM > 0) && defined(COM_PXA2X0)
	if (cken != 0 && comcnattach(&pxa2x0_a4x_bs_tag, paddr, comcnspeed,
	    PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode) == 0) {
		pxa2x0_clkman_config(cken, 1);
	}
#endif
}

#ifdef KGDB
void
kgdb_port_init(void)
{
#if (NCOM > 0) && defined(COM_PXA2X0)
	paddr_t paddr;
	u_int cken;

	if (strcmp(kgdb_devname, "ffuart") == 0) {
		paddr = PXA2X0_FFUART_BASE;
		cken = CKEN_FFUART;
		pxa2x0_gpio_set_function(34, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(39, GPIO_ALT_FN_2_OUT);
		pxa2x0_gpio_set_function(35, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(40, GPIO_ALT_FN_2_OUT);
		pxa2x0_gpio_set_function(41, GPIO_ALT_FN_2_OUT);
	} else if (strcmp(kgdb_devname, "btuart") == 0) {
		paddr = PXA2X0_BTUART_BASE;
		cken = CKEN_BTUART;
		pxa2x0_gpio_set_function(42, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(43, GPIO_ALT_FN_2_OUT);
		pxa2x0_gpio_set_function(44, GPIO_ALT_FN_1_IN);
		pxa2x0_gpio_set_function(45, GPIO_ALT_FN_2_OUT);
	} else if (strcmp(kgdb_devname, "stuart") == 0) {
		paddr = PXA2X0_STUART_BASE;
		cken = CKEN_STUART;
		pxa2x0_gpio_set_function(46, GPIO_ALT_FN_2_IN);
		pxa2x0_gpio_set_function(47, GPIO_ALT_FN_1_OUT);
		irda_on(0);
	} else
		return;

	if (com_kgdb_attach(&pxa2x0_a4x_bs_tag, paddr,
	    kgdb_rate, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comkgdbmode) == 0) {
		pxa2x0_clkman_config(cken, 1);
	}
#endif
}
#endif
