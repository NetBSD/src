/*	$NetBSD: marvell_machdep.c,v 1.6.2.2 2013/02/08 19:43:01 riz Exp $ */
/*
 * Copyright (c) 2007, 2008, 2010 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: marvell_machdep.c,v 1.6.2.2 2013/02/08 19:43:01 riz Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_ddb.h"
#include "opt_pci.h"
#include "opt_mvsoc.h"
#include "com.h"
#include "gtpci.h"
#include "mvpex.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <prop/proplib.h>

#include <dev/cons.h>
#include <dev/md.h>

#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/autoconf.h>
#include <machine/bootconfig.h>
#include <machine/pci_machdep.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/orionreg.h>
#include <arm/marvell/kirkwoodreg.h>
#include <arm/marvell/mvsocgppvar.h>

#include <evbarm/marvell/marvellreg.h>
#include <evbarm/marvell/marvellvar.h>

#include <ddb/db_extern.h>
#include <ddb/db_sym.h>

#include "ksyms.h"


/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)
#define KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x0c000000

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = 0xffff0000;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#ifdef IPKDB
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif

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
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

extern char _end[];

#define KERNEL_PT_SYS		0   /* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
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


#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifndef CONSPEED
#define CONSPEED	B115200	/* It's a setting of the default of u-boot */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
#endif

#include "opt_kgdb.h"
#ifdef KGDB
#include <sys/kgdb.h>
#endif

static void marvell_device_register(device_t, void *);
#if NGTPCI > 0 || NMVPEX > 0
static void marvell_startend_by_tag(int, uint64_t *, uint64_t *);
#endif

static void
marvell_system_reset(void)
{
	/* unmask soft reset */
	write_mlmbreg(MVSOC_MLMB_RSTOUTNMASKR,
	    MVSOC_MLMB_RSTOUTNMASKR_SOFTRSTOUTEN);
	/* assert soft reset */
	write_mlmbreg(MVSOC_MLMB_SSRR, MVSOC_MLMB_SSRR_SYSTEMSOFTRST);
	/* if we're still running, jump to the reset address */
	cpu_reset();
	/*NOTREACHED*/
}

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
		marvell_system_reset();
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
	marvell_system_reset();

	/*NOTREACHED*/
}

static inline
pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r" (ttb));

	return (pd_entry_t *)(ttb & ~((1<<14)-1));
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

static const struct pmap_devmap marvell_devmap[] = {
	{
		MARVELL_INTERREGS_VBASE,
		_A(MARVELL_INTERREGS_PBASE),
		_S(MARVELL_INTERREGS_SIZE),
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	{ 0, 0, 0, 0, 0 }
};

#undef  _A
#undef  _S

extern uint32_t *u_boot_args[];

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
	uint32_t target, attr, base, size;
	u_int l1pagetable;
	int loop, pt_index, cs, memtag = 0, iotag = 0, window;

	/* map some peripheral registers */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), marvell_devmap);

	mvsoc_bootstrap(MARVELL_INTERREGS_VBASE);

	/* Get ready for splfoo() */
	switch (mvsoc_model()) {
#ifdef ORION
	case MARVELL_ORION_1_88F1181:
	case MARVELL_ORION_1_88F5082:
	case MARVELL_ORION_1_88F5180N:
	case MARVELL_ORION_1_88F5181:
	case MARVELL_ORION_1_88F5182:
	case MARVELL_ORION_1_88F6082:
	case MARVELL_ORION_1_88F6183:
	case MARVELL_ORION_1_88W8660:
	case MARVELL_ORION_2_88F1281:
	case MARVELL_ORION_2_88F5281:
		orion_intr_bootstrap();

		memtag = ORION_TAG_PEX0_MEM;
		iotag = ORION_TAG_PEX0_IO;
		nwindow = ORION_MLMB_NWINDOW;
		nremap = ORION_MLMB_NREMAP;

		orion_getclks(MARVELL_INTERREGS_VBASE);
		break;
#endif	/* ORION */

#ifdef KIRKWOOD
	case MARVELL_KIRKWOOD_88F6180:
	case MARVELL_KIRKWOOD_88F6192:
	case MARVELL_KIRKWOOD_88F6281:
		kirkwood_intr_bootstrap();

		memtag = KIRKWOOD_TAG_PEX_MEM;
		iotag = KIRKWOOD_TAG_PEX_IO;
		nwindow = KIRKWOOD_MLMB_NWINDOW;
		nremap = KIRKWOOD_MLMB_NREMAP;

		kirkwood_getclks(MARVELL_INTERREGS_VBASE);
		break;
#endif	/* KIRKWOOD */

#ifdef MV78XX0
	case MARVELL_MV78XX0_MV78100:
	case MARVELL_MV78XX0_MV78200:
		mv78xx0_intr_bootstrap();

		memtag = MV78XX0_TAG_PEX_MEM;
		iotag = MV78XX0_TAG_PEX_IO;
		nwindow = MV78XX0_MLMB_NWINDOW;
		nremap = MV78XX0_MLMB_NREMAP;

		mv78xx0_getclks(MARVELL_INTERREGS_VBASE);
		break;
#endif	/* MV78XX0 */

	default:
		/* We can't output console here yet... */
		panic("unknown model...\n");

		/* NOTREACHED */
	}

	/* Reset PCI-Express space to window register. */
	window = mvsoc_target(memtag, &target, &attr, NULL, NULL);
	write_mlmbreg(MVSOC_MLMB_WCR(window),
	    MVSOC_MLMB_WCR_WINEN |
	    MVSOC_MLMB_WCR_TARGET(target) |
	    MVSOC_MLMB_WCR_ATTR(attr) |
	    MVSOC_MLMB_WCR_SIZE(MARVELL_PEXMEM_SIZE));
	write_mlmbreg(MVSOC_MLMB_WBR(window),
	    MARVELL_PEXMEM_PBASE & MVSOC_MLMB_WBR_BASE_MASK);
#ifdef PCI_NETBSD_CONFIGURE
	if (window < nremap) {
		write_mlmbreg(MVSOC_MLMB_WRLR(window),
		    MARVELL_PEXMEM_PBASE & MVSOC_MLMB_WRLR_REMAP_MASK);
		write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
	}
#endif
	window = mvsoc_target(iotag, &target, &attr, NULL, NULL);
	write_mlmbreg(MVSOC_MLMB_WCR(window),
	    MVSOC_MLMB_WCR_WINEN |
	    MVSOC_MLMB_WCR_TARGET(target) |
	    MVSOC_MLMB_WCR_ATTR(attr) |
	    MVSOC_MLMB_WCR_SIZE(MARVELL_PEXIO_SIZE));
	write_mlmbreg(MVSOC_MLMB_WBR(window),
	    MARVELL_PEXIO_PBASE & MVSOC_MLMB_WBR_BASE_MASK);
#ifdef PCI_NETBSD_CONFIGURE
	if (window < nremap) {
		write_mlmbreg(MVSOC_MLMB_WRLR(window),
		    MARVELL_PEXIO_PBASE & MVSOC_MLMB_WRLR_REMAP_MASK);
		write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
	}
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/*
	 * U-Boot doesn't use the virtual memory.
	 *
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 * 0x00000000 - 0x0fffffff    SDRAM Bank 0 (max 256MB)
	 * 0x10000000 - 0x1fffffff    SDRAM Bank 1 (max 256MB)
	 * 0x20000000 - 0x2fffffff    SDRAM Bank 2 (max 256MB)
	 * 0x30000000 - 0x3fffffff    SDRAM Bank 3 (max 256MB)
	 * 0xf1000000 - 0xf10fffff    SoC Internal Registers
	 */

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	/* Talk to the user */
#define BDSTR(s)	_BDSTR(s)
#define _BDSTR(s)	#s
	printf("\nNetBSD/evbarm (" BDSTR(EVBARM_BOARDTYPE) ") booting ...\n");

	/* copy command line U-Boot gave us */
	strncpy(bootargs, (char *)u_boot_args[3], sizeof(bootargs));

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	bootconfig.dramblocks = 0;
	physical_end = physmem = 0;
	for (cs = MARVELL_TAG_SDRAM_CS0; cs <= MARVELL_TAG_SDRAM_CS3; cs++) {
		mvsoc_target(cs, &target, &attr, &base, &size);
		if (size == 0)
			continue;

		bootconfig.dram[bootconfig.dramblocks].address = base;
		bootconfig.dram[bootconfig.dramblocks].pages = size / PAGE_SIZE;

		if (base != physical_end)
			panic("memory hole not support");

		physical_end += size;
		physmem += size / PAGE_SIZE;

		bootconfig.dramblocks++;
	}

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0xa0008000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the L1 table that we set up, we
	 * will panic.  We will update physical_freestart and
	 * physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;

	/*
	 * Our kernel is at the beginning of memory, so set our free space to
	 * all the memory after the kernel.
	 */
	physical_freestart = KERN_VTOPHYS(round_page((vaddr_t)_end));
	physical_freeend = physical_end;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

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
	printf("Allocating page tables\n");
#endif

	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
	    physical_freestart, free_pages, free_pages);
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
		panic("initarm: out of memory");			\
	free_pages -= (np);						\
	(var).pv_va = KERN_PHYSTOV((var).pv_pa);			\
	memset((char *)(var).pv_va, 0, ((np) * PAGE_SIZE));

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
		panic("initarm: Failed to align the kernel page directory");

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(systempage, 1);
	systempage.pv_va = ARM_VECTORS_HIGH;

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

	/* Allocate the message buffer. */
	{
		pv_addr_t msgbuf;

		valloc_pages(msgbuf, round_page(MSGBUFSIZE) / PAGE_SIZE);
		msgbufphys = msgbuf.pv_pa;
	}

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

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, ARM_VECTORS_HIGH & -L2_S_SEGSIZE,
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
		size_t textsize = (uintptr_t)etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t)_end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;

		logical = 0x00000000;	/* offset of kernel in RAM */

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
	    UPAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop)
		pmap_map_chunk(l1pagetable, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/*
	 * Map integrated peripherals at same address in first level page
	 * table so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1pagetable, marvell_devmap);

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table  @%#lx...", kernel_l1pt.pv_pa);
#endif

	cpu_setttb(kernel_l1pt.pv_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

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

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

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

	/* we've a specific device_register routine */
	evbarm_device_register = marvell_device_register;

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0
	{
		extern int mvuart_cnattach(bus_space_tag_t, bus_addr_t, int,
					   uint32_t, int);

		if (mvuart_cnattach(&mvsoc_bs_tag, 
		    MARVELL_INTERREGS_VBASE + MVSOC_COM0_BASE,
		    comcnspeed, mvTclk, comcnmode))
			panic("can't init serial console");
	}
#else
	panic("serial console not configured");
#endif
}


static void
marvell_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

#if NCOM > 0
	if (device_is_a(dev, "com") &&
	    device_is_a(device_parent(dev), "mvsoc"))
		prop_dictionary_set_uint32(dict, "frequency", mvTclk);
#endif
	if (device_is_a(dev, "gtidmac")) {
		prop_dictionary_set_uint32(dict,
		    "dmb_speed", mvTclk * sizeof(uint32_t));	/* XXXXXX */
		prop_dictionary_set_uint32(dict,
		    "xore-irq-begin", ORION_IRQ_XOR0);
	}
#if NGTPCI > 0 && defined(ORION)
	if (device_is_a(dev, "gtpci")) {
		extern struct bus_space
		    orion_pci_io_bs_tag, orion_pci_mem_bs_tag;
		extern struct arm32_pci_chipset arm32_gtpci_chipset;

		prop_data_t io_bs_tag, mem_bs_tag, pc;
		prop_array_t int2gpp;
		prop_number_t gpp;
		uint64_t start, end;
		int i, j;
		static struct {
			const char *boardtype;
			int pin[PCI_INTERRUPT_PIN_MAX];
		} hints[] = {
			{ "kuronas_x4",
			    { 11, PCI_INTERRUPT_PIN_NONE } },

			{ NULL,
			    { PCI_INTERRUPT_PIN_NONE } },
		};

		arm32_gtpci_chipset.pc_conf_v = device_private(dev);
		arm32_gtpci_chipset.pc_intr_v = device_private(dev);

		io_bs_tag = prop_data_create_data_nocopy(
		    &orion_pci_io_bs_tag, sizeof(struct bus_space));
		KASSERT(io_bs_tag != NULL);
		prop_dictionary_set(dict, "io-bus-tag", io_bs_tag);
		prop_object_release(io_bs_tag);
		mem_bs_tag = prop_data_create_data_nocopy(
		    &orion_pci_mem_bs_tag, sizeof(struct bus_space));
		KASSERT(mem_bs_tag != NULL);
		prop_dictionary_set(dict, "mem-bus-tag", mem_bs_tag);
		prop_object_release(mem_bs_tag);

		pc = prop_data_create_data_nocopy(&arm32_gtpci_chipset,
		    sizeof(struct arm32_pci_chipset));
		KASSERT(pc != NULL);
		prop_dictionary_set(dict, "pci-chipset", pc);
		prop_object_release(pc);

		marvell_startend_by_tag(ORION_TAG_PCI_IO, &start, &end);
		prop_dictionary_set_uint64(dict, "iostart", start);
		prop_dictionary_set_uint64(dict, "ioend", end);
		marvell_startend_by_tag(ORION_TAG_PCI_MEM, &start, &end);
		prop_dictionary_set_uint64(dict, "memstart", start);
		prop_dictionary_set_uint64(dict, "memend", end);
		prop_dictionary_set_uint32(dict,
		    "cache-line-size", arm_dcache_align);

		/* Setup the hint for interrupt-pin. */
#define BDSTR(s)		_BDSTR(s)
#define _BDSTR(s)		#s
#define THIS_BOARD(str)		(strcmp(str, BDSTR(EVBARM_BOARDTYPE)) == 0)
		for (i = 0; hints[i].boardtype != NULL; i++)
			if (THIS_BOARD(hints[i].boardtype))
				break;
		if (hints[i].boardtype == NULL)
			return;

		int2gpp =
		    prop_array_create_with_capacity(PCI_INTERRUPT_PIN_MAX + 1);

		/* first set dummy */
		gpp = prop_number_create_integer(0);
		prop_array_add(int2gpp, gpp);
		prop_object_release(gpp);

		for (j = 0; hints[i].pin[j] != PCI_INTERRUPT_PIN_NONE; j++) {
			gpp = prop_number_create_integer(hints[i].pin[j]);
			prop_array_add(int2gpp, gpp);
			prop_object_release(gpp);
		}
		prop_dictionary_set(dict, "int2gpp", int2gpp);
	}
#endif	/* NGTPCI > 0 && defined(ORION) */
#if NMVPEX > 0
	if (device_is_a(dev, "mvpex")) {
#ifdef ORION
		extern struct bus_space
		    orion_pex0_io_bs_tag, orion_pex0_mem_bs_tag,
		    orion_pex1_io_bs_tag, orion_pex1_mem_bs_tag;
#endif
#ifdef KIRKWOOD
		extern struct bus_space
		    kirkwood_pex_io_bs_tag, kirkwood_pex_mem_bs_tag;
#endif
		extern struct arm32_pci_chipset arm32_mvpex0_chipset;
#ifdef ORION
		extern struct arm32_pci_chipset arm32_mvpex1_chipset;
#endif

#ifdef ORION
		struct marvell_attach_args *mva = aux;
#endif
		struct bus_space *mvpex_io_bs_tag, *mvpex_mem_bs_tag;
		struct arm32_pci_chipset *arm32_mvpex_chipset;
		prop_data_t io_bs_tag, mem_bs_tag, pc;
		uint64_t start, end;
		int iotag, memtag;

		switch (mvsoc_model()) {
#ifdef ORION
		case MARVELL_ORION_1_88F5180N:
		case MARVELL_ORION_1_88F5181:
		case MARVELL_ORION_1_88F5182:
		case MARVELL_ORION_1_88W8660:
		case MARVELL_ORION_2_88F5281:
			if (mva->mva_offset == MVSOC_PEX_BASE) {
				mvpex_io_bs_tag = &orion_pex0_io_bs_tag;
				mvpex_mem_bs_tag = &orion_pex0_mem_bs_tag;
				arm32_mvpex_chipset = &arm32_mvpex0_chipset;
				iotag = ORION_TAG_PEX0_IO;
				memtag = ORION_TAG_PEX0_MEM;
			} else {
				mvpex_io_bs_tag = &orion_pex1_io_bs_tag;
				mvpex_mem_bs_tag = &orion_pex1_mem_bs_tag;
				arm32_mvpex_chipset = &arm32_mvpex1_chipset;
				iotag = ORION_TAG_PEX1_IO;
				memtag = ORION_TAG_PEX1_MEM;
			}
			break;
#endif

#ifdef KIRKWOOD
		case MARVELL_KIRKWOOD_88F6180:
		case MARVELL_KIRKWOOD_88F6192:
		case MARVELL_KIRKWOOD_88F6281:
			mvpex_io_bs_tag = &kirkwood_pex_io_bs_tag;
			mvpex_mem_bs_tag = &kirkwood_pex_mem_bs_tag;
			arm32_mvpex_chipset = &arm32_mvpex0_chipset;
			iotag = KIRKWOOD_TAG_PEX_IO;
			memtag = KIRKWOOD_TAG_PEX_MEM;
			break;
#endif

		default:
			return;
		}

		arm32_mvpex_chipset->pc_conf_v = device_private(dev);
		arm32_mvpex_chipset->pc_intr_v = device_private(dev);

		io_bs_tag = prop_data_create_data_nocopy(
		    mvpex_io_bs_tag, sizeof(struct bus_space));
		KASSERT(io_bs_tag != NULL);
		prop_dictionary_set(dict, "io-bus-tag", io_bs_tag);
		prop_object_release(io_bs_tag);
		mem_bs_tag = prop_data_create_data_nocopy(
		    mvpex_mem_bs_tag, sizeof(struct bus_space));
		KASSERT(mem_bs_tag != NULL);
		prop_dictionary_set(dict, "mem-bus-tag", mem_bs_tag);
		prop_object_release(mem_bs_tag);

		pc = prop_data_create_data_nocopy(arm32_mvpex_chipset,
		    sizeof(struct arm32_pci_chipset));
		KASSERT(pc != NULL);
		prop_dictionary_set(dict, "pci-chipset", pc);
		prop_object_release(pc);

		marvell_startend_by_tag(iotag, &start, &end);
		prop_dictionary_set_uint64(dict, "iostart", start);
		prop_dictionary_set_uint64(dict, "ioend", end);
		marvell_startend_by_tag(memtag, &start, &end);
		prop_dictionary_set_uint64(dict, "memstart", start);
		prop_dictionary_set_uint64(dict, "memend", end);
		prop_dictionary_set_uint32(dict,
		    "cache-line-size", arm_dcache_align);
	}
#endif
}

#if NGTPCI > 0 || NMVPEX > 0
static void
marvell_startend_by_tag(int tag, uint64_t *start, uint64_t *end)
{
	uint32_t base, size;
	int win;

	win = mvsoc_target(tag, NULL, NULL, &base, &size);
	if (size != 0) {
		if (win < nremap)
			*start = read_mlmbreg(MVSOC_MLMB_WRLR(win)) |
			    ((read_mlmbreg(MVSOC_MLMB_WRHR(win)) << 16) << 16);
		else
			*start = base;
		*end = *start + size - 1;
	}
}
#endif
