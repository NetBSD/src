/*	$NetBSD: smdk2800_machdep.c,v 1.16 2003/08/03 14:11:16 bsh Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
 *
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

/*
 * Machine dependant functions for kernel setup for Samsung SMDK2800
 * derived from integrator_machdep.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smdk2800_machdep.c,v 1.16 2003/08/03 14:11:16 bsh Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_pmap_debug.h"
#include "opt_md.h"
#include "pci.h"

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
#include <machine/intr.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/s3c2xx0/s3c2800reg.h>
#include <arm/s3c2xx0/s3c2800var.h>

#include "ksyms.h"

#ifndef	SDRAM_START
#define	SDRAM_START	S3C2800_DBANK0_START
#endif
#ifndef	SDRAM_SIZE
#define	SDRAM_SIZE	(32*1024*1024)
#endif

/*
 * Address to map I/O registers in early initialize stage.
 */
#define	SMDK2800_IO_AREA_VBASE	0xfd000000
#define SMDK2800_VBASE_FREE	0xfd200000

/* Kernel text starts 2MB in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00200000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * The range 0xc1000000 - 0xccffffff is available for kernel VM space
 * Core-logic registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#define KERNEL_VM_SIZE		0x0C000000

/* Memory disk support */
#if defined(MEMORY_DISK_DYNAMIC) && defined(MEMORY_DISK_ROOT_ADDR)
#define DO_MEMORY_DISK
/* We have memory disk image outside of the kernel on ROM. */
#ifdef MEMORY_DISK_ROOT_ROM
/* map the image directory and use read-only */
#else
/* copy the image to RAM */
#endif
#endif


/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */
u_int cpu_reset_address = (u_int)0;

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
int max_processes = 64;		/* Default number */
#endif				/* !PMAP_STATIC_L1S */

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
#define	KERNEL_PT_KERNEL_NUM	2	/* L2 tables for mapping kernel VM */

#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)

#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

void consinit(void);
void kgdb_port_init(void);

static int 
bootstrap_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
    int cacheable, bus_space_handle_t * bshp);
static void map_builtin_peripherals(void);
static void copy_io_area_map(pd_entry_t * new_pd);

/* A load of console goo. */
#include "vga.h"
#if NVGA > 0
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "sscom.h"
#if NSSCOM > 0
#include "opt_sscom.h"
#include <arm/s3c2xx0/sscom_var.h>
#endif

/*
 * Define the default console speed for the board.  This is generally
 * what the firmware provided with the board defaults to.
 */
#ifndef CONSPEED
#define CONSPEED B115200	/* TTYDEF_SPEED */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)   /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

struct bus_space bootstrap_bs_tag;

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

	cpu_reset_address = vtophys((u_int)s3c2800_softreset);

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
		/* NOTREACHED */
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
	/* NOTREACHED */
}
#define ioreg_write8(a,v)  (*(volatile uint8_t *)(a)=(v))

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
	extern int etext asm("_etext");
	extern int end asm("_end");
	pv_addr_t kernel_l1pt;
	struct s3c2800_softc temp_softc;	/* used to initialize IO regs */
	int progress_counter = 0;

#ifdef DO_MEMORY_DISK
	vm_offset_t md_root_start;
#define MD_ROOT_SIZE (MEMORY_DISK_ROOT_SIZE * DEV_BSIZE)
#endif

#define gpio_read8(reg) bus_space_read_1(temp_softc.sc_sx.sc_iot,  \
					 temp_softc.sc_sx.sc_gpio_ioh, (reg))

#define LEDSTEP()  __LED(progress_counter++)

#define pdatc (*(volatile uint8_t *)(S3C2800_GPIO_BASE+GPIO_PDATC))
#define __LED(x)  (pdatc = (pdatc & ~0x07) | (~(x) & 0x07))

	LEDSTEP();
	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	LEDSTEP();

	map_builtin_peripherals();

	/*
	 * prepare fake bus space tag
	 */
	bootstrap_bs_tag = s3c2xx0_bs_tag;
	bootstrap_bs_tag.bs_map = bootstrap_bs_map;
	s3c2xx0_softc = &temp_softc.sc_sx;
	s3c2xx0_softc->sc_iot = &bootstrap_bs_tag;

	bootstrap_bs_map(&bootstrap_bs_tag, S3C2800_GPIO_BASE,
	    S3C2800_GPIO_SIZE, 0, &temp_softc.sc_sx.sc_gpio_ioh);
	bootstrap_bs_map(&bootstrap_bs_tag, S3C2800_INTCTL_BASE,
	    S3C2800_INTCTL_SIZE, 0, &temp_softc.sc_sx.sc_intctl_ioh);
	bootstrap_bs_map(&bootstrap_bs_tag, S3C2800_CLKMAN_BASE,
	    S3C2800_CLKMAN_SIZE, 0, &temp_softc.sc_sx.sc_clkman_ioh);

#undef __LED
#define __LED(x)							\
	bus_space_write_1(&bootstrap_bs_tag,				\
	    temp_softc.sc_sx.sc_gpio_ioh,				\
	    GPIO_PDATC, (~(x) & 0x07) |					\
	    (bus_space_read_1(&bootstrap_bs_tag,			\
		temp_softc.sc_sx.sc_gpio_ioh, GPIO_PDATC ) & ~0x07))

	LEDSTEP();

	/* Disable all peripheral interrupts */
	bus_space_write_4(&bootstrap_bs_tag, temp_softc.sc_sx.sc_intctl_ioh,
	    INTCTL_INTMSK, 0);

	s3c2800_clock_freq(s3c2xx0_softc);

	consinit();
#ifdef VERBOSE_INIT_ARM
	printf("consinit done\n");
#endif

#ifdef KGDB
	LEDSTEP();
	kgdb_port_init();
#endif
	LEDSTEP();

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (SMDK2800) booting ...\n");
#endif

	/*
	 * Ok we have the following memory map
	 *
	 * Physical Address Range     Description
	 * -----------------------    ----------------------------------
	 * 0x00000000 - 0x00ffffff    Intel flash Memory   (16MB)
	 * 0x02000000 - 0x020fffff    AMD flash Memory   (1MB)
	 * or 			       (depend on DIPSW setting)
	 * 0x00000000 - 0x000fffff    AMD flash Memory   (1MB)
	 * 0x02000000 - 0x02ffffff    Intel flash Memory   (16MB)
	 *
	 * 0x08000000 - 0x09ffffff    SDRAM (32MB)
	 * 0x20000000 - 0x3fffffff    PCI space
	 *
	 * The initarm() has the responsibility for creating the kernel
	 * page tables.
	 * It must also set up various memory pointers that are used
	 * by pmap etc.
	 */

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = SDRAM_START;
	bootconfig.dram[0].pages = SDRAM_SIZE / PAGE_SIZE;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0x08200000 (where the kernel
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

#if DO_MEMORY_DISK
#ifdef MEMORY_DISK_ROOT_ROM
	md_root_start = MEMORY_DISK_ROOT_ADDR;
	boothowto |= RB_RDONLY;
#else
	/* Reserve physmem for ram disk */
	md_root_start = ((physical_end - MD_ROOT_SIZE) & ~(L1_S_SIZE-1));
	printf("Reserve %ld bytes for memory disk\n",  
	    physical_end - md_root_start);
	/* copy fs contents */
	memcpy((void *)md_root_start, (void *)MEMORY_DISK_ROOT_ADDR,
	    MD_ROOT_SIZE);
	physical_end = md_root_start;
#endif
#endif

	physical_freestart = 0x08000000UL;	/* XXX */
	physical_freeend = 0x08200000UL;

	physmem = (physical_end - physical_start) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);
#endif

	/*
	 * XXX
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
		panic("initarm: Failed to align the kernel page directory\n");

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

	LEDSTEP();

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
		size_t textsize = (uintptr_t)&etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t)&end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;

		logical = 0x00200000;	/* offset of kernel in RAM */

		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, textsize,
		    VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, totalsize - textsize,
		    VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);
	}

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	pmap_map_chunk(l1pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE,
	    PTE_CACHE);
	pmap_map_chunk(l1pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE,
	    PTE_CACHE);
	pmap_map_chunk(l1pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE,
	    PTE_CACHE);
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
#if 1
	/* MULTI-ICE requires that page 0 is NC/NB so that it can download the
	 * cache-clean code there.  */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ | VM_PROT_WRITE, PTE_NOCACHE);
#else
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);
#endif

#ifdef MEMORY_DISK_DYNAMIC
	/* map MD root image */
	bootstrap_bs_map(&bootstrap_bs_tag, md_root_start, MD_ROOT_SIZE,
			 BUS_SPACE_MAP_CACHEABLE | BUS_SPACE_MAP_LINEAR,
			 (bus_space_handle_t *)&md_root_start);

	md_root_setconf((void *)md_root_start, MD_ROOT_SIZE);
#endif /* MEMORY_DISK_DYNAMIC */
	/*
	 * map integrated peripherals at same address in l1pagetable
	 * so that we can continue to use console.
	 */
	copy_io_area_map((pd_entry_t *)l1pagetable);

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
		physical_freestart = physical_start +
		    (((((uintptr_t)&end) + PGOFSET) & ~PGOFSET) - KERNEL_BASE);
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
	LEDSTEP();
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

#if 0
	/*
	 * The IFPGA registers have just moved.
	 * Detach the diagnostic serial port and reattach at the new address.
	 */
	plcomcndetach();
	/*
	 * XXX this should only be done in main() but it useful to
	 * have output earlier ...
	 */
	consinit();
#endif

	LEDSTEP();
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

	LEDSTEP();

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

	LEDSTEP();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);

	LEDSTEP();
	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, KERNEL_VM_BASE,
	    KERNEL_VM_BASE + KERNEL_VM_SIZE);

	LEDSTEP();

	/* Setup the IRQ system */
#ifdef VERBOSE_INIT_ARM
	printf("irq ");
#endif
	/* XXX irq_init(); */

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef BOOTHOWTO_INIT
	boothowto |= BOOTHOWTO_INIT;
#endif
	{
		uint8_t  gpio = ~gpio_read8(GPIO_PDATF);
		
		if (gpio & (1<<5)) /* SW3 */
			boothowto ^= RB_SINGLE;
		if (gpio & (1<<7)) /* SW7 */
			boothowto ^= RB_KDB;
#ifdef VERBOSE_INIT_ARM
		printf( "sw: %x boothowto: %x\n", gpio, boothowto );
#endif
	}

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
	return (kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
consinit(void)
{
	static int consinit_done = 0;
	bus_space_tag_t iot = s3c2xx0_softc->sc_iot;
	int pclk = s3c2xx0_softc->sc_pclk;

	if (consinit_done != 0)
		return;

	consinit_done = 1;

#if NSSCOM > 0
#ifdef SSCOM0CONSOLE
	if (0 == s3c2800_sscom_cnattach(iot, 0, comcnspeed,
		pclk, comcnmode))
		return;
#endif
#ifdef SSCOM1CONSOLE
	if (0 == s3c2800_sscom_cnattach(iot, 1, comcnspeed,
		pclk, comcnmode))
		return;
#endif
#endif				/* NSSCOM */
#if NCOM>0 && defined(CONCOMADDR)
	if (comcnattach(&isa_io_bs_tag, CONCOMADDR, comcnspeed,
		COM_FREQ, COM_TYPE_NORMAL, comcnmode))
		panic("can't init serial console @%x", CONCOMADDR);
	return;
#endif

	consinit_done = 0;
}


#ifdef KGDB

#if (NSSCOM > 0)

#ifdef KGDB_DEVNAME
const char kgdb_devname[] = KGDB_DEVNAME;
#else
const char kgdb_devname[] = "";
#endif

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE|CSTOPB|PARENB))|CS8) /* 8N1 */
#endif
int kgdb_sscom_mode = KGDB_DEVMODE;

#endif				/* NSSCOM */

void
kgdb_port_init(void)
{
#if (NSSCOM > 0)
	int unit = -1;
	int pclk = s3c2xx0_softc->sc_pclk;

	if (strcmp(kgdb_devname, "sscom0") == 0)
		unit = 0;
	else if (strcmp(kgdb_devname, "sscom1") == 0)
		unit = 1;

	if (unit >= 0) {
		s3c2800_sscom_kgdb_attach(s3c2xx0_softc->sc_iot,
		    unit, kgdb_rate, pclk, kgdb_sscom_mode);
	}
#endif
}
#endif

static __inline
       pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm __volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r"(ttb));


	return (pd_entry_t *)(ttb & ~((1 << 14) - 1));
}


static __inline void
writeback_dcache_line(vaddr_t va)
{
	/* writeback Dcache line */
	/* we can't use cpu_dcache_wb_range() here, because cpufuncs for ARM9
	 * assume write-through cache, and always flush Dcache instead of
	 * cleaning it. Since Boot loader maps page table with write-back
	 * cached, we really need to clean Dcache. */
	asm("mcr	p15, 0, %0, c7, c10, 1"
	    : :	"r"(va));
}

static __inline void
clean_dcache_line(vaddr_t va)
{
	/* writeback and invalidate Dcache line */
	asm("mcr	p15, 0, %0, c7, c14, 1"
	    : : "r"(va));
}

static vaddr_t section_free = SMDK2800_VBASE_FREE;

static void
map_builtin_peripherals(void)
{
	pd_entry_t *pagedir = read_ttb();
	int i, sec;

	for (i=0; i < 2; ++i) {

		pmap_map_section((vaddr_t)pagedir, 
		    SMDK2800_IO_AREA_VBASE + (i <<L1_S_SHIFT),
		    S3C2800_PERIPHERALS + (i << L1_S_SHIFT),
		    VM_PROT_READ | VM_PROT_WRITE, PTE_NOCACHE);

		sec = (SMDK2800_IO_AREA_VBASE >> L1_S_SHIFT) + i;
		writeback_dcache_line((vaddr_t)&pagedir[sec]);
	}

	cpu_drain_writebuf();
	cpu_tlb_flushD();
}

/*
 * simple memory mapping function used in early bootstrap stage
 * before pmap is initialized.
 * This assumes only peripheral registers to map. they are mapped to
 * fixed address with section mapping.
 */
static int
bootstrap_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
    int flag, bus_space_handle_t * bshp)
{
	long offset;
	int modified = 0;
	pd_entry_t *pagedir = read_ttb();
	/* This assumes PA==VA for page directory */

	if (S3C2800_PERIPHERALS <= bpa && bpa < S3C2800_PERIPHERALS + 0x200000) {
		offset = bpa - S3C2800_PERIPHERALS;
		if (offset < 0 || 2 * L1_S_SIZE < offset)
			panic("bootstrap_bs_map: can't map");
		*bshp = (bus_space_handle_t)(SMDK2800_IO_AREA_VBASE + offset);
	} else {
		vaddr_t va;
		bus_addr_t pa;
		int cacheable = flag & BUS_SPACE_MAP_CACHEABLE;


		size = (size + L1_S_OFFSET) & ~L1_S_OFFSET;
		pa = bpa & ~L1_S_OFFSET;
		offset = bpa - pa;

		va = section_free;
		while (size) {
			pmap_map_section((vaddr_t)pagedir, va,
			    pa, VM_PROT_READ | VM_PROT_WRITE,
			    cacheable ? PTE_CACHE : PTE_NOCACHE);
			writeback_dcache_line((vaddr_t)& pagedir[va >> L1_S_SHIFT]);
			va += L1_S_SIZE;
			pa += L1_S_SIZE;
			size -= L1_S_SIZE;
		}

		*bshp = (bus_space_handle_t)(section_free + offset);
		section_free = va;
	}


	if (modified) {

		cpu_drain_writebuf();
		cpu_tlb_flushD();
	}
	return (0);
}

static void
copy_io_area_map(pd_entry_t * new_pd)
{
	pd_entry_t *cur_pd = read_ttb();
	int sec;

	for (sec = SMDK2800_IO_AREA_VBASE >> L1_S_SHIFT;
	    sec < (section_free >> L1_S_SHIFT); ++sec) {
		new_pd[sec] = cur_pd[sec];
	}
}
