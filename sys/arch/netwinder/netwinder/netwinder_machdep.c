/*	$NetBSD: netwinder_machdep.c,v 1.78.2.2 2014/08/20 00:03:15 tls Exp $	*/

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
 * Machine dependent functions for kernel setup for EBSA285 core architecture
 * using Netwinder firmware
 *
 * Created      : 24/11/97
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netwinder_machdep.c,v 1.78.2.2 2014/08/20 00:03:15 tls Exp $");

#include "opt_ddb.h"
#include "opt_pmap_debug.h"

#define	_ARM32_BUS_DMA_PRIVATE

#include "isa.h"
#include "isadma.h"
#include "igsfb.h"
#include "pckbc.h"
#include "com.h"
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
#include <sys/ksyms.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#if NISA > 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#if NIGSFB > 0
#include <dev/pci/pcivar.h>
#include <dev/pci/igsfb_pcivar.h>
#endif

#if NPCKBC > 0
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <arm/locore.h>
#include <arm/undefined.h>

#include <machine/netwinder_boot.h>
#include <arm/footbridge/dc21285mem.h>
#include <arm/footbridge/dc21285reg.h>


static bus_space_handle_t isa_base = (bus_space_handle_t) DC21285_PCI_IO_VBASE;

bs_protos(generic);

#define	ISA_GETBYTE(r)		generic_bs_r_1(0, isa_base, (r))
#define	ISA_PUTBYTE(r,v)	generic_bs_w_1(0, isa_base, (r), (v))

static void netwinder_reset(void);

u_int dc21285_fclk = 63750000;

struct nwbootinfo nwbootinfo;
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

/*int debug_flags;*/
#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

vm_offset_t msgbufphys;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define KERNEL_PT_VMDATA	2	/* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

#define	KERNEL_VM_BASE		(KERNEL_BASE + 0x01000000)
/*
 * The range 0xf1000000 - 0xfcffffff is available for kernel VM space
 * Footbridge registers and I/O mappings occupy 0xfd000000 - 0xffffffff
 */
#if NIGSFB > 0
/* XXX: uwe: map 16 megs at 0xfc000000 for igsfb(4) */
#define KERNEL_VM_SIZE		0x0B000000
#else
#define KERNEL_VM_SIZE		0x0C000000
#endif

/* Prototypes */

void consinit(void);
void process_kernel_args(char *);
void data_abort_handler(trapframe_t *);
void prefetch_abort_handler(trapframe_t *);
void undefinedinstruction_bounce(trapframe_t *);


/* A load of console goo. */
#ifndef CONSDEVNAME
#  if (NIGSFB > 0) && (NPCKBC > 0)
#    define CONSDEVNAME "igsfb"
#  elif NCOM > 0
#    define CONSDEVNAME "com"
#  else
#    error CONSDEVNAME not defined and no known console device configured
#  endif
#endif /* !CONSDEVNAME */

#ifndef CONCOMADDR
#define CONCOMADDR 0x3f8
#endif

#ifndef CONSPEED
#define CONSPEED B115200	/* match NeTTrom */
#endif

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

extern struct consdev kcomcons;
static void kcomcnputc(dev_t, int);

#if NIGSFB > 0
/* XXX: uwe */
#define IGS_PCI_MEM_VBASE		0xfc000000
#define IGS_PCI_MEM_VSIZE		0x01000000
#define IGS_PCI_MEM_BASE		0x08000000

extern struct arm32_pci_chipset footbridge_pci_chipset;
extern struct bus_space footbridge_pci_io_bs_tag;
extern struct bus_space footbridge_pci_mem_bs_tag;
extern void footbridge_pci_bs_tag_init(void);

/* standard methods */
extern bs_map_proto(footbridge_mem);
extern bs_unmap_proto(footbridge_mem);

/* our hooks */
static bs_map_proto(nw_footbridge_mem);
static bs_unmap_proto(nw_footbridge_mem);
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
#ifdef DIAGNOSTIC
	/* info */
	printf("boot: howto=%08x curlwp=%p\n", howto, curlwp);
#endif

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
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
	
	/* Run any shutdown hooks */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

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
 * NB: this function runs with MMU disabled!
 */
static void
netwinder_reset(void)
{
	register u_int base = DC21285_PCI_IO_BASE;

#define PUTBYTE(reg, val) \
	*((volatile u_int8_t *)(base + (reg))) = (val)

	PUTBYTE(0x338, 0x84);	/* Red led(GP17), fan on(GP12) */
	PUTBYTE(0x370, 0x87);	/* Enter the extended function mode */
	PUTBYTE(0x370, 0x87);	/* (need to write the magic twice) */
	PUTBYTE(0x370, 0x07); 	/* Select Logical Device Number reg */
	PUTBYTE(0x371, 0x07);	/* Select Logical Device 7 (GPIO) */
	PUTBYTE(0x370, 0xe6);	/* Select GP16 Control Reg */
	PUTBYTE(0x371, 0x00);	/* Make GP16 an output */
	PUTBYTE(0x338, 0xc4);	/* RESET(GP16), red led, fan on */
}

/*
 * Mapping table for core kernel memory. This memory is mapped at init
 * time with section mappings.
 */
struct l1_sec_map {
	vm_offset_t	va;
	vm_offset_t	pa;
	vm_size_t	size;
	vm_prot_t	prot;
	int		cache;
} l1_sec_table[] = {
	/* Map 1MB for CSR space */
	{ DC21285_ARMCSR_VBASE,			DC21285_ARMCSR_BASE,
	    DC21285_ARMCSR_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1MB for fast cache cleaning space */
	{ DC21285_CACHE_FLUSH_VBASE,		DC21285_SA_CACHE_FLUSH_BASE,
	    DC21285_CACHE_FLUSH_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_CACHE },

	/* Map 1MB for PCI IO space */
	{ DC21285_PCI_IO_VBASE,			DC21285_PCI_IO_BASE,
	    DC21285_PCI_IO_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1MB for PCI IACK space */
	{ DC21285_PCI_IACK_VBASE,		DC21285_PCI_IACK_SPECIAL,
	    DC21285_PCI_IACK_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 16MB of type 1 PCI config access */
	{ DC21285_PCI_TYPE_1_CONFIG_VBASE,	DC21285_PCI_TYPE_1_CONFIG,
	    DC21285_PCI_TYPE_1_CONFIG_VSIZE,	VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 16MB of type 0 PCI config access */
	{ DC21285_PCI_TYPE_0_CONFIG_VBASE,	DC21285_PCI_TYPE_0_CONFIG,
	    DC21285_PCI_TYPE_0_CONFIG_VSIZE,	VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

	/* Map 1MB of 32 bit PCI address space for ISA MEM accesses via PCI */
	{ DC21285_PCI_ISA_MEM_VBASE,		DC21285_PCI_MEM_BASE,
	    DC21285_PCI_ISA_MEM_VSIZE,		VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },

#if NIGSFB > 0
	/* XXX: uwe: Map 16MB of PCI address space for CyberPro as console */
	{ IGS_PCI_MEM_VBASE,	DC21285_PCI_MEM_BASE + IGS_PCI_MEM_BASE,
	    IGS_PCI_MEM_VSIZE,			VM_PROT_READ|VM_PROT_WRITE,
	    PTE_NOCACHE },
#endif

	{ 0, 0, 0, 0, 0 }
};

/*
 * u_int initarm(...);
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
	extern char _end[];

	/*
	 * Turn the led off, then turn it yellow.
	 * 0x80 - red; 0x04 - fan; 0x02 - green.
	 */
	ISA_PUTBYTE(0x338, 0x04);
	ISA_PUTBYTE(0x338, 0x86);

	/*
	 * Set up a diagnostic console so we can see what's going
	 * on.
	 */
	cn_tab = &kcomcons;

	/* Talk to the user */
	printf("\nNetBSD/netwinder booting ...\n");

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("CPU not recognized!");

	/*
	 * We are currently running with the MMU enabled and the
	 * entire address space mapped VA==PA, except for the
	 * first 64MB of RAM is also double-mapped at 0xf0000000.
	 * There is an L1 page table at 0x00008000.
	 *
	 * We also have the 21285's PCI I/O space mapped where
	 * we expect it.
	 */

	printf("initarm: Configuring system ...\n");

	/*
	 * Copy out the boot info passed by the firmware.  Note that
	 * early versions of NeTTrom fill this in with bogus values,
	 * so we need to sanity check it.
	 */
	memcpy(&nwbootinfo, (void *)(KERNEL_BASE + 0x100),
	    sizeof(nwbootinfo));
#ifdef VERBOSE_INIT_ARM
	printf("NeTTrom boot info:\n");
	printf("\tpage size = 0x%08lx\n", nwbootinfo.bi_pagesize);
	printf("\tnpages = %ld (0x%08lx)\n", nwbootinfo.bi_nrpages,
	    nwbootinfo.bi_nrpages);
	printf("\trootdev = 0x%08lx\n", nwbootinfo.bi_rootdev);
	printf("\tcmdline = %s\n", nwbootinfo.bi_cmdline);
#endif
	if (nwbootinfo.bi_nrpages != 0x02000 &&
	    nwbootinfo.bi_nrpages != 0x04000 &&
	    nwbootinfo.bi_nrpages != 0x08000 &&
	    nwbootinfo.bi_nrpages != 0x10000) {
		nwbootinfo.bi_pagesize = 0xdeadbeef;
		nwbootinfo.bi_nrpages = 0x01000;	/* 16MB */
		nwbootinfo.bi_rootdev = 0;
	}

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independent */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = 0;
	bootconfig.dram[0].pages = nwbootinfo.bi_nrpages;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.
	 *
	 * Since the NetWinder NeTTrom doesn't load ELF symbols
	 * for us, we can safely assume that everything after end[]
	 * is free.  We start there and allocate upwards.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = ((((vaddr_t) _end) + PGOFSET) & ~PGOFSET) -
	    KERNEL_BASE;
	physical_freeend = physical_end;
	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
	       physical_freestart, free_pages, free_pages);
#endif

	physmem = (physical_end - physical_start) / PAGE_SIZE;

	/* Tell the user about the memory */
	printf("physmemory: %d pages at 0x%08lx -> 0x%08lx\n", physmem,
	    physical_start, physical_end - 1);

	/*
	 * Okay, we need to allocate some fixed page tables to get the
	 * kernel going.  We allocate one page directory and a number
	 * of page tables and store the physical addresses in the
	 * kernel_pt_table array.
	 *
	 * The kernel page directory must be on a 16K boundary.  The page
	 * tables must be on 4K boundaries.  What we do is allocate the
	 * page directory on the first 16K boundary that we encounter,
	 * and the page tables on 4K boundaries otherwise.  Since we
	 * allocate at least 3 L2 page tables, we are guaranteed to
	 * encounter at least one 16K aligned region.
	 */

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif

	/* Define a macro to simplify memory allocation */
#define	valloc_pages(var, np)			\
	alloc_pages((var).pv_pa, (np));		\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;

#define alloc_pages(var, np)			\
	(var) = physical_freestart;		\
	physical_freestart += ((np) * PAGE_SIZE);\
	free_pages -= (np);			\
	memset((char *)(var), 0, ((np) * PAGE_SIZE));

	loop1 = 0;
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
	 * Now we start consturction of the L1 page table
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

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr =
	    KERNEL_VM_BASE + (KERNEL_PT_VMDATA_NUM * 0x00400000);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	{
		/*
		 * The kernel starts in the first 1MB of RAM, and we'd
		 * like to use a section mapping for text, so we'll just
		 * map from KERNEL_BASE to etext[] to _end[].
		 */

		extern char etext[];
		size_t textsize = (uintptr_t) etext - KERNEL_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;

		textsize = textsize & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;

		logical = 0;		/* offset into RAM */

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

	/* Map the vector page. */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	/*
	 * Map devices we can map w/ section mappings.
	 */
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
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
	       physical_freestart, free_pages, free_pages);
	printf("switching to new L1 page table  @%#lx...", kernel_l1pt.pv_pa);
#endif

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(kernel_l1pt.pv_pa, true);
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

#ifdef VERBOSE_INIT_ARM
	printf("done!\n");
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
	printf("init subsystems: stacks ");

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
	printf("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
	printf("undefined ");
	undefined_init();

	/* Load memory into UVM. */
	printf("page ");
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */

	/* XXX Always one RAM block -- nuke the loop. */
	for (loop = 0; loop < bootconfig.dramblocks; loop++) {
		paddr_t start = (paddr_t)bootconfig.dram[loop].address;
		paddr_t end = start + (bootconfig.dram[loop].pages * PAGE_SIZE);
#if NISADMA > 0
		paddr_t istart, isize;
		extern struct arm32_dma_range *footbridge_isa_dma_ranges;
		extern int footbridge_isa_dma_nranges;
#endif

		if (start < physical_freestart)
			start = physical_freestart;
		if (end > physical_freeend)
			end = physical_freeend;

#if 0
		printf("%d: %lx -> %lx\n", loop, start, end - 1);
#endif

#if NISADMA > 0
		if (arm32_dma_range_intersect(footbridge_isa_dma_ranges,
					      footbridge_isa_dma_nranges,
					      start, end - start,
					      &istart, &isize)) {
			/*
			 * Place the pages that intersect with the
			 * ISA DMA range onto the ISA DMA free list.
			 */
#if 0
			printf("    ISADMA 0x%lx -> 0x%lx\n", istart,
			    istart + isize - 1);
#endif
			uvm_page_physload(atop(istart),
			    atop(istart + isize), atop(istart),
			    atop(istart + isize), VM_FREELIST_ISADMA);

			/*
			 * Load the pieces that come before the
			 * intersection onto the default free list.
			 */
			if (start < istart) {
#if 0
				printf("    BEFORE 0x%lx -> 0x%lx\n",
				    start, istart - 1);
#endif
				uvm_page_physload(atop(start),
				    atop(istart), atop(start),
				    atop(istart), VM_FREELIST_DEFAULT);
			}

			/*
			 * Load the pieces that come after the
			 * intersection onto the default free list.
			 */
			if ((istart + isize) < end) {
#if 0
				printf("     AFTER 0x%lx -> 0x%lx\n",
				    (istart + isize), end - 1);
#endif
				uvm_page_physload(atop(istart + isize),
				    atop(end), atop(istart + isize),
				    atop(end), VM_FREELIST_DEFAULT);
			}
		} else {
			uvm_page_physload(atop(start), atop(end),
			    atop(start), atop(end), VM_FREELIST_DEFAULT);
		}
#else /* NISADMA > 0 */
		uvm_page_physload(atop(start), atop(end),
		    atop(start), atop(end), VM_FREELIST_DEFAULT);
#endif /* NISADMA > 0 */
	}

	/* Boot strap pmap telling it where the kernel page table is */
	printf("pmap ");
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

	/* Now that pmap is inited, we can set cpu_reset_address */
	cpu_reset_address_paddr = vtophys((vaddr_t)netwinder_reset);

	/* Setup the IRQ system */
	printf("irq ");
	footbridge_intr_init();
	printf("done.\n");

	/*
	 * Warn the user if the bootinfo was bogus.  We already
	 * faked up some safe values.
	 */
	if (nwbootinfo.bi_pagesize == 0xdeadbeef)
		printf("WARNING: NeTTrom boot info corrupt\n");

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* Turn the led green */
	ISA_PUTBYTE(0x338, 0x06);

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

void
process_kernel_args(char *args)
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

void
consinit(void)
{
	static int consinit_called = 0;
	const char *console = CONSDEVNAME;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#ifdef DIAGNOSTIC
	printf("consinit(\"%s\")\n", console);
#endif

#if NISA > 0
	/* Initialise the ISA subsystem early ... */
	isa_footbridge_init(DC21285_PCI_IO_VBASE, DC21285_PCI_ISA_MEM_VBASE);
#endif

	if (strncmp(console, "igsfb", 5) == 0) {
#if NIGSFB > 0
		int res;

		footbridge_pci_bs_tag_init();

		/*
		 * XXX: uwe: special case mapping for the igsfb memory space.
		 * 
		 * The problem with this is that when footbridge is
		 * attached during normal autoconfiguration the bus
		 * space tags will be reinited and these hooks lost.
		 * However, since igsfb(4) don't unmap memory during
		 * normal operation, this is ok.  But if the igsfb is
		 * configured but is not a console, we waste 16M of
		 * kernel VA space.
		 */
		footbridge_pci_mem_bs_tag.bs_map = nw_footbridge_mem_bs_map;
		footbridge_pci_mem_bs_tag.bs_unmap = nw_footbridge_mem_bs_unmap;

		igsfb_pci_cnattach(&footbridge_pci_io_bs_tag,
				   &footbridge_pci_mem_bs_tag,
				   &footbridge_pci_chipset,
				   0, 8, 0);
#if NPCKBC > 0
		res = pckbc_cnattach(&isa_io_bs_tag,
				     IO_KBD, KBCMDP, PCKBC_KBD_SLOT, 0);
		if (res)
			printf("pckbc_cnattach: %d!\n", res);
#endif
#else
		panic("igsfb console not configured");
#endif /* NIGSFB */
	} else {
#ifdef DIAGNOSTIC
		if (strncmp(console, "com", 3) != 0) {
			printf("consinit: unknown CONSDEVNAME=\"%s\","
			       " falling back to \"com\"\n", console);
		}
#endif
#if NCOM > 0
		if (comcnattach(&isa_io_bs_tag, CONCOMADDR, comcnspeed,
				COM_FREQ, COM_TYPE_NORMAL, comcnmode))
			panic("can't init serial console @%x", CONCOMADDR);
#else
		panic("serial console @%x not configured", CONCOMADDR);
#endif
	}
}


#if NIGSFB > 0
static int
nw_footbridge_mem_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	bus_addr_t startpa, endpa;

	/* Round the allocation to page boundries */
	startpa = trunc_page(bpa);
	endpa = round_page(bpa + size);

	/*
	 * Check for mappings of the igsfb(4) memory space as we have
	 * this space already mapped.
	 */
	if (startpa >= IGS_PCI_MEM_BASE
	    && endpa < (IGS_PCI_MEM_BASE + IGS_PCI_MEM_VSIZE)) {
		/* Store the bus space handle */
		*bshp =  IGS_PCI_MEM_VBASE
			+ (bpa - IGS_PCI_MEM_BASE);
#ifdef DEBUG
		printf("nw/mem_bs_map: %08x+%08x: %08x..%08x -> %08x\n",
		       (u_int32_t)bpa, (u_int32_t)size,
		       (u_int32_t)startpa, (u_int32_t)endpa,
		       (u_int32_t)*bshp);
#endif
		return 0;
	}

	return (footbridge_mem_bs_map(t, bpa, size, cacheable, bshp));
}


static void
nw_footbridge_mem_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	/*
	 * Check for mappings of the igsfb(4) memory space as we have
	 * this space already mapped.
	 */
	if (bsh >= IGS_PCI_MEM_VBASE
	    && bsh < (IGS_PCI_MEM_VBASE + IGS_PCI_MEM_VSIZE)) {
#ifdef DEBUG
		printf("nw/bs_unmap: 0x%08x\n", (u_int32_t)bsh);
#endif
		return;
	}

	footbridge_mem_bs_unmap(t, bsh, size);
}
#endif /* NIGSFB */


static bus_space_handle_t kcom_base = (bus_space_handle_t) (DC21285_PCI_IO_VBASE + CONCOMADDR);

#define	KCOM_GETBYTE(r)		generic_bs_r_1(0, kcom_base, (r))
#define	KCOM_PUTBYTE(r,v)	generic_bs_w_1(0, kcom_base, (r), (v))

static int
kcomcngetc(dev_t dev)
{
	int stat, c;

	/* block until a character becomes available */
	while (!ISSET(stat = KCOM_GETBYTE(com_lsr), LSR_RXRDY))
		;

	c = KCOM_GETBYTE(com_data);
	stat = KCOM_GETBYTE(com_iir);
	return c;
}

/*
 * Console kernel output character routine.
 */
static void
kcomcnputc(dev_t dev, int c)
{
	int timo;

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (!ISSET(KCOM_GETBYTE(com_lsr), LSR_TXRDY) && --timo)
		continue;

	KCOM_PUTBYTE(com_data, c);

	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(KCOM_GETBYTE(com_lsr), LSR_TXRDY) && --timo)
		continue;
}

static void
kcomcnpollc(dev_t dev, int on)
{
}

struct consdev kcomcons = {
	NULL, NULL, kcomcngetc, kcomcnputc, kcomcnpollc, NULL,
	NULL, NULL, NODEV, CN_NORMAL
};
