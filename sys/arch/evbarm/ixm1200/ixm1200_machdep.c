/*	$NetBSD: ixm1200_machdep.c,v 1.49.2.2 2013/01/16 05:32:54 yamt Exp $ */

/*
 * Copyright (c) 2002, 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 *      This product includes software developed by Mark Brinicombe
 *      for the NetBSD Project.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixm1200_machdep.c,v 1.49.2.2 2013/01/16 05:32:54 yamt Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
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

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define ELFSIZE	DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <machine/bootconfig.h>
#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>
#include <arm/ixp12x0/ixp12x0_comreg.h>
#include <arm/ixp12x0/ixp12x0_comvar.h>
#include <arm/ixp12x0/ixp12x0_pcireg.h>

#include <evbarm/ixm1200/ixm1200reg.h>
#include <evbarm/ixm1200/ixm1200var.h>

/* XXX for consinit related hacks */
#include <sys/conf.h>

void ixp12x0_reset(void) __attribute__((noreturn));

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
 * This is machine architecture dependent as it varies depending
 * on where the ROM appears when you turn the MMU off.
 */

/*
 * Define the default console speed for the board.
 */
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB)) | CS8) /* 8N1 */
#endif
#ifndef CONSPEED
#define CONSPEED B38400
#endif
#ifndef CONADDR
#define CONADDR IXPCOM_UART_BASE
#endif

BootConfig bootconfig;          /* Boot config storage */
char *boot_args = NULL;
char *boot_file = NULL;

vm_offset_t physical_start;
vm_offset_t physical_freestart;
vm_offset_t physical_freeend;
vm_offset_t physical_end;
u_int free_pages;

/*int debug_flags;*/
#ifndef PMAP_STATIC_L1S
int max_processes = 64;                 /* Default number */
#endif  /* !PMAP_STATIC_L1S */

vm_offset_t msgbufphys;

extern int end;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif  /* PMAP_DEBUG */

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define KERNEL_PT_KERNEL_NUM	2	
#define KERNEL_PT_IO		(KERNEL_PT_KERNEL + KERNEL_PT_KERNEL_NUM)
					/* Page table for mapping IO */
#define KERNEL_PT_VMDATA	(KERNEL_PT_IO + 1)
					/* Page tables for mapping kernel VM */
#define KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

#ifdef CPU_IXP12X0
#define CPU_IXP12X0_CACHE_CLEAN_SIZE (0x4000 * 2)
extern unsigned int ixp12x0_cache_clean_addr;
extern unsigned int ixp12x0_cache_clean_size;
static vaddr_t ixp12x0_cc_base;
#endif  /* CPU_IXP12X0 */

/* Prototypes */

void consinit(void);
u_int cpu_get_control(void);

void ixdp_ixp12x0_cc_setup(void);

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
		pmf_system_shutdown(boothowto);
		printf("Halted while still in the ICE age.\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		ixp12x0_reset();
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

	pmf_system_shutdown(boothowto);

	/* Make sure IRQ's are disabled */
	IRQdisable;

	if (howto & RB_HALT) {
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");

	/* all interrupts are disabled */
	disable_interrupts(I32_bit);

	ixp12x0_reset();

	/* ...and if that didn't work, just croak. */
	printf("RESET FAILED!\n");
	for (;;);
}

/* Static device mappings. */
static const struct pmap_devmap ixm1200_devmap[] = {
	/* StrongARM System and Peripheral Registers */
	{
		IXP12X0_SYS_VBASE,
		IXP12X0_SYS_HWBASE,
		IXP12X0_SYS_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	/* PCI Registers Accessible Through StrongARM Core */
	{
		IXP12X0_PCI_VBASE, IXP12X0_PCI_HWBASE,
		IXP12X0_PCI_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	/* PCI Registers Accessible Through I/O Cycle Access */
	{
		IXP12X0_PCI_IO_VBASE, IXP12X0_PCI_IO_HWBASE,
		IXP12X0_PCI_IO_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	/* PCI Type0 Configuration Space */
	{
		IXP12X0_PCI_TYPE0_VBASE, IXP12X0_PCI_TYPE0_HWBASE,
		IXP12X0_PCI_TYPE0_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	/* PCI Type1 Configuration Space */
	{
		IXP12X0_PCI_TYPE1_VBASE, IXP12X0_PCI_TYPE1_HWBASE,
		IXP12X0_PCI_TYPE1_SIZE,
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		0,
		0,
		0,
		0,
		0
	},
};

/*
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
	u_int kerneldatasize, symbolsize;
	vaddr_t l1pagetable;
	vaddr_t freemempos;
#if NKSYMS || defined(DDB) || defined(MODULAR)
        Elf_Shdr *sh;
#endif

	cpu_reset_address = ixp12x0_reset;

        /*
         * Since we map v0xf0000000 == p0x90000000, it's possible for
         * us to initialize the console now.
         */
	consinit();

#ifdef VERBOSE_INIT_ARM
	/* Talk to the user */
	printf("\nNetBSD/evbarm (IXM1200) booting ...\n");
#endif

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("CPU not recognized!");

	/* XXX overwrite bootconfig to hardcoded values */
	bootconfig.dram[0].address = 0xc0000000;
	bootconfig.dram[0].pages   = 0x10000000 / PAGE_SIZE; /* SDRAM 256MB */
	bootconfig.dramblocks = 1;

	kerneldatasize = (uint32_t)&end - (uint32_t)KERNEL_TEXT_BASE;

	symbolsize = 0;

#ifdef PMAP_DEBUG
	pmap_debug(-1);
#endif

#if NKSYMS || defined(DDB) || defined(MODULAR)
        if (! memcmp(&end, "\177ELF", 4)) {
                sh = (Elf_Shdr *)((char *)&end + ((Elf_Ehdr *)&end)->e_shoff);
                loop = ((Elf_Ehdr *)&end)->e_shnum;
                for(; loop; loop--, sh++)
                        if (sh->sh_offset > 0 &&
                            (sh->sh_offset + sh->sh_size) > symbolsize)
                                symbolsize = sh->sh_offset + sh->sh_size;
        }
#endif
#ifdef VERBOSE_INIT_ARM
	printf("kernsize=0x%x\n", kerneldatasize);
#endif
	kerneldatasize += symbolsize;
	kerneldatasize = ((kerneldatasize - 1) & ~(PAGE_SIZE * 4 - 1)) + PAGE_SIZE * 8;

	/*
	 * Set up the variables that define the availablilty of physcial
	 * memory
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = physical_start
		+ (KERNEL_TEXT_BASE - KERNEL_BASE) + kerneldatasize;
	physical_freeend = physical_end;

	physmem = (physical_end - physical_start) / PAGE_SIZE;

	freemempos = 0xc0000000;

#ifdef VERBOSE_INIT_ARM
	printf("Allocating page tables\n");
#endif
	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

#ifdef VERBOSE_INIT_ARM
	printf("CP15 Register1 = 0x%08x\n", cpu_get_control());
	printf("freestart = 0x%08lx, free_pages = %d (0x%08x)\n",
		physical_freestart, free_pages, free_pages);
	printf("physical_start = 0x%08lx, physical_end = 0x%08lx\n",
		physical_start, physical_end);
#endif

	/* Define a macro to simplify memory allocation */
#define valloc_pages(var, np)			\
	alloc_pages((var).pv_pa, (np));		\
	(var).pv_va = KERNEL_BASE + (var).pv_pa - physical_start;
#define alloc_pages(var, np)				\
	(var) = freemempos;				\
	memset((char *)(var), 0, ((np) * PAGE_SIZE));	\
	freemempos += (np) * PAGE_SIZE;

	loop1 = 0;
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

#ifdef DIAGNOSTIC
	/* This should never be able to happen but better confirm that. */
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE-1)) != 0)
		panic("initarm: Failed to align the kernel page directory");
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
	printf("IRQ stack: p0x%08lx v0x%08lx\n", irqstack.pv_pa, irqstack.pv_va); 
	printf("ABT stack: p0x%08lx v0x%08lx\n", abtstack.pv_pa, abtstack.pv_va); 
	printf("UND stack: p0x%08lx v0x%08lx\n", undstack.pv_pa, undstack.pv_va); 
	printf("SVC stack: p0x%08lx v0x%08lx\n", kernelstack.pv_pa, kernelstack.pv_va); 
#endif

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

#ifdef CPU_IXP12X0
        /*
         * XXX totally stuffed hack to work round problems introduced
         * in recent versions of the pmap code. Due to the calls used there
         * we cannot allocate virtual memory during bootstrap.
         */
	for(;;) {
		alloc_pages(ixp12x0_cc_base, 1);
		if (! (ixp12x0_cc_base & (CPU_IXP12X0_CACHE_CLEAN_SIZE - 1)))
			break;
	}
	{
		vaddr_t dummy;
		alloc_pages(dummy, CPU_IXP12X0_CACHE_CLEAN_SIZE / PAGE_SIZE - 1);
	}
	ixp12x0_cache_clean_addr = ixp12x0_cc_base;
	ixp12x0_cache_clean_size = CPU_IXP12X0_CACHE_CLEAN_SIZE / 2;
#endif /* CPU_IXP12X0 */

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
	pmap_link_l2pt(l1pagetable, ARM_VECTORS_HIGH & ~(0x00400000 - 1),
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

	pmap_link_l2pt(l1pagetable, IXP12X0_IO_VBASE,
	    &kernel_pt_table[KERNEL_PT_IO]);

#ifdef VERBOSE_INIT_ARM
	printf("Mapping kernel\n");
#endif

#if XXX
	/* Now we fill in the L2 pagetable for the kernel code/data */
	{
		extern char etext[], _end[];
		size_t textsize = (uintptr_t) etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;
                
		logical = 0x00200000;   /* offset of kernel in RAM */

		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
		logical += pmap_map_chunk(l1pagetable, KERNEL_BASE + logical,
		    physical_start + logical, totalsize - textsize,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	}
#else
	{
		pmap_map_chunk(l1pagetable, KERNEL_TEXT_BASE,
                    KERNEL_TEXT_BASE, kerneldatasize,
                    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
	}
#endif

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
	pmap_map_entry(l1pagetable, ARM_VECTORS_HIGH, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

#ifdef VERBOSE_INIT_ARM
	printf("systempage (vector page): p0x%08lx v0x%08lx\n",
	       systempage.pv_pa, vector_page);
#endif

	/* Map the statically mapped devices. */
	pmap_devmap_bootstrap(l1pagetable, ixm1200_devmap);

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

	/*
	 * Map the Dcache Flush page.
	 * Hw Ref Manual 3.2.4.5 Software Dcache Flush 
	 */
	pmap_map_chunk(l1pagetable, ixp12x0_cache_clean_addr, 0xe0000000,
	    CPU_IXP12X0_CACHE_CLEAN_SIZE, VM_PROT_READ, PTE_CACHE);

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(kernel_l1pt.pv_pa, true);
	cpu_tlb_flushID();
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved here from cpu_startup() as data_abort_handler() references
	 * this during init
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

	/*
	 * We must now clean the cache again....
	 * Cleaning may be done by reading new data to displace any
	 * dirty data in the cache. This will have happened in cpu_setttb()
	 * but since we are boot strapping the addresses used for the read
	 * may have just been remapped and thus the cache could be out
	 * of sync. A re-clean after the switch will cure this.
	 * After booting there are no gross reloations of the kernel thus
	 * this problem will not occur after initarm().
	 */
	cpu_idcache_wbinv_all();

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
#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("kstack V%08lx P%08lx\n", kernelstack.pv_va,
		    kernelstack.pv_pa);
#endif  /* PMAP_DEBUG */

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler. Until then we will use a handler that just panics but
	 * tells us why.
	 * Initialisation of the vetcors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;
#ifdef VERBOSE_INIT_ARM
	printf("\ndata_abort_handler_address = %08x\n", data_abort_handler_address);
	printf("prefetch_abort_handler_address = %08x\n", prefetch_abort_handler_address);
	printf("undefined_handler_address = %08x\n", undefined_handler_address);
#endif

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

	/* Setup the IRQ system */
#ifdef VERBOSE_INIT_ARM
	printf("irq ");
#endif
	ixp12x0_intr_init();

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("freestart = 0x%08lx, free_pages = %d (0x%x)\n",
		physical_freestart, free_pages, free_pages);
	printf("freemempos=%08lx\n", freemempos);
	printf("switching to new L1 page table  @%#lx... \n", kernel_l1pt.pv_pa);
#endif

	consinit();
#ifdef VERBOSE_INIT_ARM
	printf("consinit \n");
#endif

	ixdp_ixp12x0_cc_setup();

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf(symbolsize, ((int *)&end), ((char *)&end) + symbolsize);
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

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	pmap_devmap_register(ixm1200_devmap);

	if (ixpcomcnattach(&ixp12x0_bs_tag,
			   IXPCOM_UART_HWBASE, IXPCOM_UART_VBASE,
			   CONSPEED, CONMODE))
		panic("can't init serial console @%lx", IXPCOM_UART_HWBASE);
}

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
ixdp_ixp12x0_cc_setup(void)
{
	int loop;
	paddr_t kaddr;
	pt_entry_t *pte;

	(void) pmap_extract(pmap_kernel(), KERNEL_TEXT_BASE, &kaddr);
	for (loop = 0; loop < CPU_IXP12X0_CACHE_CLEAN_SIZE; loop += PAGE_SIZE) {
                pte = vtopte(ixp12x0_cc_base + loop);
                *pte = L2_S_PROTO | kaddr |
                    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) | pte_l2_s_cache_mode;
		PTE_SYNC(pte);
        }
	ixp12x0_cache_clean_addr = ixp12x0_cc_base;
	ixp12x0_cache_clean_size = CPU_IXP12X0_CACHE_CLEAN_SIZE / 2;
}
