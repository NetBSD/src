/*	$NetBSD: hpc_machdep.c,v 1.90.2.1 2009/05/13 17:17:45 jym Exp $	*/

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
 */

/*
 * Machine dependent functions for kernel setup.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpc_machdep.c,v 1.90.2.1 2009/05/13 17:17:45 jym Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_pmap_debug.h"
#include "fs_nfs.h"
#include "ksyms.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/exec.h>
#include <sys/ksyms.h>
#include <sys/boot_flag.h>
#include <sys/conf.h>	/* XXX for consinit related hacks */
#include <sys/device.h>

#if NKSYMS || defined(DDB) || defined(MODULAR)
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

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/cpuconf.h>
#include <arm/undefined.h>

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/io.h>
#include <machine/platid.h>
#include <machine/rtc.h>
#include <machine/signal.h>

#include <dev/cons.h>
#include <dev/hpc/apm/apmvar.h>
#include <dev/hpc/bicons.h>

#ifdef NFS
#include <sys/mount.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#endif /* NFS */

/* Kernel text starts 256K in from the bottom of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00040000)
#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x00C00000)
#define	KERNEL_VM_SIZE		0x05000000

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */
u_int cpu_reset_address = 0;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#define UND_STACK_SIZE	1

BootConfig bootconfig;		/* Boot config storage */
struct bootinfo *bootinfo, bootinfo_storage;
static char booted_kernel_storage[80];
char *booted_kernel = booted_kernel_storage;

paddr_t physical_start;
paddr_t physical_freestart;
paddr_t physical_freeend;
paddr_t physical_end;
int physmem = 0;

#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif /* !PMAP_STATIC_L1S */


/* Physical and virtual addresses for some global pages */
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t abtstack;
pv_addr_t kernelstack;

char *boot_args = NULL;
char boot_file[16];

vaddr_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;
extern int end;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif /* PMAP_DEBUG */

#define	KERNEL_PT_VMEM		0	/* Page table for mapping video memory */
#define	KERNEL_PT_SYS		1	/* Page table for mapping proc0 zero page */
#define	KERNEL_PT_IO		2	/* Page table for mapping IO */
#define	KERNEL_PT_KERNEL	3	/* Page table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL+KERNEL_PT_KERNEL_NUM)
				        /* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define	NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

#define CPU_SA110_CACHE_CLEAN_SIZE (0x4000 * 2)
extern unsigned int sa1_cache_clean_addr;
extern unsigned int sa1_cache_clean_size;
static vaddr_t sa1_cc_base;

/* Mode dependent sleep function holder */
void (*__sleep_func)(void *);
void *__sleep_ctx;

/* Non-buffered non-cacheable memory needed to enter idle mode */
extern vaddr_t sa11x0_idle_mem;

/* Prototypes */
void		data_abort_handler(trapframe_t *);
void		prefetch_abort_handler(trapframe_t *);
void		undefinedinstruction_bounce(trapframe_t *);
void		dumpsys(void);
u_int		cpu_get_control(void);

u_int		initarm(int, char **, struct bootinfo *);

#ifdef DEBUG_BEFOREMMU
static void	fakecninit(void);
#endif

#ifdef BOOT_DUMP
static void	dumppages(char *, int);
#endif

/*
 * Reboots the system.
 *
 * Deal with any syncing, unmounting, dumping and shutdown hooks,
 * then reset the CPU.
 */
void
cpu_reboot(int howto, char *bootstr)
{
	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast.
	 */
	if (cold) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		printf("Halted while still in the ICE age.\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		cpu_reset();
		/* NOTREACHED */
	}

	/* Reset the sleep function. */
	__sleep_func = NULL;
	__sleep_ctx = NULL;

	/* Disable console buffering. */
	cnpollc(1);

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during
	 * the unmount.  It looks like syslogd is getting woken up only
	 * to find that it cannot page part of the binary in as the
	 * file system has been unmounted.
	 */
	if (!(howto & RB_NOSYNC))
		bootsync();

	/* Say NO to interrupts. */
	(void)splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	/* Make sure IRQs are disabled. */
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

/* Number of DRAM pages which are installed */
/* Units are 4K pages, so 8192 is 32 MB of memory */
#ifndef DRAM_PAGES
#define DRAM_PAGES	8192
#endif

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel and stay at the same address
 * throughout whole kernel's life time.
 */
static const struct pmap_devmap sa11x0_devmap[] = {
	/* Physical/virtual address for UART #3. */
	{
		SACOM3_VBASE,
		SACOM3_BASE,
		0x24,
		VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE
	},
	{ 0, 0, 0, 0, 0 }
};

/*
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responsible for setting up everything that must be
 * in place when main is called.
 * This includes:
 *   Taking a copy of the boot configuration structure.
 *   Initializing the physical console so characters can be printed.
 *   Setting up page tables for the kernel.
 */
u_int
initarm(int argc, char **argv, struct bootinfo *bi)
{
	u_int kerneldatasize, symbolsize;
	u_int l1pagetable;
	vaddr_t freemempos;
	vsize_t pt_size;
	int loop, i;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	Elf_Shdr *sh;
#endif

	__sleep_func = NULL;
	__sleep_ctx = NULL;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions.
	 */
	set_cpufuncs();
	IRQdisable;

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
	bootconfig.dram[0].pages = DRAM_PAGES;
	bootconfig.dramblocks = 1;
	kerneldatasize = (uint32_t)&end - (uint32_t)KERNEL_TEXT_BASE;

	symbolsize = 0;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (!memcmp(&end, "\177ELF", 4)) {
		sh = (Elf_Shdr *)((char *)&end + ((Elf_Ehdr *)&end)->e_shoff);
		loop = ((Elf_Ehdr *)&end)->e_shnum;
		for (; loop; loop--, sh++)
			if (sh->sh_offset > 0 &&
			    (sh->sh_offset + sh->sh_size) > symbolsize)
				symbolsize = sh->sh_offset + sh->sh_size;
	}
#endif

	printf("kernsize=0x%x\n", kerneldatasize);
	kerneldatasize += symbolsize;
	kerneldatasize = ((kerneldatasize - 1) & ~(PAGE_SIZE * 4 - 1)) +
	    PAGE_SIZE * 8;

	/* parse kernel args */
	boothowto = 0;
	boot_file[0] = '\0';
	strncpy(booted_kernel_storage, argv[0], sizeof(booted_kernel_storage));
	for (i = 1; i < argc; i++) {
		char *cp = argv[i];

		switch (*cp) {
		case 'b':
			/* boot device: -b=sd0 etc. */
			cp = cp + 2;
#ifdef NFS
			if (strcmp(cp, MOUNT_NFS) == 0)
				rootfstype = MOUNT_NFS;
			else
				strncpy(boot_file, cp, sizeof(boot_file));
#else /* !NFS */
			strncpy(boot_file, cp, sizeof(boot_file));
#endif /* !NFS */
			break;
		default:
			BOOT_FLAG(*cp, boothowto);
			break;
		}
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
	 * So create kernel page tables and enable MMU.
	 */

	/*
	 * Set up the variables that define the availability of physcial
	 * memory.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_freestart = physical_start
	    + (KERNEL_TEXT_BASE - KERNEL_BASE) + kerneldatasize;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address
	    + bootconfig.dram[bootconfig.dramblocks - 1].pages * PAGE_SIZE;
	physical_freeend = physical_end;
    
	for (loop = 0; loop < bootconfig.dramblocks; ++loop)
		physmem += bootconfig.dram[loop].pages;
    
	/* XXX handle UMA framebuffer memory */

	/* Use the first 256kB to allocate things */
	freemempos = KERNEL_BASE;
	memset((void *)KERNEL_BASE, 0, KERNEL_TEXT_BASE - KERNEL_BASE);

	/*
	 * Right. We have the bottom meg of memory mapped to 0x00000000
	 * so was can get at it. The kernel will occupy the start of it.
	 * After the kernel/args we allocate some of the fixed page tables
	 * we need to get the system going.
	 * We allocate one page directory and NUM_KERNEL_PTS page tables
	 * and store the physical addresses in the kernel_pt_table array.	
	 * Must remember that neither the page L1 or L2 page tables are the
	 * same size as a page !
	 *
	 * Ok, the next bit of physical allocate may look complex but it is
	 * simple really. I have done it like this so that no memory gets
	 * wasted during the allocate of various pages and tables that are
	 * all different sizes.
	 * The start address will be page aligned.
	 * We allocate the kernel page directory on the first free 16KB
	 * boundary we find.
	 * We allocate the kernel page tables on the first 1KB boundary we 
	 * find.  We allocate at least 9 PT's (12 currently).  This means
	 * that in the process we KNOW that we will encounter at least one
	 * 16KB boundary.
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
	freemempos += (np) * PAGE_SIZE;
#define	alloc_pages(var, np)			\
	(var) = freemempos;			\
	freemempos += (np) * PAGE_SIZE;


	valloc_pages(kernel_l1pt, L1_TABLE_SIZE / PAGE_SIZE);
	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		alloc_pages(kernel_pt_table[loop].pv_pa,
		    L2_TABLE_SIZE / PAGE_SIZE);
		kernel_pt_table[loop].pv_va = kernel_pt_table[loop].pv_pa;
	}

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	valloc_pages(systempage, 1);

	pt_size = round_page(freemempos) - KERNEL_BASE;

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
	 * XXX Actually, we only need virtual space and don't need
	 * XXX physical memory for sa110_cc_base and sa11x0_idle_mem.
	 */
	/*
	 * XXX totally stuffed hack to work round problems introduced
	 * in recent versions of the pmap code. Due to the calls used there
	 * we cannot allocate virtual memory during bootstrap.
	 */
	for (;;) {
		alloc_pages(sa1_cc_base, 1);
		if (!(sa1_cc_base & (CPU_SA110_CACHE_CLEAN_SIZE - 1)))
			break;
	}
	{
		vaddr_t dummy;
		alloc_pages(dummy, CPU_SA110_CACHE_CLEAN_SIZE / PAGE_SIZE - 1);
	}
	sa1_cache_clean_addr = sa1_cc_base;
	sa1_cache_clean_size = CPU_SA110_CACHE_CLEAN_SIZE / 2;

	alloc_pages(sa11x0_idle_mem, 1);

	/*
	 * Ok, we have allocated physical pages for the primary kernel
	 * page tables.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Creating L1 page table\n");
#endif

	/*
	 * Now we start construction of the L1 page table.
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary.
	 */
	l1pagetable = kernel_l1pt.pv_pa;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, 0x00000000,
	    &kernel_pt_table[KERNEL_PT_SYS]);
#define SAIPIO_BASE		0xd0000000		/* XXX XXX */
	pmap_link_l2pt(l1pagetable, SAIPIO_BASE,
	    &kernel_pt_table[KERNEL_PT_IO]);
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

	/* Now we fill in the L2 pagetable for the kernel code/data */

	/*
	 * XXX there is no ELF header to find RO region.
	 * XXX What should we do?
	 */
#if 0
	if (N_GETMAGIC(kernexec[0]) == ZMAGIC) {
		logical = pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    VM_PROT_READ, PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable,
		    KERNEL_TEXT_BASE + logical, physical_start + logical,
		    kerneldatasize - kernexec->a_text,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	} else
#endif
		pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
		    KERNEL_TEXT_BASE, kerneldatasize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

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

	/* Map page tables */
	pmap_map_chunk(l1pagetable, KERNEL_BASE, KERNEL_BASE, pt_size,
	    VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	/* Map a page for entering idle mode */
	pmap_map_entry(l1pagetable, sa11x0_idle_mem, sa11x0_idle_mem,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/* Map the statically mapped devices. */
	pmap_devmap_bootstrap(l1pagetable, sa11x0_devmap);

	pmap_map_chunk(l1pagetable, sa1_cache_clean_addr, 0xe0000000,
	    CPU_SA110_CACHE_CLEAN_SIZE, VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	printf("done.\n");

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
	printf("init subsystems: stacks ");

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
#endif /* PMAP_DEBUG */

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler. Until then we will use a handler that just panics but
	 * tells us why.
	 * Initialization of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
	printf("%08x %08x %08x\n", data_abort_handler_address,
	    prefetch_abort_handler_address, undefined_handler_address); 

	/* Initialize the undefined instruction handlers */
	printf("undefined ");
	undefined_init();

	/* Set the page table address. */
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	setttb(kernel_l1pt.pv_pa);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init.
	 */
	proc0paddr = (struct user *)kernelstack.pv_va;
	lwp0.l_addr = proc0paddr;

#ifdef BOOT_DUMP
	dumppages((char *)0xc0000000, 16 * PAGE_SIZE);
	dumppages((char *)0xb0100000, 64); /* XXX */
#endif
	/* Enable MMU, I-cache, D-cache, write buffer. */
	cpufunc_control(0x337f, 0x107d);

	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL);

	consinit();

#ifdef VERBOSE_INIT_ARM
	printf("freemempos=%08lx\n", freemempos);
	printf("MMU enabled. control=%08x\n", cpu_get_control());
#endif

	/* Load memory into UVM. */
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	for (loop = 0; loop < bootconfig.dramblocks; loop++) {
		paddr_t dblk_start = (paddr_t)bootconfig.dram[loop].address;
		paddr_t dblk_end = dblk_start
			+ (bootconfig.dram[loop].pages * PAGE_SIZE);

		if (dblk_start < physical_freestart)
			dblk_start = physical_freestart;
		if (dblk_end > physical_freeend)
			dblk_end = physical_freeend;

		uvm_page_physload(atop(dblk_start), atop(dblk_end),
		    atop(dblk_start), atop(dblk_end), VM_FREELIST_DEFAULT);
	}

	/* Boot strap pmap telling it where the kernel page table is */
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

#ifdef BOOT_DUMP
	dumppages((char *)kernel_l1pt.pv_va, 16);
#endif

#ifdef DDB
	db_machine_init();
#endif
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf(symbolsize, ((int *)&end), ((char *)&end) + symbolsize);
#endif

	printf("kernsize=0x%x", kerneldatasize);
	printf(" (including 0x%x symbols)\n", symbolsize);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif /* DDB */

	if (bootinfo->magic == BOOTINFO_MAGIC) {
		platid.dw.dw0 = bootinfo->platid_cpu;
		platid.dw.dw1 = bootinfo->platid_machine;
	}

	/* We return the new stack pointer address */
	return (kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
machine_sleep(void)
{

	if (__sleep_func != NULL)
		__sleep_func(__sleep_ctx);
}

void
machine_standby(void)
{

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

static void
fakecninit(void)
{
	static struct consdev fakecntab = cons_init(sacom);
	cn_tab = &fakecntab;

	(*cn_tab->cn_init)(0);
	cn_tab->cn_pri = CN_REMOTE;
}
#endif

#ifdef BOOT_DUMP
static void
dumppages(char *start, int nbytes)
{
	char *p = start;
	char *p1;
	int i;

	for (i = nbytes; i > 0; i -= 16, p += 16) {
		for (p1 = p + 15; p != p1; p1--) {
			if (*p1)
				break;
		}
		if (!*p1)
			continue;
		printf("%08x %02x %02x %02x %02x %02x %02x %02x %02x"
		    " %02x %02x %02x %02x %02x %02x %02x %02x\n",
		    (unsigned int)p,
		    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}
#endif
