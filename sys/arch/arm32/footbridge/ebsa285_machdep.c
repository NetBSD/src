/*	$NetBSD: ebsa285_machdep.c,v 1.9 1999/12/03 22:48:23 thorpej Exp $	*/

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
 * Machine dependant functions for kernel setup for EBSA285 core architecture
 * using cyclone firmware
 *
 * Created      : 24/11/97
 */

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

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <vm/vm_kern.h>

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/irqhandler.h>
#include <machine/pte.h>
#include <machine/undefined.h>

#include <arm32/footbridge/cyclone_boot.h>
#include <arm32/footbridge/dc21285mem.h>
#include <arm32/footbridge/dc21285reg.h>

#include "ipkdb.h"

#include "isa.h"
#if NISA > 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#define VERBOSE_INIT_ARM

/*
 * Address to call from cpu_reset() to reset the machine.
 * This is machine architecture dependant as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

u_int cpu_reset_address = DC21285_ROM_BASE;

u_int dc21285_fclk = FCLK;

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#if NIPKDB > 0
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif

struct ebsaboot ebsabootinfo;
BootConfig bootconfig;		/* Boot config storage */
static char bootargs[MAX_BOOT_STRING + 1];
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

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define KERNEL_PT_VMDATA	2	/* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	(KERNEL_VM_SIZE >> (PDSHIFT + 2))
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pt_entry_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

void consinit		__P((void));

int fcomcnattach __P((u_int iobase, int rate,tcflag_t cflag));
int fcomcndetach __P((void));

void isa_cats_init __P((u_int iobase, u_int membase));

void map_section	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa,
			     int cacheable));
void map_pagetable	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry		__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_nc	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_ro	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
vm_size_t map_chunk	__P((vm_offset_t pd, vm_offset_t pt, vm_offset_t va,
			     vm_offset_t pa, vm_size_t size, u_int acc,
			     u_int flg));

void pmap_bootstrap		__P((vm_offset_t kernel_l1pt,
    pv_addr_t kernel_ptpt));
void process_kernel_args	__P((char *));
void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void undefinedinstruction_bounce	__P((trapframe_t *frame));
void zero_page_readonly		__P((void));
void zero_page_readwrite	__P((void));
extern void configure		__P((void));
extern pt_entry_t *pmap_pte	__P((pmap_t pmap, vm_offset_t va));
extern void db_machine_init	__P((void));
extern void parse_mi_bootargs	__P((char *args));
extern void dumpsys		__P((void));

/* A load of console goo. */
#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#ifndef CONCOMADDR
#define CONCOMADDR 0x3f8
#endif
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "vga"
#endif

#define CONSPEED B38400
#ifndef CONSPEED
#define CONSPEED B9600	/* TTYDEF_SPEED */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnspeed = CONSPEED;
int comcnmode = CONMODE;


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
		printf("rebooting...\n");
		cpu_reset();
		/*NOTREACHED*/
	}

	/* Disable console buffering */
/*	cnpollc(1);*/

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
 * Mapping table for core kernel memory. This memory is mapped at init
 * time with section mappings.
 */
struct l1_sec_map {
	vm_offset_t	va;
	vm_offset_t	pa;
	vm_size_t	size;
	int		flags;
} l1_sec_table[] = {
	/* Map 1MB for CSR space */
	{ DC21285_ARMCSR_VBASE,			DC21285_ARMCSR_BASE,
	    DC21285_ARMCSR_VSIZE,		0 },
	/* Map 1MB for fast cache cleaning space */
	{ DC21285_CACHE_FLUSH_VBASE,		DC28285_SA_CACHE_FLUSH_BASE,
	    DC21285_CACHE_FLUSH_VSIZE,		1 },
	/* Map 1MB for PCI IO space */
	{ DC21285_PCI_IO_VBASE,			DC21285_PCI_IO_BASE,
	    DC21285_PCI_IO_VSIZE,		0 },
	/* Map 1MB for PCI IACK space */
	{ DC21285_PCI_IACK_VBASE,		DC21285_PCI_IACK_SPECIAL,
	    DC21285_PCI_IACK_VSIZE,		0 },
	/* Map 16MB of type 1 PCI config access */
	{ DC21285_PCI_TYPE_1_CONFIG_VBASE,	DC21285_PCI_TYPE_1_CONFIG,
	    DC21285_PCI_TYPE_1_CONFIG_VSIZE,	0 },
	/* Map 16MB of type 0 PCI config access */
	{ DC21285_PCI_TYPE_0_CONFIG_VBASE,	DC21285_PCI_TYPE_0_CONFIG,
	    DC21285_PCI_TYPE_0_CONFIG_VSIZE,	0 },
	/* Map 128MB of 32 bit PCI address space for MEM accesses */
	{ DC21285_PCI_MEM_VBASE,		DC21285_PCI_MEM_BASE,
	    DC21285_PCI_MEM_VSIZE,		0 },
	{ 0, 0, 0, 0 }
};

/*
 * u_int initarm(struct ebsaboot *bootinfo)
 *
 * Initial entry point on startup. This gets called before main() is
 * entered.
 * It should be responcible for setting up everything that must be
 * in place when main is called.
 * This includes
 *   Taking a copy of the boot configuration structure.
 *   Initialising the physical console so characters can be printed.
 *   Setting up page tables for the kernel
 *   Relocating the kernel to the bottom of physical memory
 */

u_int
initarm(bootinfo)
	struct ebsaboot *bootinfo;
{
	int loop;
	int loop1;
	u_int logical;
	u_int l1pagetable;
	u_int l2pagetable;
	extern char page0[], page0_end[];
	struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
	pv_addr_t kernel_l1pt;
	pv_addr_t kernel_ptpt;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	set_cpufuncs();

	/* Copy the boot configuration structure */
	ebsabootinfo = *bootinfo;

	if (ebsabootinfo.bt_fclk >= 50000000
	    && ebsabootinfo.bt_fclk <= 66000000)
		dc21285_fclk = ebsabootinfo.bt_fclk;

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independant */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = ebsabootinfo.bt_memstart;
	bootconfig.dram[0].pages = (ebsabootinfo.bt_memend
	    - ebsabootinfo.bt_memstart) / NBPG;

	/*
	 * Initialise the diagnostic serial console
	 * This allows a means of generating output during initarm().
	 * Once all the memory map changes are complete we can call consinit()
	 * and not have to worry about things moving.
	 */
/*	fcomcnattach(DC21285_ARMCSR_BASE, comcnspeed, comcnmode);*/

	/* Talk to the user */
	printf("NetBSD/arm32 booting ...\n");

	if (ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_EBSA
	    && ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_CATS)
		panic("Incompatible magic number passed in boot args\n");

/*	{
	int loop;
	for (loop = 0; loop < 8; ++loop) {
		printf("%08x\n", *(((int *)bootinfo)+loop));
	}
	}*/

	/*
	 * Ok we have the following memory map
	 *
	 * virtual address == physical address apart from the areas:
	 * 0x00000000 -> 0x000fffff which is mapped to
	 * top 1MB of physical memory
	 * 0x00100000 -> 0x0fffffff which is mapped to
	 * physical addresses 0x00100000 -> 0x0fffffff
	 * 0x10000000 -> 0x1fffffff which is mapped to
	 * physical addresses 0x00000000 -> 0x0fffffff
	 * 0x20000000 -> 0xefffffff which is mapped to
	 * physical addresses 0x20000000 -> 0xefffffff
	 * 0xf0000000 -> 0xf03fffff which is mapped to
	 * physical addresses 0x00000000 -> 0x003fffff
	 *
	 * This means that the kernel is mapped suitably for continuing
	 * execution, all I/O is mapped 1:1 virtual to physical and
	 * physical memory is accessable.
	 *
	 * The initarm() has the responcibility for creating the kernel
	 * page tables.
	 * It must also set up various memory pointers that are used
	 * by pmap etc. 
	 */

	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
	process_kernel_args((char *)ebsabootinfo.bt_args);

	printf("initarm: Configuring system ...\n");

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory
	 */
	physical_start = ebsabootinfo.bt_memstart;
	physical_freestart = physical_start;
	physical_end = ebsabootinfo.bt_memend;
	physical_freeend = physical_end;
	free_pages = (physical_end - physical_start) / NBPG;
    
	physmem = (physical_end - physical_start) / NBPG;

	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);

	/*
	 * Ok the kernel occupies the bottom of physical memory.
	 * The first free page after the kernel can be found in
	 * ebsabootinfo->bt_memavail
	 * We now need to allocate some fixed page tables to get the kernel
	 * going.
	 * We allocate one page directory and a number page tables and store
	 * the physical addresses in the kernel_pt_table array.
	 *
	 * Ok the next bit of physical allocation may look complex but it is
	 * simple really. I have done it like this so that no memory gets
	 * wasted during the allocation of various pages and tables that are
	 * all different sizes.
	 * The start addresses will be page aligned.
	 * We allocate the kernel page directory on the first free 16KB boundry
	 * we find.
	 * We allocate the kernel page tables on the first 4KB boundry we find.
	 * Since we allocate at least 3 L2 pagetables we know that we must
	 * encounter at least one 16KB aligned address.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

	/* Update the address of the first free page of physical memory */
	physical_freestart = ebsabootinfo.bt_memavail;
	free_pages -= (physical_freestart - physical_start) / NBPG;

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)			\
	alloc_pages((var).pv_pa, (np));	\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)			\
	(var) = physical_freestart;		\
	physical_freestart += ((np) * NBPG);	\
	free_pages -= (np);			\
	memset((char *)(var), 0, ((np) * NBPG));

	loop1 = 0;
	kernel_l1pt.pv_pa = 0;
	for (loop = 0; loop <= NUM_KERNEL_PTS; ++loop) {
		/* Are we 16KB aligned for an L1 ? */
		if ((physical_freestart & (PD_SIZE - 1)) == 0
		    && kernel_l1pt.pv_pa == 0) {
			valloc_pages(kernel_l1pt, PD_SIZE / NBPG);
		} else {
			alloc_pages(kernel_pt_table[loop1], PT_SIZE / NBPG);
			++loop1;
		}
	}

#ifdef DIAGNOSTIC
	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (PD_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory\n");
#endif

	/*
	 * Allocate a page for the system page mapped to V0x00000000
	 * This page will just contain the system vectors and can be
	 * shared by all processes.
	 */
	alloc_pages(systempage.pv_pa, 1);

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
	map_pagetable(l1pagetable, KERNEL_BASE,
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		map_pagetable(l1pagetable, KERNEL_VM_BASE + loop * 0x00400000,
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);
	map_pagetable(l1pagetable, PROCESS_PAGE_TBLS_BASE,
	    kernel_ptpt.pv_pa);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	l2pagetable = kernel_pt_table[KERNEL_PT_KERNEL];

	if (N_GETMAGIC(kernexec[0]) != ZMAGIC)
		panic("Illegal kernel format\n");
	else {
		extern int end;

		logical = map_chunk(0, l2pagetable, KERNEL_TEXT_BASE,
		    physical_start, kernexec->a_text,
		    AP_KR, PT_CACHEABLE);
		logical += map_chunk(0, l2pagetable, KERNEL_TEXT_BASE + logical,
		    physical_start + logical, kernexec->a_data,
		    AP_KRW, PT_CACHEABLE);
		logical += map_chunk(0, l2pagetable, KERNEL_TEXT_BASE + logical,
		    physical_start + logical, kernexec->a_bss,
		    AP_KRW, PT_CACHEABLE);
		logical += map_chunk(0, l2pagetable, KERNEL_TEXT_BASE + logical,
		    physical_start + logical, kernexec->a_syms + sizeof(int)
		    + *(u_int *)((int)&end + kernexec->a_syms + sizeof(int)),
		    AP_KRW, PT_CACHEABLE);
	}

	/*
	 * PATCH PATCH ...
	 *
	 * Fixup the first word of the kernel to be the instruction
	 * add pc, pc, #0x41000000
	 *
	 * This traps the case where the CPU core resets due to bus contention
	 * on a prototype CATS system and will reboot into the firmware.
	 */
	*((u_int *)KERNEL_TEXT_BASE) = 0xe28ff441;

#ifdef VERBOSE_INIT_ARM
	printf("Constructing L2 page tables\n");
#endif

	/* Map the boot arguments page */
	map_entry_ro(l2pagetable, ebsabootinfo.bt_vargp, ebsabootinfo.bt_pargp);

	/* Map the stack pages */
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

	/*
	 * Map entries in the page table used to map PTE's
	 * Basically every kernel page table gets mapped here
	 */
	/* The -2 is slightly bogus, it should be -log2(sizeof(pt_entry_t)) */
	l2pagetable = kernel_ptpt.pv_pa;
	map_entry_nc(l2pagetable, (KERNEL_BASE >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_KERNEL]);
	map_entry_nc(l2pagetable, (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT-2)),
	    kernel_ptpt.pv_pa);
	map_entry_nc(l2pagetable, (0x00000000 >> (PGSHIFT-2)),
	    kernel_pt_table[KERNEL_PT_SYS]);
	for (loop = 0; loop < KERNEL_PT_VMDATA_NUM; ++loop)
		map_entry_nc(l2pagetable, ((KERNEL_VM_BASE +
		    (loop * 0x00400000)) >> (PGSHIFT-2)),
		    kernel_pt_table[KERNEL_PT_VMDATA + loop]);

	/*
	 * Map the system page in the kernel page table for the bottom 1Meg
	 * of the virtual memory map.
	 */
	l2pagetable = kernel_pt_table[KERNEL_PT_SYS];
	map_entry(l2pagetable, 0x00000000, systempage.pv_pa);

	/* Map the core memory needed before autoconfig */
	loop = 0;
	while (l1_sec_table[loop].size) {
		vm_size_t sz;

#ifdef VERBOSE_INIT_ARM
		printf("%08lx -> %08lx @ %08lx\n", l1_sec_table[loop].pa,
		    l1_sec_table[loop].pa + l1_sec_table[loop].size - 1,
		    l1_sec_table[loop].va);
#endif
		for (sz = 0; sz < l1_sec_table[loop].size; sz += L1_SEC_SIZE)
			map_section(l1pagetable, l1_sec_table[loop].va + sz,
			    l1_sec_table[loop].pa + sz,
			    l1_sec_table[loop].flags);
		++loop;
	}

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("switching to new L1 page table\n");
#endif

	setttb(kernel_l1pt.pv_pa);

	/*
	 * Ok the DC21285 CSR registers have just moved.
	 * Detach the diagnostic serial port and reattach at the new address.
	 */
/*	fcomcndetach();*/

	/*
	 * XXX this should only be done in main() but it useful to
	 * have output earlier ...
	 */
	consinit();

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

	/* Right set up the vectors at the bottom of page 0 */
	memcpy((char *)0x00000000, page0, page0_end - page0);

	/* We have modified a text page so sync the icache */
	cpu_cache_syncI();

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

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slighly better one.
	 */
	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* At last !
	 * We now have the kernel in physical memory from the bottom upwards.
	 * Kernel page tables are physically above this.
	 * The kernel is mapped to KERNEL_TEXT_BASE
	 * The kernel data PTs will handle the mapping of 0xf1000000-0xf3ffffff
	 * The page tables are mapped to 0xefc00000
	 */

	/* Initialise the undefined instruction handlers */
	printf("undefined ");
	undefined_init();

	/* Boot strap pmap telling it where the kernel page table is */
	printf("pmap ");
	pmap_bootstrap(kernel_l1pt.pv_va, kernel_ptpt);

	/* Setup the IRQ system */
	printf("irq ");
	irq_init();
	printf("done.\n");

#if NIPKDB > 0
	/* Initialise ipkdb */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#ifdef DDB
	printf("ddb: ");
	db_machine_init();
	{
		extern int end;
		extern int *esym;

		ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
	}

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
process_kernel_args(args)
	char *args;
{

	boothowto = 0;

	/* Make a local copy of the bootargs */
	strncpy(bootargs, args, MAX_BOOT_STRING);

	args = bootargs;
	boot_file = bootargs;

	/* Skip the kernel image filename */
	while (*args != ' ' && *args != 0)
		++args;

	if (*args != 0)
		*args++ = 0;

	while (*args == ' ')
		++args;

	boot_args = args;

	printf("bootfile: %s\n", boot_file);
	printf("bootargs: %s\n", boot_args);

	parse_mi_bootargs(boot_args);
}

#if 0
void
arm32_cachectl(va, len, flags)
	vm_offset_t va;
	int len;
	int flags;
{
	pt_entry_t *ptep, pte;
	int loop;
	vm_offset_t addr;

/*	printf("arm32_cachectl(%x,%x,%x)\n", va, len, flags);*/

	if (flags & 1) {
		addr = va;
		loop = len;
		while (loop > 0) {
			ptep = vtopte(addr & (~PGOFSET));
			pte = *ptep;
	
			*ptep = (pte & ~(PT_C | PT_B)) | (flags & (PT_C | PT_B));
	
			loop -= NBPG;
			addr += NBPG;
		}
		tlb_flush();
	}
	
	cpu_cache_purgeD_rng(va, len);	
}
#endif

extern struct bus_space footbridge_pci_io_bs_tag;
extern struct bus_space footbridge_pci_mem_bs_tag;
void footbridge_pci_bs_tag_init __P((void));

void
consinit(void)
{
	static int consinit_called = 0;
	char *console = CONSDEVNAME;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NISA > 0
	/* Initialise the ISA subsystem early ... */

	isa_cats_init(DC21285_PCI_IO_VBASE, DC21285_PCI_MEM_VBASE);
#endif

	footbridge_pci_bs_tag_init();

	get_bootconf_option(boot_args, "console", BOOTOPT_TYPE_STRING,
	    &console);

	if (strncmp(console, "fcom", 4) == 0
	    || strncmp(console, "diag", 4) == 0)
		fcomcnattach(DC21285_ARMCSR_VBASE, comcnspeed, comcnmode);
#if (NVGA > 0)
	else if (strncmp(console, "vga", 3) == 0) {
		vga_cnattach(&footbridge_pci_io_bs_tag,
		    &footbridge_pci_mem_bs_tag, - 1, 0);
#if (NPCKBC > 0)
		pckbc_cnattach(&isa_io_bs_tag, IO_KBD, PCKBC_KBD_SLOT);
#endif	/* NPCKBC */
	}
#endif	/* NVGA */
#if (NCOM > 0)
	else if (strncmp(console, "com", 3) == 0) {
		if (comcnattach(&isa_io_bs_tag, CONCOMADDR, comcnspeed,
		    COM_FREQ, comcnmode))
			panic("can't init serial console @%x", CONCOMADDR);
	}
#endif
	/* Don't know what console was requested so use the fall back. */
	else
		fcomcnattach(DC21285_ARMCSR_VBASE, comcnspeed, comcnmode);
}

/* End of ebsa285_machdep.c */
