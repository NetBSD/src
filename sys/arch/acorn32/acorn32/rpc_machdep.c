/*	$NetBSD: rpc_machdep.c,v 1.55 2003/05/21 22:48:20 thorpej Exp $	*/

/*
 * Copyright (c) 2000-2002 Reinoud Zandijk.
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * machdep.c
 *
 * Machine dependant functions for kernel setup
 *
 * This file still needs a lot of work
 *
 * Created      : 17/09/94
 * Updated for yet another new bootloader 28/12/02
 */

#include "opt_ddb.h"
#include "opt_pmap_debug.h"
#include "vidcvideo.h"
#include "rpckbd.h"
#include "pckbc.h"
#include "podulebus.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: rpc_machdep.c,v 1.55 2003/05/21 22:48:20 thorpej Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/exec.h>
#include <sys/ksyms.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <uvm/uvm.h>

#include <machine/signal.h>
#include <machine/frame.h>
#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/io.h>
#include <machine/intr.h>
#include <arm/cpuconf.h>
#include <arm/arm32/katelib.h>
#include <arm/arm32/machdep.h>
#include <machine/vconsole.h>
#include <arm/undefined.h>
#include <machine/rtc.h>
#include <machine/bus.h>

#include <arm/iomd/vidc.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>

#include <arm/iomd/vidcvideo.h>

#include <sys/device.h>
#include <arm/iomd/rpckbdvar.h>
#include <dev/ic/pckbcvar.h>

#include "opt_ipkdb.h"
#include "ksyms.h"

/* Kernel text starts at the base of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */
u_int cpu_reset_address = 0x0; /* XXX 0x3800000 too for rev0 RiscPC 600 */


#define VERBOSE_INIT_ARM


/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#ifdef IPKDB
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif


struct bootconfig bootconfig;	/* Boot config storage */
videomemory_t videomemory;	/* Video memory descriptor */

char *boot_args = NULL;		/* holds the pre-processed boot arguments */
extern char *booted_kernel;	/* used for ioctl to retrieve booted kernel */

extern int       *vidc_base;
extern u_int32_t  iomd_base;
extern struct bus_space iomd_bs_tag;

paddr_t physical_start;
paddr_t physical_freestart;
paddr_t physical_freeend;
paddr_t physical_end;
paddr_t dma_range_begin;
paddr_t dma_range_end;

u_int free_pages;
int physmem = 0;
paddr_t memoryblock_end;

#ifndef PMAP_STATIC_L1S
int max_processes = 64;		/* Default number */
#endif	/* !PMAP_STATIC_L1S */

u_int videodram_size = 0;	/* Amount of DRAM to reserve for video */

/* Physical and virtual addresses for some global pages */
pv_addr_t systempage;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

paddr_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif	/* PMAP_DEBUG */

#define	KERNEL_PT_VMEM		0 /* Page table for mapping video memory */
#define	KERNEL_PT_SYS		1 /* Page table for mapping proc0 zero page */
#define	KERNEL_PT_KERNEL	2 /* Page table for mapping kernel */
#define	KERNEL_PT_VMDATA	3 /* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	4 /* start with 16MB of KVM */	
#define	NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

#ifdef CPU_SA110
#define CPU_SA110_CACHE_CLEAN_SIZE (0x4000 * 2)
static vaddr_t sa110_cc_base;
#endif	/* CPU_SA110 */

/* Prototypes */
void physcon_display_base(u_int);
extern void consinit(void);

void data_abort_handler(trapframe_t *);
void prefetch_abort_handler(trapframe_t *);
void undefinedinstruction_bounce(trapframe_t *frame);

static void canonicalise_bootconfig(struct bootconfig *, struct bootconfig *);
static void process_kernel_args(void);

extern void dump_spl_masks(void);
extern void vidcrender_reinit(void);
extern int vidcrender_blank(struct vconsole *, int);

void rpc_sa110_cc_setup(void);

extern void parse_mi_bootargs(char *args);
void parse_rpc_bootargs(char *args);

extern void dumpsys(void);


#if NVIDCVIDEO > 0
#	define console_flush()		/* empty */
#else
	extern void console_flush(void);
#endif


#define panic2(a) do {							\
	memset((void *) (videomemory.vidm_vbase), 0x55, 50*1024);	\
	consinit();							\
	panic a;							\
} while (/* CONSTCOND */ 0)

/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */

/* NOTE: These variables will be removed, well some of them */

extern u_int spl_mask;
extern u_int current_mask;

void
cpu_reboot(int howto, char *bootstr)
{

#ifdef DIAGNOSTIC
	printf("boot: howto=%08x curlwp=%p\n", howto, curlwp);

	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_imp=%08x\n",
	    irqmasks[IPL_BIO], irqmasks[IPL_NET], irqmasks[IPL_TTY],
	    irqmasks[IPL_IMP]);
	printf("ipl_audio=%08x ipl_clock=%08x ipl_none=%08x\n",
	    irqmasks[IPL_AUDIO], irqmasks[IPL_CLOCK], irqmasks[IPL_NONE]);

	dump_spl_masks();
#endif	/* DIAGNOSTIC */

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		printf("Halted while still in the ICE age.\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		cpu_reset();
		/*NOTREACHED*/
	}

	/* Disable console buffering */
	cnpollc(1);

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during
	 * the unmount.  It looks like syslogd is getting woken up
	 * only to find that it cannot page part of the binary in as
	 * the filesystem has been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	/*
	 * Auto reboot overload protection
	 *
	 * This code stops the kernel entering an endless loop of reboot
	 * - panic cycles. This will have the effect of stopping further
	 * reboots after it has rebooted 8 times after panics. A clean
	 * halt or reboot will reset the counter.
	 */

	/*
	 * Have we done 8 reboots in a row ? If so halt rather than reboot
	 * since 8 panics in a row without 1 clean halt means something is
	 * seriously wrong.
	 */
	if (cmos_read(RTC_ADDR_REBOOTCNT) > 8)
		howto |= RB_HALT;

	/*
	 * If we are rebooting on a panic then up the reboot count
	 * otherwise reset.
	 * This will thus be reset if the kernel changes the boot action from
	 * reboot to halt due to too any reboots.
	 */
	if (((howto & RB_HALT) == 0) && panicstr)
		cmos_write(RTC_ADDR_REBOOTCNT,
		   cmos_read(RTC_ADDR_REBOOTCNT) + 1);
	else
		cmos_write(RTC_ADDR_REBOOTCNT, 0);

	/*
	 * If we need a RiscBSD reboot, request it buy setting a bit in
	 * the CMOS RAM. This can be detected by the RiscBSD boot loader
	 * during a RISCOS boot. No other way to do this as RISCOS is in ROM.
	 */
	if ((howto & RB_HALT) == 0)
		cmos_write(RTC_ADDR_BOOTOPTS,
		    cmos_read(RTC_ADDR_BOOTOPTS) | 0x02);

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
 * u_int initarm(BootConfig *bootconf)
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

/*
 * this part is completely rewritten for the new bootloader ... It features
 * a flat memory map with a mapping comparable to the EBSA arm32 machine
 * to boost the portability and likeness of the code
 */

/*
 * Mapping table for core kernel memory. This memory is mapped at init
 * time with section mappings.
 * 
 * XXX One big assumption in the current architecture seems that the kernel is
 * XXX supposed to be mapped into bootconfig.dram[0].
 */

#define ONE_MB	0x100000

struct l1_sec_map {
	vaddr_t		va;
	paddr_t		pa;
	vsize_t		size;
	vm_prot_t	prot;
	int		cache;
} l1_sec_table[] = {
	/* Map 1Mb section for VIDC20 */
	{ VIDC_BASE,		VIDC_HW_BASE,
	    ONE_MB,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1Mb section from IOMD */
	{ IOMD_BASE,		IOMD_HW_BASE,
	    ONE_MB,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1Mb of COMBO (and module space) */
	{ IO_BASE,		IO_HW_BASE,
	    ONE_MB,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },
#if NPODULEBUS > 0	/* XXXJRT */
	/* Map the Fast and Sync simple podule space */
	{ SYNC_PODULE_BASE & 0xfff00000, SYNC_PODULE_HW_BASE & 0xfff00000,
	    L1_S_SIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },
	/* Map the EASI podule space */
	{ EASI_BASE,		EASI_HW_BASE,
	    MAX_PODULES * EASI_SIZE,	VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },
#endif
	{ 0, 0, 0, 0, 0 }
};


static void
canonicalise_bootconfig(struct bootconfig *bootconf, struct bootconfig *raw_bootconf)
{
	/* check for bootconfig v2+ structure */
	if (raw_bootconf->magic == BOOTCONFIG_MAGIC) {
		/* v2+ cleaned up structure found */
		*bootconf = *raw_bootconf;
		return;
	} else {
		panic2(("Internal error: no valid bootconfig block found"));
	}
}


u_int
initarm(void *cookie)
{
	struct bootconfig *raw_bootconf = cookie;
	int loop;
	int loop1;
	u_int logical;
	u_int kerneldatasize;
	u_int l1pagetable;
	struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
	pv_addr_t kernel_l1pt;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	set_cpufuncs();

	/* canonicalise the boot configuration structure to alow versioning */
	canonicalise_bootconfig(&bootconfig, raw_bootconf);
	booted_kernel = bootconfig.kernelname;

	/* if the wscons interface is used, switch off VERBOSE booting :( */
#if NVIDCVIDEO>0
#	undef VERBOSE_INIT_ARM
#	undef PMAP_DEBUG
#endif

	/*
	 * Initialise the video memory descriptor
	 *
	 * Note: all references to the video memory virtual/physical address
	 * should go via this structure.
	 */

	/* Hardwire it on the place the bootloader tells us */
	videomemory.vidm_vbase = bootconfig.display_start;
	videomemory.vidm_pbase = bootconfig.display_phys;
	videomemory.vidm_size = bootconfig.display_size;
	if (bootconfig.vram[0].pages) 
		videomemory.vidm_type = VIDEOMEM_TYPE_VRAM;
	else 
		videomemory.vidm_type = VIDEOMEM_TYPE_DRAM;
	vidc_base = (int *) VIDC_HW_BASE;
	iomd_base =         IOMD_HW_BASE;

	/*
	 * Initialise the physical console
	 * This is done in main() but for the moment we do it here so that
	 * we can use printf in initarm() before main() has been called.
	 * only for `vidcconsole!' ... not wscons
	 */
#if NVIDCVIDEO == 0
	consinit();
#endif

	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 * Once all the memory map changes are complete we can call consinit()
	 * and not have to worry about things moving.
	 */
	/* fcomcnattach(DC21285_ARMCSR_BASE, comcnspeed, comcnmode); */
	/* XXX snif .... i am still not able to this */

	/*
	 * We have the following memory map (derived from EBSA)
	 *
	 * virtual address == physical address apart from the areas:
	 * 0x00000000 -> 0x000fffff which is mapped to
	 * top 1MB of physical memory
	 * 0xf0000000 -> 0xf0ffffff wich is mapped to
	 * physical address 0x01000000 -> 0x01ffffff (DRAM0a, dram[0])
	 *
	 * This means that the kernel is mapped suitably for continuing
	 * execution, all I/O is mapped 1:1 virtual to physical and
	 * physical memory is accessible.
	 *
	 * The initarm() has the responsibility for creating the kernel
	 * page tables.
	 * It must also set up various memory pointers that are used
	 * by pmap etc. 
	 */

	/* START OF REAL NEW STUFF */

	/* Check to make sure the page size is correct */
	if (PAGE_SIZE != bootconfig.pagesize)
		panic2(("Page size is %d bytes instead of %d !! (huh?)\n",
			   bootconfig.pagesize, PAGE_SIZE));

	/* process arguments */
	process_kernel_args();


	/*
	 * Now set up the page tables for the kernel ... this part is copied
	 * in a (modified?) way from the EBSA machine port.... 
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif
	/*
	 * Set up the variables that define the availablilty of physical
	 * memory
	 */
	physical_start = 0xffffffff;
	physical_end = 0; 
	for (loop = 0, physmem = 0; loop < bootconfig.dramblocks; ++loop) {
	    	if (bootconfig.dram[loop].address < physical_start)
			physical_start = bootconfig.dram[loop].address;
		memoryblock_end = bootconfig.dram[loop].address +
		    bootconfig.dram[loop].pages * PAGE_SIZE;
		if (memoryblock_end > physical_end)
			physical_end = memoryblock_end;
		physmem += bootconfig.dram[loop].pages;
	};
	/* constants for now, but might be changed/configured */
	dma_range_begin = (paddr_t) physical_start;
	dma_range_end   = (paddr_t) MIN(physical_end, 512*1024*1024);
	/* XXX HACK HACK XXX */
	/* dma_range_end   = 0x18000000; */

	if (physical_start !=  bootconfig.dram[0].address) {
		int oldblocks = 0;

		/* 
		 * must be a kinetic, as it's the only thing to shuffle memory
		 * around
		 */
		/* hack hack - throw away the slow dram */
		for (loop = 0; loop < bootconfig.dramblocks; ++loop) {
			if (bootconfig.dram[loop].address <
			    bootconfig.dram[0].address)	{
				/* non kinetic ram */
				bootconfig.dram[loop].address = 0;
				physmem -= bootconfig.dram[loop].pages;
				bootconfig.drampages -=
				    bootconfig.dram[loop].pages;
				bootconfig.dram[loop].pages = 0;
				oldblocks++;
			}
		}
		physical_start = bootconfig.dram[0].address;
		bootconfig.dramblocks -= oldblocks; 
	}
      
	physical_freestart = physical_start;
	free_pages = bootconfig.drampages;
	physical_freeend = physical_end;
 

	/*
	 * AHUM !! set this variable ... it was set up in the old 1st
	 * stage bootloader
	 */
	kerneldatasize = bootconfig.kernsize + bootconfig.MDFsize;

	/* Update the address of the first free page of physical memory */
	/* XXX Assumption that the kernel and stuff is at the LOWEST physical memory address? XXX */
	physical_freestart +=
	    bootconfig.kernsize + bootconfig.MDFsize + bootconfig.scratchsize;
	free_pages -= (physical_freestart - physical_start) / PAGE_SIZE;
  
	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)						\
	alloc_pages((var).pv_pa, (np));					\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)						\
	(var) = physical_freestart;					\
	physical_freestart += ((np) * PAGE_SIZE);			\
	free_pages -= (np);						\
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


#ifdef DIAGNOSTIC
	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE-1)) != 0)
		panic2(("initarm: Failed to align the kernel page "
		    "directory\n"));
#endif

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
	printf("Setting up stacks :\n");
	printf("IRQ stack: p0x%08lx v0x%08lx\n",
	    irqstack.pv_pa, irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n",
	    abtstack.pv_pa, abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n",
	    undstack.pv_pa, undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n",
	    kernelstack.pv_pa, kernelstack.pv_va); 
	printf("\n");
#endif

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

#ifdef CPU_SA110
	/*
	 * XXX totally stuffed hack to work round problems introduced
	 * in recent versions of the pmap code. Due to the calls used there
	 * we cannot allocate virtual memory during bootstrap.
	 */
	sa110_cc_base = (KERNEL_BASE + (physical_freestart - physical_start)
	    + (CPU_SA110_CACHE_CLEAN_SIZE - 1))
	    & ~(CPU_SA110_CACHE_CLEAN_SIZE - 1);
#endif	/* CPU_SA110 */

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table\n");
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
	pmap_link_l2pt(l1pagetable, KERNEL_BASE,
	    &kernel_pt_table[KERNEL_PT_KERNEL]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		pmap_link_l2pt(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    &kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	pmap_link_l2pt(l1pagetable, VMEM_VBASE,
	    &kernel_pt_table[KERNEL_PT_VMEM]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel code/data */
	/* XXX Kernel doesn't have to be on physical_start (!) use bootconfig XXX */
	/*
	 * The defines are a workaround for a recent problem that occurred
	 * with ARM 610 processors and some ARM 710 processors
	 * Other ARM 710 and StrongARM processors don't have a problem.
	 */
	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
#if defined(CPU_ARM6) || defined(CPU_ARM7)
		logical = pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
#else	/* CPU_ARM6 || CPU_ARM7 */
		logical = pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    VM_PROT_READ, PTE_CACHE);
#endif	/* CPU_ARM6 || CPU_ARM7 */
		logical += pmap_map_chunk(l1pagetable,
		    KERNEL_TEXT_BASE + logical, physical_start + logical,
		    kerneldatasize - kernexec->a_text,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	} else {	/* !ZMAGIC */
		/*
		 * Most likely an ELF kernel ...
		 * XXX no distinction yet between read only and
		 * read/write area's ...
		 */
		pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    physical_start, kerneldatasize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	};


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

	/* Now we fill in the L2 pagetable for the VRAM */
	/*
	 * Current architectures mean that the VRAM is always in 1
	 * continuous bank.  This means that we can just map the 2 meg
	 * that the VRAM would occupy.  In theory we don't need a page
	 * table for VRAM, we could section map it but we would need
	 * the page tables if DRAM was in use.
	 * XXX please map two adjacent virtual areas to ONE physical
	 * area
	 */
	pmap_map_chunk(l1pagetable, VMEM_VBASE, videomemory.vidm_pbase,
	    videomemory.vidm_size, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	pmap_map_chunk(l1pagetable, VMEM_VBASE + videomemory.vidm_size,
	    videomemory.vidm_pbase, videomemory.vidm_size,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the core memory needed before autoconfig */
	loop = 0;
	while (l1_sec_table[loop].size) {
		vm_size_t sz;

#ifdef VERBOSE_INIT_ARM
		printf("%08lx -> %08lx @ %08lx\n", l1_sec_table[loop].pa,
			l1_sec_table[loop].pa + l1_sec_table[loop].size - 1,
			l1_sec_table[loop].va);
#endif
		for (sz = 0; sz < l1_sec_table[loop].size; sz += L1_S_SIZE)
			pmap_map_section(l1pagetable,
			    l1_sec_table[loop].va + sz,
			    l1_sec_table[loop].pa + sz,
			    l1_sec_table[loop].prot,
			    l1_sec_table[loop].cache);
		++loop;
	}

	/*
	 * Now we have the real page tables in place so we can switch
	 * to them.  Once this is done we will be running with the
	 * REAL kernel page tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table\n");
#endif
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	setttb(kernel_l1pt.pv_pa);

	/*
	 * We must now clean the cache again....
	 * Cleaning may be done by reading new data to displace any
	 * dirty data in the cache. This will have happened in setttb()
	 * but since we are boot strapping the addresses used for the read
	 * may have just been remapped and thus the cache could be out
	 * of sync. A re-clean after the switch will cure this.
	 * After booting there are no gross reloations of the kernel thus
	 * this problem will not occur after initarm().
	 */
	cpu_idcache_wbinv_all();
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	proc0paddr = (struct user *)kernelstack.pv_va;
	lwp0.l_addr = proc0paddr;

	/* 
	 * if there is support for a serial console ...we should now
	 * reattach it
	 */
	/*      fcomcndetach();*/

	/*
	 * Reflect videomemory relocation in the videomemory structure
	 * and reinit console
	 */
	if (bootconfig.vram[0].pages == 0) {
		videomemory.vidm_vbase   = VMEM_VBASE;
	} else {
		videomemory.vidm_vbase   = VMEM_VBASE;
		bootconfig.display_start = VMEM_VBASE;
	};
	vidc_base = (int *) VIDC_BASE;
	iomd_base =         IOMD_BASE;

#if NVIDCVIDEO == 0
	physcon_display_base(VMEM_VBASE);
	vidcrender_reinit();
#endif

#ifdef VERBOSE_INIT_ARM
	printf("running on the new L1 page table!\n");
	printf("done.\n");
#endif

	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL);

#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif

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
	console_flush();
#endif

	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);
#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("kstack V%08lx P%08lx\n", kernelstack.pv_va,
		    kernelstack.pv_pa);
#endif	/* PMAP_DEBUG */

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler. Until then we will use a handler that just panics but
	 * tells us why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slighly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
	console_flush();


	/*
	 * At last !
	 * We now have the kernel in physical memory from the bottom upwards.
	 * Kernel page tables are physically above this.
	 * The kernel is mapped to 0xf0000000
	 * The kernel data PTs will handle the mapping of 
	 *   0xf1000000-0xf5ffffff (80 Mb)
	 * 2Meg of VRAM is mapped to 0xf7000000
	 * The page tables are mapped to 0xefc00000
	 * The IOMD is mapped to 0xf6000000
	 * The VIDC is mapped to 0xf6100000
	 * The IOMD/VIDC could be pushed up higher but i havent got
	 * sufficient documentation to do so; the addresses are not
	 * parametized yet and hard to read... better fix this before;
	 * its pretty unforgiving.
	 */

	/* Initialise the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();
	console_flush();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	for (loop = 0; loop < bootconfig.dramblocks; loop++) {
		paddr_t start = (paddr_t)bootconfig.dram[loop].address;
		paddr_t end = start + (bootconfig.dram[loop].pages * PAGE_SIZE);

		if (start < physical_freestart)
			start = physical_freestart;
		if (end > physical_freeend)
			end = physical_freeend;

		/* XXX Consider DMA range intersection checking. */

		uvm_page_physload(atop(start), atop(end),
		    atop(start), atop(end), VM_FREELIST_DEFAULT);
	}

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, KERNEL_VM_BASE,
	    KERNEL_VM_BASE + KERNEL_VM_SIZE);
	console_flush();

	/* Setup the IRQ system */
#ifdef VERBOSE_INIT_ARM
	printf("irq ");
#endif
	console_flush();
	irq_init();
#ifdef VERBOSE_INIT_ARM
	printf("done.\n\n");
#endif

#if NVIDCVIDEO>0
	consinit();		/* necessary ? */
#endif

	/* Talk to the user */
	printf("NetBSD/acorn32 booting ... \n");

	/* Tell the user if his boot loader is too old */
	if ((bootconfig.magic < BOOTCONFIG_MAGIC) ||
	    (bootconfig.version != BOOTCONFIG_VERSION)) {
		printf("\nDETECTED AN OLD BOOTLOADER. PLEASE UPGRADE IT\n\n");
		delay(5000000);
	}

	printf("Kernel loaded from file %s\n", bootconfig.kernelname);
	printf("Kernel arg string (@%p) %s\n",
	    bootconfig.args, bootconfig.args);
	printf("\nBoot configuration structure reports the following "
	    "memory\n");

	printf(" DRAM block 0a at %08x size %08x "
	    "DRAM block 0b at %08x size %08x\n\r",
	    bootconfig.dram[0].address,
	    bootconfig.dram[0].pages * bootconfig.pagesize,
	    bootconfig.dram[1].address,
	    bootconfig.dram[1].pages * bootconfig.pagesize);
	printf(" DRAM block 1a at %08x size %08x "
	    "DRAM block 1b at %08x size %08x\n\r",
	    bootconfig.dram[2].address,
	    bootconfig.dram[2].pages * bootconfig.pagesize,
	    bootconfig.dram[3].address,
	    bootconfig.dram[3].pages * bootconfig.pagesize);
	printf(" VRAM block 0  at %08x size %08x\n\r",
	    bootconfig.vram[0].address,
	    bootconfig.vram[0].pages * bootconfig.pagesize);


	if (cmos_read(RTC_ADDR_REBOOTCNT) > 0)
		printf("Warning: REBOOTCNT = %d\n",
		    cmos_read(RTC_ADDR_REBOOTCNT));

#ifdef CPU_SA110
	if (cputype == CPU_ID_SA110)
		rpc_sa110_cc_setup();	
#endif	/* CPU_SA110 */

#ifdef IPKDB
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif	/* NIPKDB */

#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(bootconfig.ksym_end - bootconfig.ksym_start,
		(void *) bootconfig.ksym_start, (void *) bootconfig.ksym_end);
#endif


#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif	/* DDB */

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}


static void
process_kernel_args(void)
{
	char *args;

	/* Ok now we will check the arguments for interesting parameters. */
	args = bootconfig.args;
	boothowto = 0;

	/* Only arguments itself are passed from the new bootloader */
	while (*args == ' ')
		++args;

	boot_args = args;
	parse_mi_bootargs(boot_args);
	parse_rpc_bootargs(boot_args);
}


void
parse_rpc_bootargs(char *args)
{
	int integer;

	if (get_bootconf_option(args, "videodram", BOOTOPT_TYPE_INT,
	    &integer)) {
		videodram_size = integer;
		/* Round to 4K page */
		videodram_size *= 1024;
		videodram_size = round_page(videodram_size);
		if (videodram_size > 1024*1024)
			videodram_size = 1024*1024;
	}

#if 0
	/* XXX this I would rather have in the new bootconfig structure */
	if (get_bootconf_option(args, "kinetic", BOOTOPT_TYPE_BOOLEAN,
	    &integer)) {
		bootconfig.RPC_kinetic_card_support = 1;
	}
#endif
}


#ifdef CPU_SA110

/*
 * For optimal cache cleaning we need two 16K banks of
 * virtual address space that NOTHING else will access
 * and then we alternate the cache cleaning between the
 * two banks.
 * The cache cleaning code requires requires 2 banks aligned
 * on total size boundry so the banks can be alternated by
 * eorring the size bit (assumes the bank size is a power of 2)
 */
extern unsigned int sa1_cache_clean_addr;
extern unsigned int sa1_cache_clean_size;
void
rpc_sa110_cc_setup(void)
{
	int loop;
	paddr_t kaddr;
	pt_entry_t *pte;

	(void) pmap_extract(pmap_kernel(), KERNEL_TEXT_BASE, &kaddr);
	for (loop = 0; loop < CPU_SA110_CACHE_CLEAN_SIZE; loop += PAGE_SIZE) {
		pte = vtopte(sa110_cc_base + loop);
		*pte = L2_S_PROTO | kaddr |
		    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) | pte_l2_s_cache_mode;
		PTE_SYNC(pte);
	}
	sa1_cache_clean_addr = sa110_cc_base;
	sa1_cache_clean_size = CPU_SA110_CACHE_CLEAN_SIZE / 2;
}
#endif	/* CPU_SA110 */

/* End of machdep.c */
