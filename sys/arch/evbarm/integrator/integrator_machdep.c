/*	$NetBSD: integrator_machdep.c,v 1.47 2003/09/06 10:57:12 rearnsha Exp $	*/

/*
 * Copyright (c) 2001,2002 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ARM LTD
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
 * Machine dependant functions for kernel setup for integrator board
 *
 * Created      : 24/11/97
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: integrator_machdep.c,v 1.47 2003/09/06 10:57:12 rearnsha Exp $");

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
#include <machine/intr.h>
#include <evbarm/ifpga/irqhandler.h>	/* XXX XXX XXX */
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <evbarm/integrator/integrator_boot.h>

#include "opt_ipkdb.h"
#include "pci.h"
#include "ksyms.h"

void ifpga_reset(void) __attribute__((noreturn));

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
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

u_int cpu_reset_address = (u_int) ifpga_reset;

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
vm_offset_t physical_end;
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

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* L2 table for mapping zero page */

#define KERNEL_PT_KERNEL	1	/* L2 table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	2
					/* L2 tables for mapping kernel VM */
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

static void	integrator_sdram_bounds	(paddr_t *, psize_t *);

void	consinit(void);

/* A load of console goo. */
#include "vga.h"
#if NVGA > 0
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if NPCKBC > 0
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#ifndef CONCOMADDR
#define CONCOMADDR 0x3f8
#endif
#endif

/*
 * Define the default console speed for the board.  This is generally
 * what the firmware provided with the board defaults to.
 */
#ifndef CONSPEED
#define CONSPEED B115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

#include "plcom.h"
#if (NPLCOM > 0)
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <evbarm/ifpga/ifpgamem.h>
#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgavar.h>
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "plcom"
#endif

#ifndef PLCONSPEED
#define PLCONSPEED B38400
#endif
#ifndef PLCONMODE
#define PLCONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef PLCOMCNUNIT
#define PLCOMCNUNIT -1
#endif

int plcomcnspeed = PLCONSPEED;
int plcomcnmode = PLCONMODE;

#if 0
extern struct consdev kcomcons;
static void kcomcnputc(dev_t, int);
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
		doshutdownhooks();
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		ifpga_reset();
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
	ifpga_reset();
	/*NOTREACHED*/
}

/* Statically mapped devices. */
static const struct pmap_devmap integrator_devmap[] = {
#if NPLCOM > 0 && defined(PLCONSOLE)
	{
		UART0_BOOT_BASE,
		IFPGA_IO_BASE + IFPGA_UART0,
		1024 * 1024,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},

	{
		UART1_BOOT_BASE,
		IFPGA_IO_BASE + IFPGA_UART1,
		1024 * 1024,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},
#endif
#if NPCI > 0
	{
		IFPGA_PCI_IO_VBASE,
		IFPGA_PCI_IO_BASE,
		IFPGA_PCI_IO_VSIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},

	{
		IFPGA_PCI_CONF_VBASE,
		IFPGA_PCI_CONF_BASE,
		IFPGA_PCI_CONF_VSIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE
	},
#endif

	{
		0,
		0,
		0,
		0,
		0
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
	int loop;
	int loop1;
	u_int l1pagetable;
	extern char etext asm ("_etext");
	extern char end asm ("_end");
	pv_addr_t kernel_l1pt;
	paddr_t memstart;
	psize_t memsize;
	vm_offset_t physical_freestart;
	vm_offset_t physical_freeend;
#if NPLCOM > 0 && defined(PLCONSOLE)
	static struct bus_space plcom_bus_space;
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

#if NPLCOM > 0 && defined(PLCONSOLE)
	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 * Once all the memory map changes are complete we can call consinit()
	 * and not have to worry about things moving.
	 */

	if (PLCOMCNUNIT == 0) {
		ifpga_create_io_bs_tag(&plcom_bus_space, (void*)0xfd600000);
		plcomcnattach(&plcom_bus_space, 0, plcomcnspeed,
		    IFPGA_UART_CLK, plcomcnmode, PLCOMCNUNIT);
	} else if (PLCOMCNUNIT == 1) {
		ifpga_create_io_bs_tag(&plcom_bus_space, (void*)0xfd700000);
		plcomcnattach(&plcom_bus_space, 0, plcomcnspeed,
		    IFPGA_UART_CLK, plcomcnmode, PLCOMCNUNIT);
	}
#endif

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (Integrator) booting ...\n");
#endif

	/*
	 * Fetch the SDRAM start/size from the CM configuration registers.
	 */
	integrator_sdram_bounds(&memstart, &memsize);

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system ...\n");
#endif

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;
	bootconfig.dram[0].flags = BOOT_DRAM_CAN_DMA | BOOT_DRAM_PREFER;

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
	 * We assume that the kernel is loaded into bank[0].
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = 0;

	/* Update the address of the first free 16KB chunk of physical memory */
	physical_freestart = ((uintptr_t) &end - KERNEL_BASE + PGOFSET)
	    & ~PGOFSET;
	if (physical_freestart < bootconfig.dram[0].address)
		physical_freestart = bootconfig.dram[0].address;
	physical_freeend = bootconfig.dram[0].address +
	    bootconfig.dram[0].pages * PAGE_SIZE;

	for (loop = 0, physmem = 0; loop < bootconfig.dramblocks; loop++) {
		paddr_t memoryblock_end;

		memoryblock_end = bootconfig.dram[loop].address +
		    bootconfig.dram[loop].pages * PAGE_SIZE;
		if (memoryblock_end > physical_end)
			physical_end = memoryblock_end;
		if (bootconfig.dram[loop].address < physical_start)
			physical_start = bootconfig.dram[loop].address;

		physmem += bootconfig.dram[loop].pages;
	}

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

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free pages = %d (0x%08x)\n",
	       physical_freestart, physmem, physmem);
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)				\
	alloc_pages((var).pv_pa, (np));			\
	(var).pv_va = KERNEL_BASE + (var).pv_pa;

#define alloc_pages(var, np)				\
	(var) = physical_freestart;			\
	physical_freestart += ((np) * PAGE_SIZE);	\
	if (physical_freeend < physical_freestart)	\
		panic("initarm: out of memory");	\
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

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	{
		size_t textsize = (uintptr_t) &etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) &end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;

		logical = 0x00200000;	/* offset of kernel in RAM */

		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    logical, textsize, VM_PROT_READ | VM_PROT_WRITE,
		    PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    logical, totalsize - textsize,
		    VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);
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
#if 1
	/* MULTI-ICE requires that page 0 is NC/NB so that it can download
	   the cache-clean code there.  */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);
#else
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
#endif

	/* Map the statically mapped devices. */
	pmap_devmap_bootstrap(l1pagetable, integrator_devmap);

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
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

#ifdef PLCONSOLE
	/*
	 * The IFPGA registers have just moved.
	 * Detach the diagnostic serial port and reattach at the new address.
	 */
	plcomcndetach();
#endif

	/*
	 * XXX this should only be done in main() but it useful to
	 * have output earlier ...
	 */
	consinit();

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
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */

	/* Round the start up and the end down to a page.  */
	physical_freestart = (physical_freestart + PGOFSET) & ~PGOFSET;
	physical_freeend &= ~PGOFSET;

	for (loop = 0; loop < bootconfig.dramblocks; loop++) {
		paddr_t block_start = (paddr_t) bootconfig.dram[loop].address;
		paddr_t block_end = block_start +
		    (bootconfig.dram[loop].pages * PAGE_SIZE);

		if (loop == 0) {
			block_start = physical_freestart;
			block_end = physical_freeend;
		}


		uvm_page_physload(atop(block_start), atop(block_end),
		    atop(block_start), atop(block_end),
		    (bootconfig.dram[loop].flags & BOOT_DRAM_PREFER) ?
		    VM_FREELIST_DEFAULT : VM_FREELIST_DEFAULT + 1);
	}

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
	irq_init();

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
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
	static int consinit_called = 0;
#if NPLCOM > 0 && defined(PLCONSOLE)
	static struct bus_space plcom_bus_space;
#endif
#if 0
	char *console = CONSDEVNAME;
#endif

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NPLCOM > 0 && defined(PLCONSOLE)
	if (PLCOMCNUNIT == 0) {
		ifpga_create_io_bs_tag(&plcom_bus_space,
		    (void*)UART0_BOOT_BASE);
		if (plcomcnattach(&plcom_bus_space, 0, plcomcnspeed,
		    IFPGA_UART_CLK, plcomcnmode, PLCOMCNUNIT))
			panic("can't init serial console");
		return;
	} else if (PLCOMCNUNIT == 1) {
		ifpga_create_io_bs_tag(&plcom_bus_space,
		    (void*)UART0_BOOT_BASE);
		if (plcomcnattach(&plcom_bus_space, 0, plcomcnspeed,
		    IFPGA_UART_CLK, plcomcnmode, PLCOMCNUNIT))
			panic("can't init serial console");
		return;
	}
#endif
#if (NCOM > 0)
	if (comcnattach(&isa_io_bs_tag, CONCOMADDR, comcnspeed,
	    COM_FREQ, COM_TYPE_NORMAL, comcnmode))
		panic("can't init serial console @%x", CONCOMADDR);
	return;
#endif
	panic("No serial console configured");
}

static void
integrator_sdram_bounds(paddr_t *memstart, psize_t *memsize)
{
	volatile unsigned long *cm_sdram 
	    = (volatile unsigned long *)0x10000020;
	volatile unsigned long *cm_stat
	    = (volatile unsigned long *)0x10000010;

	*memstart = *cm_stat & 0x00ff0000;

	/*
	 * Although the SSRAM overlaps the SDRAM, we can use the wrap-around
	 * to access the entire bank.
	 */
	switch ((*cm_sdram >> 2) & 0x7)
	{
	case 0:
		*memsize = 16 * 1024 * 1024;
		break;
	case 1:
		*memsize = 32 * 1024 * 1024;
		break;
	case 2:
		*memsize = 64 * 1024 * 1024;
		break;
	case 3:
		*memsize = 128 * 1024 * 1024;
		break;
	case 4:
		/* With 256M of memory there is no wrap-around.  */
		*memsize = 256 * 1024 * 1024 - *memstart;
		break;
	default:
		printf("CM_SDRAM retuns unknown value, using 16M\n");
		*memsize = 16 * 1024 * 1024;
		break;
	}
}
