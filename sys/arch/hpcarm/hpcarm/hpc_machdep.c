/*	$NetBSD: hpc_machdep.c,v 1.28 2002/02/20 00:10:19 thorpej Exp $	*/

/*
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
 *      This product includes software developed by Brini.
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
 * This file needs a lot of work. 
 *
 * Created      : 17/09/94
 */
/*
 * hpc_machdep.c 
 */

#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/exec.h>

#include <dev/cons.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <uvm/uvm.h>

#include <machine/signal.h>
#include <machine/frame.h>
#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/io.h>
#include <machine/intr.h>
#include <arm/arm32/katelib.h>
#include <machine/bootinfo.h>
#include <arm/undefined.h>
#include <machine/rtc.h>
#include <machine/platid.h>
#include <hpcarm/sa11x0/sa11x0_reg.h>

#include <dev/hpc/bicons.h>

#include "opt_ipkdb.h"

/* XXX for consinit related hacks */
#include <sys/conf.h>

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

BootConfig bootconfig;		/* Boot config storage */
struct bootinfo *bootinfo, bootinfo_storage;
static char booted_kernel_storage[80];
char *booted_kernel = booted_kernel_storage;

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
pv_addr_t kernelstack;

char *boot_args = NULL;
char *boot_file = NULL;

vaddr_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;
extern int end;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif	/* PMAP_DEBUG */

#define	KERNEL_PT_VMEM		0	/* Page table for mapping video memory */
#define	KERNEL_PT_SYS		1	/* Page table for mapping proc0 zero page */
#define	KERNEL_PT_KERNEL	2	/* Page table for mapping kernel */
#define	KERNEL_PT_IO		3	/* Page table for mapping IO */
#define	KERNEL_PT_VMDATA	4	/* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	(KERNEL_VM_SIZE >> (PDSHIFT + 2))
#define	NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pt_entry_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

#ifdef CPU_SA110
#define CPU_SA110_CACHE_CLEAN_SIZE (0x4000 * 2)
extern unsigned int sa110_cache_clean_addr;
extern unsigned int sa110_cache_clean_size;
static vaddr_t sa110_cc_base;
#endif	/* CPU_SA110 */

/* Non-buffered non-cachable memory needed to enter idle mode */
extern vaddr_t sa11x0_idle_mem;

/* Prototypes */

void physcon_display_base	__P((u_int addr));
void consinit		__P((void));

void map_pagetable	__P((vaddr_t pt, vaddr_t va, vaddr_t pa));
void map_entry		__P((vaddr_t pt, vaddr_t va, vaddr_t pa));
void map_entry_nc	__P((vaddr_t pt, vaddr_t va, vaddr_t pa));
void map_entry_ro	__P((vaddr_t pt, vaddr_t va, vaddr_t pa));
vm_size_t map_chunk	__P((vaddr_t pd, vaddr_t pt, vaddr_t va,
			     vaddr_t pa, vm_size_t size, u_int acc,
			     u_int flg));

void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void undefinedinstruction_bounce	__P((trapframe_t *frame));

u_int cpu_get_control		__P((void));

void rpc_sa110_cc_setup(void);

#ifdef DEBUG_BEFOREMMU
static void fakecninit();
#endif

#ifdef BOOT_DUMP
void dumppages(char *, int);
#endif

extern int db_trapper();

extern void dump_spl_masks	__P((void));
extern pt_entry_t *pmap_pte	__P((pmap_t pmap, vaddr_t va));

extern void dumpsys	__P((void));

/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
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
	 * Note: Unless cold is set to 1 here, syslogd will die during the unmount.
	 * It looks like syslogd is getting woken up only to find that it cannot
	 * page part of the binary in as the filesystem has been unmounted.
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
	/*NOTREACHED*/
}

/*
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
initarm(argc, argv, bi)
	int argc;
	char **argv;
	struct bootinfo *bi;
{
	int loop;
	u_int kerneldatasize, symbolsize;
	u_int l1pagetable;
	u_int l2pagetable;
	vaddr_t freemempos;
	extern char page0[], page0_end[];
	pv_addr_t kernel_l1pt;
	pv_addr_t kernel_ptpt;
#ifdef DDB
	Elf_Shdr *sh;
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	set_cpufuncs();

#ifdef DEBUG_BEFOREMMU
	/*
	 * At this point, we cannot call real consinit().
	 * Just call a faked up version of consinit(), which does the thing
	 * with MMU disabled.
	 */
	fakecninit();
#endif

	/*
	 * XXX for now, overwrite bootconfig to hardcoded values.
	 * XXX kill bootconfig and directly call uvm_physload
	 */
	bootconfig.dram[0].address = 0xc0000000;
	bootconfig.dram[0].pages = 8192;
	bootconfig.dramblocks = 1;
	kerneldatasize = (u_int32_t)&end - (u_int32_t)KERNEL_TEXT_BASE;

	symbolsize = 0;
#ifdef DDB
	if (! memcmp(&end, "\177ELF", 4)) {
		sh = (Elf_Shdr *)((char *)&end + ((Elf_Ehdr *)&end)->e_shoff);
		loop = ((Elf_Ehdr *)&end)->e_shnum;
		for(; loop; loop--, sh++)
			if (sh->sh_offset > 0 &&
			    (sh->sh_offset + sh->sh_size) > symbolsize)
				symbolsize = sh->sh_offset + sh->sh_size;
	}
#endif

	printf("kernsize=0x%x\n", kerneldatasize);
	kerneldatasize += symbolsize;
	kerneldatasize = ((kerneldatasize - 1) & ~(NBPG * 4 - 1)) + NBPG * 8;

	/* parse kernel args */
	strncpy(booted_kernel_storage, *argv, sizeof(booted_kernel_storage));
	for(argc--, argv++; argc; argc--, argv++)
		switch(**argv) {
		case 'a':
			boothowto |= RB_ASKNAME;
			break;
		case 's':
			boothowto |= RB_SINGLE;
			break;
		default:
			break;
		}
		
	/* copy bootinfo into known kernel space */
	bootinfo_storage = *bi;
	bootinfo = &bootinfo_storage;

#ifdef BOOTINFO_FB_WIDTH
	bootinfo->fb_line_bytes = BOOTINFO_FB_LINE_BYTES;
	bootinfo->fb_width = BOOTINFO_FB_WIDTH;
	bootinfo->fb_height = BOOTINFO_FB_HEIGHT;
	bootinfo->fb_type = BOOTINFO_FB_TYPE;
#endif

	/*
	 * hpcboot has loaded me with MMU disabled.
	 * So create kernel page tables and enable MMU
	 */

	/*
	 * Set up the variables that define the availablilty of physcial
	 * memory
	 */
	physical_start = bootconfig.dram[0].address;
	physical_freestart = physical_start
	    + (KERNEL_TEXT_BASE - KERNEL_SPACE_START) + kerneldatasize;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address
	    + bootconfig.dram[bootconfig.dramblocks - 1].pages * NBPG;
	physical_freeend = physical_end;
/*	free_pages = bootconfig.drampages;*/
    
	for (loop = 0; loop < bootconfig.dramblocks; ++loop)
		physmem += bootconfig.dram[loop].pages;
    
	/* XXX handle UMA framebuffer memory */

	/* Use the first 1MB to allocate things */
	freemempos = 0xc0000000;
	memset((void *)0xc0000000, 0, KERNEL_TEXT_BASE - 0xc0000000);

	/*
	 * Right We have the bottom meg of memory mapped to 0x00000000
	 * so was can get at it. The kernel will ocupy the start of it.
	 * After the kernel/args we allocate some of the fixed page tables
	 * we need to get the system going.
	 * We allocate one page directory and 8 page tables and store the
	 * physical addresses in the kernel_pt_table array.	
	 * Must remember that neither the page L1 or L2 page tables are the
	 * same size as a page !
	 *
	 * Ok the next bit of physical allocate may look complex but it is
	 * simple really. I have done it like this so that no memory gets
	 * wasted during the allocate of various pages and tables that are
	 * all different sizes.
	 * The start address will be page aligned.
	 * We allocate the kernel page directory on the first free 16KB
	 * boundry we find.
	 * We allocate the kernel page tables on the first 1KB boundry we find.
	 * We allocate 9 PT's. This means that in the process we
	 * KNOW that we will encounter at least 1 16KB boundry.
	 *
	 * Eventually if the top end of the memory gets used for process L1
	 * page tables the kernel L1 page table may be moved up there.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)			\
	(var).pv_pa = (var).pv_va = freemempos;	\
	freemempos += (np) * NBPG;
#define	alloc_pages(var, np)			\
	(var) = freemempos;			\
	freemempos += (np) * NBPG;


	valloc_pages(kernel_l1pt, PD_SIZE / NBPG);
	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		alloc_pages(kernel_pt_table[loop], PT_SIZE / NBPG);
	}

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(systempage, 1);

	/* Allocate a page for the page table to map kernel page tables*/
	valloc_pages(kernel_ptpt, PT_SIZE / NBPG);

	/* Allocate stacks for all modes */
	valloc_pages(irqstack, IRQ_STACK_SIZE);
	valloc_pages(abtstack, ABT_STACK_SIZE);
	valloc_pages(undstack, UND_STACK_SIZE);
	valloc_pages(kernelstack, UPAGES);

#ifdef VERBOSE_INIT_ARM
	printf("IRQ stack: p0x%08lx v0x%08lx\n", irqstack.pv_pa, irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n", abtstack.pv_pa, abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n", undstack.pv_pa, undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n", kernelstack.pv_pa, kernelstack.pv_va); 
#endif

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / NBPG);

	/*
	 * XXX Actually, we only need virtual space and don't need
	 * XXX physical memory for sa110_cc_base and sa11x0_idle_mem.
	 */
#ifdef CPU_SA110
	/*
	 * XXX totally stuffed hack to work round problems introduced
	 * in recent versions of the pmap code. Due to the calls used there
	 * we cannot allocate virtual memory during bootstrap.
	 */
	for(;;) {
		alloc_pages(sa110_cc_base, 1);
		if (! (sa110_cc_base & (CPU_SA110_CACHE_CLEAN_SIZE - 1)))
			break;
	}
	{
		vaddr_t dummy;
		alloc_pages(dummy, CPU_SA110_CACHE_CLEAN_SIZE / NBPG - 1);
	}
	sa110_cache_clean_addr = sa110_cc_base;
	sa110_cache_clean_size = CPU_SA110_CACHE_CLEAN_SIZE / 2;
#endif	/* CPU_SA110 */

	alloc_pages(sa11x0_idle_mem, 1);

	/*
	 * Ok we have allocated physical pages for the primary kernel
	 * page tables
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table\n");
#endif

	/*
	 * Now we start consturction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	map_pagetable(l1pagetable, 0x00000000,
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_pagetable(l1pagetable, KERNEL_SPACE_START,
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		map_pagetable(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	map_pagetable(l1pagetable, PROCESS_PAGE_TBLS_BASE,
	    kernel_ptpt.pv_pa);
#define SAIPIO_BASE		0xd0000000		/* XXX XXX */
	map_pagetable(l1pagetable, SAIPIO_BASE,
	    kernel_pt_table[KERNEL_PT_IO]);


#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel code/data */
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL];

	/*
	 * XXX there is no ELF header to find RO region.
	 * XXX What should we do?
	 */
#if 0
	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
		logical = map_chunk(l1pagetable, l2pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    AP_KR, PT_CACHEABLE);
		logical += map_chunk(l1pagetable, l2pagetable,
		    KERNEL_TEXT_BASE + logical, physical_start + logical,
		    kerneldatasize - kernexec->a_text, AP_KRW, PT_CACHEABLE);
	} else
#endif
		map_chunk(l1pagetable, l2pagetable, KERNEL_TEXT_BASE,
		    KERNEL_TEXT_BASE, kerneldatasize,
		    AP_KRW, PT_CACHEABLE);

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the stack pages */
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL];
	map_chunk(0, l2pagetable, irqstack.pv_va, irqstack.pv_pa,
	    IRQ_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, abtstack.pv_va, abtstack.pv_pa,
	    ABT_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, undstack.pv_va, undstack.pv_pa,
	    UND_STACK_SIZE * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, kernelstack.pv_va, kernelstack.pv_pa,
	    UPAGES * NBPG, AP_KRW, PT_CACHEABLE);
	map_chunk(0, l2pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    PD_SIZE, AP_KRW, 0);

	/* Map the page table that maps the kernel pages */
	map_entry_nc(l2pagetable, kernel_ptpt.pv_pa, kernel_ptpt.pv_pa);

	/* Map a page for entering idle mode */
	map_entry_nc(l2pagetable, sa11x0_idle_mem, sa11x0_idle_mem);

	/*
	 * Map entries in the page table used to map PTE's
	 * Basically every kernel page table gets mapped here
	 */
	/* The -2 is slightly bogus, it should be -log2(sizeof(pt_entry_t)) */
	l2pagetable = kernel_ptpt.pv_pa;
	map_entry_nc(l2pagetable, (0x00000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_SYS]);
	map_entry_nc(l2pagetable, (KERNEL_SPACE_START >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_entry_nc(l2pagetable, (KERNEL_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop) {
		map_entry_nc(l2pagetable, ((KERNEL_VM_BASE +
		    (loop * 0x00400000)) >> (PGSHIFT-2)),
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	}
	map_entry_nc(l2pagetable, (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT-2)),
	    kernel_ptpt.pv_pa);
	map_entry_nc(l2pagetable, (SAIPIO_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_IO]);

	/*
	 * Map the system page in the kernel page table for the bottom 1Meg
	 * of the virtual memory map.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_SYS];
	map_entry(l2pagetable, 0x0000000, systempage.pv_pa);

	/* Map any I/O modules here, as we don't have real bus_space_map() */
	printf("mapping IO...");
	l2pagetable = kernel_pt_table[KERNEL_PT_IO];
	map_entry_nc(l2pagetable, SACOM3_BASE, SACOM3_HW_BASE);

#ifdef CPU_SA110
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL];
	map_chunk(0, l2pagetable, sa110_cache_clean_addr,
	    0xe0000000, CPU_SA110_CACHE_CLEAN_SIZE,
	    AP_KRW, PT_CACHEABLE);
#endif
	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	printf("done.\n");

	/* Right set up the vectors at the bottom of page 0 */
	memcpy((char *)systempage.pv_va, page0, page0_end - page0);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
	printf("init subsystems: stacks ");

	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + IRQ_STACK_SIZE * NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + ABT_STACK_SIZE * NBPG);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + UND_STACK_SIZE * NBPG);
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
	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
	printf("%08x %08x %08x\n", data_abort_handler_address,
	    prefetch_abort_handler_address, undefined_handler_address); 

	/* Initialise the undefined instruction handlers */
	printf("undefined ");
	undefined_init();

	/* Set the page table address. */
	setttb(kernel_l1pt.pv_pa);

#ifdef BOOT_DUMP
	dumppages((char *)0xc0000000, 16 * NBPG);
	dumppages((char *)0xb0100000, 64); /* XXX */
#endif
	/* Enable MMU, I-cache, D-cache, write buffer. */
	cpufunc_control(0x337f, 0x107d);

	consinit();

#ifdef VERBOSE_INIT_ARM
	printf("freemempos=%08lx\n", freemempos);
	printf("MMU enabled. control=%08x\n", cpu_get_control());
#endif

	/* Boot strap pmap telling it where the kernel page table is */
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, kernel_ptpt);


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

#ifdef BOOT_DUMP
	dumppages((char *)kernel_l1pt.pv_va, 16);
	dumppages((char *)PROCESS_PAGE_TBLS_BASE, 16);
#endif

#ifdef DDB
	{
		static struct undefined_handler uh;

		uh.uh_handler = db_trapper;
		install_coproc_handler_static(0, &uh);
	}
	ddb_init(symbolsize, ((int *)&end), ((char *)&end) + symbolsize);
#endif

	printf("kernsize=0x%x", kerneldatasize);
	printf(" (including 0x%x symbols)\n", symbolsize);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif	/* DDB */

	if (bootinfo->magic == BOOTINFO_MAGIC) {
		platid.dw.dw0 = bootinfo->platid_cpu;
		platid.dw.dw1 = bootinfo->platid_machine;
	}

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
	if (bootinfo->bi_cnuse == BI_CNUSE_SERIAL)
		cninit();
	else {
		/*
		 * Nothing to do here.  Console initialization is done at
		 * autoconf device attach time.
		 */
	}
}

#ifdef DEBUG_BEFOREMMU
cons_decl(sacom);
void
fakecninit()
{
	static struct consdev fakecntab = cons_init(sacom);
	cn_tab = &fakecntab;

	(*cn_tab->cn_init)(0);
	cn_tab->cn_pri = CN_REMOTE;
}
#endif

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
void
rpc_sa110_cc_setup(void)
{
	int loop;
	paddr_t kaddr;
	pt_entry_t *pte;

	(void) pmap_extract(pmap_kernel(), KERNEL_TEXT_BASE, &kaddr);
	for (loop = 0; loop < CPU_SA110_CACHE_CLEAN_SIZE; loop += NBPG) {
		pte = pmap_pte(pmap_kernel(), (sa110_cc_base + loop));
		*pte = L2_PTE(kaddr, AP_KR);
	}
	sa110_cache_clean_addr = sa110_cc_base;
	sa110_cache_clean_size = CPU_SA110_CACHE_CLEAN_SIZE / 2;
}
#endif	/* CPU_SA110 */

#ifdef BOOT_DUMP
void dumppages(char *start, int nbytes)
{
	char *p = start;
	char *p1;
	int i;

	for(i = nbytes; i > 0; i -= 16, p += 16) {
		for(p1 = p + 15; p != p1; p1--) {
			if (*p1)
				break;
		}
		if (! *p1)
			continue;
		printf("%08x %02x %02x %02x %02x %02x %02x %02x %02x"
		    " %02x %02x %02x %02x %02x %02x %02x %02x\n",
		    (unsigned int)p,
		    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}
#endif

/* End of machdep.c */
