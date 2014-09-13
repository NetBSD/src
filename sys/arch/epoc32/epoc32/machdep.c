/*	$NetBSD: machdep.c,v 1.4 2014/09/13 18:08:40 matt Exp $	*/
/*
 * Copyright (c) 2012, 2013 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.4 2014/09/13 18:08:40 matt Exp $");

#include "clpscom.h"
#include "clpslcd.h"
#include "wmcom.h"
#include "wmlcd.h"
#include "epockbd.h"
#include "ksyms.h"
#include "opt_ddb.h"
#include "opt_md.h"
#include "opt_modular.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/pmf.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/md.h>

#include <arm/locore.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>
#include <arm/arm32/pmap.h>

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/epoc32.h>

#include <arm/clps711x/clpssocvar.h>
#include <epoc32/windermere/windermerevar.h>
#include <epoc32/windermere/windermerereg.h>
#include <epoc32/dev/epockbdvar.h>

#include <machine/db_machdep.h>
#include <ddb/db_extern.h>

#define KERNEL_OFFSET		0x00030000
#define KERNEL_TEXT_BASE	(KERNEL_BASE + KERNEL_OFFSET)
#ifndef KERNEL_VM_BASE
#define KERNEL_VM_BASE		(KERNEL_BASE + 0x00300000)
#endif
#define KERNEL_VM_SIZE		0x04000000	/* XXXX 64M */

/* Define various stack sizes in pages */
#define IRQ_STACK_SIZE	1
#define ABT_STACK_SIZE	1
#define UND_STACK_SIZE	1


BootConfig bootconfig;		/* Boot config storage */
static char bootargs[256];
char *boot_args = NULL;

vaddr_t physical_start;
vaddr_t physical_freestart;
vaddr_t physical_freeend;
vaddr_t physical_end;
u_int free_pages;

paddr_t msgbufphys;

enum {
	KERNEL_PT_SYS = 0,	/* Page table for mapping proc0 zero page */
	KERNEL_PT_KERNEL,	/* Page table for mapping kernel and VM */

	NUM_KERNEL_PTS
};
pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

char epoc32_model[256];
int epoc32_fb_width;
int epoc32_fb_height;
int epoc32_fb_addr;

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
#define _S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE - 1))
static const struct pmap_devmap epoc32_devmap[] = {
	{
		ARM7XX_INTRREG_VBASE,		/* included com, lcd-ctrl */
		_A(ARM7XX_INTRREG_BASE),
		_S(ARM7XX_INTRREG_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	{ 0, 0, 0, 0, 0 }
};
static const struct pmap_devmap epoc32_fb_devmap[] = {
	{
		ARM7XX_FB_VBASE,
		_A(ARM7XX_FB_BASE),
		_S(ARM7XX_FB_SIZE),
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},

	{ 0, 0, 0, 0, 0 }
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
	extern char _end[];
	extern vaddr_t startup_pagetable;
	extern struct btinfo_common bootinfo;
	struct btinfo_common *btinfo = &bootinfo;
	struct btinfo_model *model = NULL;
	struct btinfo_memory *memory = NULL;
	struct btinfo_video *video = NULL;
	struct btinfo_bootargs *args = NULL;
	u_int l1pagetable, _end_physical;
	int loop, loop1, n, i;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* map some peripheral registers at static I/O area. */
	pmap_devmap_bootstrap(startup_pagetable, epoc32_devmap);

	bootconfig.dramblocks = 0;
	while (btinfo->type != BTINFO_NONE) {
		switch (btinfo->type) {
		case BTINFO_MODEL:
			model = (struct btinfo_model *)btinfo;
			btinfo = &(model + 1)->common;
			strncpy(epoc32_model, model->model,
			    sizeof(epoc32_model));
			break;

		case BTINFO_MEMORY:
			memory = (struct btinfo_memory *)btinfo;
			btinfo = &(memory + 1)->common;

			/*
			 * Fake bootconfig structure for the benefit of pmap.c
			 */
			i = bootconfig.dramblocks;
			bootconfig.dram[i].address = memory->address;
			bootconfig.dram[i].pages = memory->size / PAGE_SIZE;
			bootconfig.dramblocks++;
			break;

		case BTINFO_VIDEO:
			video = (struct btinfo_video *)btinfo;
			btinfo = &(video + 1)->common;
			epoc32_fb_width = video->width;
			epoc32_fb_height = video->height;
			break;

		case BTINFO_BOOTARGS:
			args = (struct btinfo_bootargs *)btinfo;
			btinfo = &(args + 1)->common;
			memcpy(bootargs, args->bootargs,
			    min(sizeof(bootargs), sizeof(args->bootargs)));
			bootargs[sizeof(bootargs) - 1] = '\0';
			boot_args = bootargs;
			break;

		default:
#define NEXT_BOOTINFO(bi) (struct btinfo_common *)((char *)bi + (bi)->len)

			btinfo = NEXT_BOOTINFO(btinfo);
		}
	}
	if (bootconfig.dramblocks == 0)
		panic("BTINFO_MEMORY not found");

	consinit();

	if (boot_args != NULL)
		parse_mi_bootargs(boot_args);

	physical_start = bootconfig.dram[0].address;
	physical_freestart = bootconfig.dram[0].address;
	physical_freeend = KERNEL_TEXT_BASE;

	free_pages = (physical_freeend - physical_freestart) / PAGE_SIZE;

	/* Define a macro to simplify memory allocation */
#define valloc_pages(var, np)				\
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
	if (!kernel_l1pt.pv_pa ||
	    (kernel_l1pt.pv_pa & (L1_TABLE_SIZE - 1)) != 0)
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

	alloc_pages(msgbufphys, round_page(MSGBUFSIZE) / PAGE_SIZE);

	/*
	 * Now we start construction of the L1 page table
	 * We start by mapping the L2 page tables into the L1.
	 * This means that we can replace L1 mappings later on if necessary
	 */
	l1pagetable = kernel_l1pt.pv_va;

	/* Map the L2 pages tables in the L1 page table */
	pmap_link_l2pt(l1pagetable, 0x00000000,
	    &kernel_pt_table[KERNEL_PT_SYS]);
	pmap_link_l2pt(l1pagetable, KERNEL_BASE,
	    &kernel_pt_table[KERNEL_PT_KERNEL]);

	/* update the top of the kernel VM */
	pmap_curmaxkvaddr = KERNEL_VM_BASE;

	/* Now we fill in the L2 pagetable for the kernel static code/data */
	{
		extern char etext[];
		size_t textsize = (uintptr_t) etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_TEXT_BASE;
		size_t datasize;
		PhysMem *dram = bootconfig.dram;
		u_int logical, physical, size;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;
		datasize = totalsize - textsize;	/* data and bss */

		logical = KERNEL_OFFSET;	/* offset of kernel in RAM */
		physical = KERNEL_OFFSET;
		i = 0;
		size = dram[i].pages * PAGE_SIZE - physical;
		/* Map kernel text section. */
		while (1 /*CONSTINT*/) {
			size = pmap_map_chunk(l1pagetable,
			    KERNEL_BASE + logical, dram[i].address + physical,
			    textsize < size ? textsize : size,
			    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
			logical += size;
			physical += size;
			textsize -= size;
			if (physical >= dram[i].pages * PAGE_SIZE) {
				i++;
				size = dram[i].pages * PAGE_SIZE;
				physical = 0;
			}
			if (textsize == 0)
				break;
		}
		size = dram[i].pages * PAGE_SIZE - physical;
		/* Map data and bss section. */
		while (1 /*CONSTINT*/) {
			size = pmap_map_chunk(l1pagetable,
			    KERNEL_BASE + logical, dram[i].address + physical,
			    datasize < size ? datasize : size,
			    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
			logical += size;
			physical += size;
			datasize -= size;
			if (physical >= dram[i].pages * PAGE_SIZE) {
				i++;
				size = dram[i].pages * PAGE_SIZE;
				physical = 0;
			}
			if (datasize == 0)
				break;
		}
		_end_physical = dram[i].address + physical;
		n = i;
		physical_end = dram[n].address + dram[n].pages * PAGE_SIZE;
		n++;
	}

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
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);

	pmap_devmap_bootstrap(l1pagetable, epoc32_devmap);
	pmap_devmap_bootstrap(l1pagetable, epoc32_fb_devmap);
	epoc32_fb_addr = ARM7XX_FB_VBASE;

	/*
	 * Now we have the real page tables in place so we can switch to them.
	 * Once this is done we will be running with the REAL kernel page
	 * tables.
	 */

	/* Switch tables */
	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);
	cpu_setttb(kernel_l1pt.pv_pa, true);
	cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2));

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */

	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler. Until then we will use a handler that just panics but
	 * tells us why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
	undefined_init();

        /* Load memory into UVM. */
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */
	uvm_page_physload(
	    atop(_end_physical), atop(physical_end),
	    atop(_end_physical), atop(physical_end),
	    VM_FREELIST_DEFAULT);
	physmem = bootconfig.dram[0].pages;
	for (i = 1; i < n; i++)
		physmem += bootconfig.dram[i].pages;
	if (physmem < 0x400000)
		physical_end = 0;
	for (loop = n; loop < bootconfig.dramblocks; loop++) {
		size_t start = bootconfig.dram[loop].address;
		size_t size = bootconfig.dram[loop].pages * PAGE_SIZE;

		uvm_page_physload(atop(start), atop(start + size),
		    atop(start), atop(start + size), VM_FREELIST_DEFAULT);
		physmem += bootconfig.dram[loop].pages;

		if (physical_end == 0 && physmem >= 0x400000 / PAGE_SIZE)
			/* Fixup physical_end for Series5. */
			physical_end = start + size;
	}

	/* Boot strap pmap telling it where the kernel page table is */
	pmap_bootstrap(KERNEL_VM_BASE, KERNEL_VM_BASE + KERNEL_VM_SIZE);

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* Firmware doesn't load symbols. */
	ddb_init(0, NULL, NULL);
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
cpu_reboot(int howto, char *bootstr)
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
		pmf_system_shutdown(boothowto);
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
		printf("rebooting...\n");
		cpu_reset();
		/*NOTREACHED*/
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

void
consinit(void)
{
	static int consinit_called = 0;
#if (NWMCOM + NCLPSCOM) > 0
	const tcflag_t mode = (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8;
#endif

	if (consinit_called)
		return;
	consinit_called = 1;

	if (strcmp(epoc32_model, "SERIES5 R1") == 0) {
#if NCLPSLCD > 0
		if (clpslcd_cnattach() == 0) {
#if NEPOCKBD > 0
			epockbd_cnattach();
#endif
			return;
		}
#endif
#if NCLPSCOM > 0
		if (clpscom_cnattach(ARM7XX_INTRREG_VBASE, 115200, mode) == 0)
			return;
#endif
	}
	if (strcmp(epoc32_model, "SERIES5mx") == 0) {
		vaddr_t vbase = ARM7XX_INTRREG_VBASE;
#if NWMCOM > 0
		vaddr_t offset;
		volatile uint8_t *gpio;
		int irda;
#endif

#if NWMLCD > 0
		if (wmlcd_cnattach() == 0) {
#if NEPOCKBD > 0
			epockbd_cnattach();
#endif
			return;
		}
#endif
#if NWMCOM > 0
		gpio = (uint8_t *)ARM7XX_INTRREG_VBASE + WINDERMERE_GPIO_OFFSET;
		if (0) {
			/* Enable UART0 to PCDR */
			*(gpio + 0x08) |= 1 << 5;
			offset = WINDERMERE_COM0_OFFSET;
			irda = 1;			/* IrDA */
		} else {
			/* Enable UART1 to PCDR */
			*(gpio + 0x08) |= 1 << 3;
			offset = WINDERMERE_COM1_OFFSET;
			irda = 0;			/* UART */
		}

		if (wmcom_cnattach(vbase + offset, 115200, mode, irda) == 0)
			return;
#endif
	}
	if (strcmp(epoc32_model, "SERIES7") == 0) {
	}
	panic("can't init console");
}
