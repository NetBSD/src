/*	$NetBSD: rpi_machdep.c,v 1.3.2.2 2012/08/09 06:36:51 jdc Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rpi_machdep.c,v 1.3.2.2 2012/08/09 06:36:51 jdc Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_broadcom.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/bus.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/vmparam.h>
#include <machine/bootconfig.h>
#include <machine/pmap.h>

#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_pmvar.h>

#include <evbarm/rpi/rpi.h>

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#endif

#include "ksyms.h"

static void setup_real_page_tables(void);

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */
u_int cpu_reset_address;

/* Define various stack sizes in pages */
#define FIQ_STACK_SIZE	1
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#define UND_STACK_SIZE	1

BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_end;
u_int free_pages;
int physmem = 0;

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t fiqstack;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

extern char etext[];
extern char __data_start[], _edata[];
extern char __bss_start[], __bss_end__[];
extern char _end[];

#define KERNEL_PT_SYS		0  /* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1  /* Page table for mapping kernel */
#define KERNEL_PT_KERNEL_NUM	4

#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
/* Page tables for mapping kernel VM */
#define KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERNEL_BASE_PHYS	physical_start
#define KERN_VTOPHYS(va) \
	((paddr_t)((vaddr_t)va - KERNEL_BASE + KERNEL_BASE_PHYS))
#define KERN_PHYSTOV(pa) \
	((vaddr_t)((paddr_t)pa - KERNEL_BASE_PHYS + KERNEL_BASE))

#define	PLCONADDR 0x20201000

#ifndef CONSDEVNAME
#define CONSDEVNAME "plcom"
#endif

#ifndef PLCONSPEED
#define PLCONSPEED B115200
#endif
#ifndef PLCONMODE
#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef PLCOMCNUNIT
#define PLCOMCNUNIT -1
#endif

#if (NPLCOM > 0)
static const bus_addr_t consaddr = (bus_addr_t)PLCONADDR;

int plcomcnspeed = PLCONSPEED;
int plcomcnmode = PLCONMODE;
#endif

#include "opt_kgdb.h"
#ifdef KGDB
#include <sys/kgdb.h>
#endif

void
cpu_reboot(int howto, char *bootstr)
{

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
		printf("rebooting...\r\n");
		bcm2835_system_reset();
	}

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
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cngetc();
	}

	printf("rebooting...\r\n");
	bcm2835_system_reset();

	/*NOTREACHED*/
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
#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static inline
pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc   p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1<<14)-1));
}

#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap rpi_devmap[] = {
	{
		_A(RPI_KERNEL_IO_VBASE),	/* 0xf2000000 */
		_A(RPI_KERNEL_IO_PBASE),	/* 0x20000000 */
		_S(RPI_KERNEL_IO_VSIZE),	/* 16Mb */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{ 0, 0, 0, 0, 0 }
};

#undef  _A
#undef  _S

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
 */
u_int
initarm(void *arg)
{

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), rpi_devmap);

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	bootargs[0] = '\0';

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	bootconfig.dramblocks = 1;
	physical_end = (MEMSIZE * 1024 * 1024); /* MEMSIZE */
	physmem = physical_end / PAGE_SIZE;
	physical_start = 0;

	bootconfig.dram[0].address = 0x0;
	bootconfig.dram[0].pages = physmem;

	/*
	 * Our kernel is at the beginning of memory, so set our free space to
	 * all the memory after the kernel.
	 */
	physical_freestart = KERN_VTOPHYS(round_page((vaddr_t)_end));
	physical_freeend = physical_end;
	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * This is going to do all the hard work of setting up the first and
	 * and second level page tables.  Pages of memory will be allocated
	 * and mapped for other structures that are required for system
	 * operation.  When it returns, physical_freestart and free_pages will
	 * have been updated to reflect the allocations that were made.  In
	 * addition, kernel_l1pt, kernel_pt_table[], systempage, irqstack,
	 * abtstack, undstack, kernelstack, msgbufphys will be set to point to
	 * the memory that was allocated for them.
	 */
	setup_real_page_tables();

	/*
	 * Okay, the kernel starts 8kB in from the bottom of physical
	 * memory.  We are going to allocate our bootstrap pages upwards
	 * from physical_freestart.
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
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	    physical_freestart, free_pages, free_pages);
#endif

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init.
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
	set_stackptr(PSR_FIQ32_MODE,
	    fiqstack.pv_va + FIQ_STACK_SIZE * PAGE_SIZE);
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
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);
   
	/* flag used in locore.s */
	extern u_int cpu_reset_needs_v4_MMU_disable;
	cpu_reset_needs_v4_MMU_disable = 0;
	cpu_reset_address = (u_int) bcm2835_system_reset;

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

#ifdef KGDB
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return kernelstack.pv_va + USPACE_SVC_STACK_TOP;
}

void
consinit(void)
{
	static int consinit_called = 0;
#if (NPLCOM > 0 && defined(PLCONSOLE))
	static struct plcom_instance rpi_pi = {
		.pi_type = PLCOM_TYPE_PL011,
		.pi_flags = PLC_FLAG_32BIT_ACCESS,
		.pi_iot = &bcm2835_bs_tag,
		.pi_size = BCM2835_UART0_SIZE
	};
#endif
	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if (NPLCOM > 0 && defined(PLCONSOLE))
	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 */
	rpi_pi.pi_iobase = consaddr;
	
	plcomcnattach(&rpi_pi, plcomcnspeed, BCM2835_UART0_CLK,
	    plcomcnmode, PLCOMCNUNIT);

#endif
}

static void
setup_real_page_tables(void)
{
	/*
	 * We need to allocate some fixed page tables to get the kernel going.
	 *
	 * We are going to allocate our bootstrap pages from the beginning of
	 * the free space that we just calculated.  We allocate one page
	 * directory and a number of page tables and store the physical
	 * addresses in the kernel_pt_table array.
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

	/*
	 * Define a macro to simplify memory allocation.  As we allocate the
	 * memory, make sure that we don't walk over our temporary first level
	 * translation table.
	 */
#define valloc_pages(var, np)						\
	(var).pv_pa = physical_freestart;				\
	physical_freestart += ((np) * PAGE_SIZE);			\
	if (physical_freestart > (physical_freeend - L1_TABLE_SIZE))	\
		panic("%s: out of memory", __func__);			\
	free_pages -= (np);						\
	(var).pv_va = KERN_PHYSTOV((var).pv_pa);			\
	memset((char *)(var).pv_va, 0, ((np) * PAGE_SIZE));

	int loop, pt_index;

	pt_index = 0;
	kernel_l1pt.pv_pa = 0;
	kernel_l1pt.pv_va = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if ((physical_freestart & (L1_TABLE_SIZE - 1)) == 0 &&
		    kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
		} else {
			valloc_pages(kernel_pt_table[pt_index],
			    L2_TABLE_SIZE / PAGE_SIZE);
			++pt_index;
		}
	}

	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa ||
	    (kernel_l1pt.pv_pa & (L1_TABLE_SIZE - 1)) != 0)
		panic("%s: Failed to align the kernel page directory", __func__);

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

	/* Allocate the message buffer. */
	pv_addr_t msgbuf;
	int msgbuf_pgs = round_page(MSGBUFSIZE) / PAGE_SIZE;
	valloc_pages(msgbuf, msgbuf_pgs);
	msgbufphys = msgbuf.pv_pa;

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
	vaddr_t l1_va = kernel_l1pt.pv_va;
	paddr_t l1_pa = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1_va, ARM_VECTORS_HIGH & ~(0x00400000 - 1),
	    &kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_KERNEL_NUM; loop++)
		pmap_link_l2pt(l1_va, KERNEL_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_KERNEL + loop]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; loop++)
		pmap_link_l2pt(l1_va, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	extern char etext[], _end[];
	size_t textsize = (uintptr_t)etext - KERNEL_BASE;
	size_t totalsize = (uintptr_t)_end - KERNEL_BASE;
	u_int logical;

	textsize = (textsize + PGOFSET) & ~PGOFSET;
	totalsize = (totalsize + PGOFSET) & ~PGOFSET;

	logical = 0x00000000;	/* offset of kernel in RAM */

	logical += pmap_map_chunk(l1_va, KERNEL_BASE + logical,
	    physical_start + logical, textsize,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	logical += pmap_map_chunk(l1_va, KERNEL_BASE + logical,
	    physical_start + logical, totalsize - textsize,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	pmap_map_chunk(l1_va, fiqstack.pv_va, fiqstack.pv_pa,
	    FIQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1_va, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1_va, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop)
		pmap_map_chunk(l1_va, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	/* Map the vector page. */
	pmap_map_entry(l1_va, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/*
	 * Map integrated peripherals at same address in first level page
	 * table so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1_va, rpi_devmap);

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
	       KERN_VTOPHYS(KERNEL_BASE), KERN_VTOPHYS(etext-1),
	       (vaddr_t)KERNEL_BASE, (vaddr_t)etext-1,
	       (int)(textsize / PAGE_SIZE));
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
	    (vaddr_t)ARM_VECTORS_HIGH, (vaddr_t)ARM_VECTORS_HIGH + PAGE_SIZE - 1,
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
	    msgbufphys, msgbufphys + msgbuf_pgs * PAGE_SIZE - 1, msgbuf_pgs);
	printf(mem_fmt, "Free Memory", physical_freestart, physical_freeend-1,
	    KERN_PHYSTOV(physical_freestart), KERN_PHYSTOV(physical_freeend-1),
	    free_pages);
#endif
	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table  @%#lx...", l1_pa);
#endif

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(l1_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

#ifdef VERBOSE_INIT_ARM
	printf("OK.\n");
#endif
}
