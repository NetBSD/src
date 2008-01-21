/*	$NetBSD: smdk2410_machdep.c,v 1.12.4.2 2008/01/21 09:36:15 yamt Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003, 2005 Genetec Corporation
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
 * Machine dependant functions for kernel setup for Samsung SMDK2410
 * derived from integrator_machdep.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smdk2410_machdep.c,v 1.12.4.2 2008/01/21 09:36:15 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_pmap_debug.h"
#include "opt_md.h"

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

#include <arm/s3c2xx0/s3c2410reg.h>
#include <arm/s3c2xx0/s3c2410var.h>

#include "ksyms.h"

#ifndef	SDRAM_START
#define	SDRAM_START	S3C2410_SDRAM_START
#endif
#ifndef	SDRAM_SIZE
#define	SDRAM_SIZE	(32*1024*1024)
#endif

/*
 * Address to map I/O registers in early initialize stage.
 */
#define SMDK2410_IO_VBASE	0xfd000000

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
#define UND_STACK_SIZE	1

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
	printf("boot: howto=%08x curproc=%p\n", howto, curproc);
#endif

	cpu_reset_address = vtophys((u_int)s3c2410_softreset);

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

/*
 * Static device mappings. These peripheral registers are mapped at
 * fixed virtual addresses very early in initarm() so that we can use
 * them while booting the kernel , and stay at the same address
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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

#define	_V(n)	(SMDK2410_IO_VBASE + (n) * L1_S_SIZE)

#define	GPIO_VBASE	_V(0)
#define	INTCTL_VBASE	_V(1)
#define	CLKMAN_VBASE	_V(2)
#define	UART_VBASE	_V(3)
#ifdef	MEMORY_DISK_DYNAMIC
#define	MEMORY_DISK_VADDR	_V(4)
#endif

static const struct pmap_devmap smdk2410_devmap[] = {
	/* GPIO registers */
	{
		GPIO_VBASE,
		_A(S3C2410_GPIO_BASE),
		_S(S3C2410_GPIO_SIZE),
		VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
	},
	{
		INTCTL_VBASE,
		_A(S3C2410_INTCTL_BASE),
		_S(S3C2410_INTCTL_SIZE),
		VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
	},
	{
		CLKMAN_VBASE,
		_A(S3C2410_CLKMAN_BASE),
		_S(S3C24X0_CLKMAN_SIZE),
		VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
	},
	{	/* UART registers for UART0, 1, 2. */
		UART_VBASE,
		_A(S3C2410_UART0_BASE),
		_S(S3C2410_UART_BASE(3) - S3C2410_UART0_BASE),
		VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
	},

	{ 0, 0, 0, 0 }
};

#undef	_A
#undef	_S

static inline	pd_entry_t *
read_ttb(void)
{
	long ttb;

	__asm volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r"(ttb));


	return (pd_entry_t *)(ttb & ~((1 << 14) - 1));
}


#define	ioreg_read8(a)  	(*(volatile uint8_t *)(a))
#define	ioreg_write8(a,v)	(*(volatile uint8_t *)(a)=(v))
#define	ioreg_read32(a)  	(*(volatile uint32_t *)(a))
#define	ioreg_write32(a,v)  	(*(volatile uint32_t *)(a)=(v))

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
	extern int etext __asm("_etext");
	extern int end __asm("_end");
	pv_addr_t kernel_l1pt;
	int progress_counter = 0;

#ifdef DO_MEMORY_DISK
	vm_offset_t md_root_start;
#define MD_ROOT_SIZE (MEMORY_DISK_ROOT_SIZE * DEV_BSIZE)
#endif

#define gpio_read8(reg) ioreg_read8(GPIO_VBASE + (reg))

#define LEDSTEP()  __LED(progress_counter++)

#define pdatf (*(volatile uint8_t *)(S3C2410_GPIO_BASE+GPIO_PFDAT))
#define __LED(x)  (pdatf = (pdatf & ~0xf0) | (~(x) & 0xf0))

	LEDSTEP();

	/* CS8900A on CS3 and CL-PD7610 need nBE1 signal. make sure
	 * memory controller is set correctly.  (USB download firmware
	 * doesn't do this right) Also, we use WAIT signal for them.
	 */
	ioreg_write32(S3C2410_MEMCTL_BASE + MEMCTL_BWSCON, 
	    (BWSCON_ST|BWSCON_WS) << BWSCON_BANK_SHIFT(2) |
	    (BWSCON_ST|BWSCON_WS) << BWSCON_BANK_SHIFT(3) | 
	    ioreg_read32(S3C2410_MEMCTL_BASE + MEMCTL_BWSCON));
	/* tweak access timing for CS8900A */
	ioreg_write32(S3C2410_MEMCTL_BASE + MEMCTL_BANKCON(3),
	    (0<<BANKCON_TACS_SHIFT)|(1<<BANKCON_TCOS_SHIFT)|
	    (7<<BANKCON_TACC_SHIFT)|(0<<BANKCON_TOCH_SHIFT)|
	    (0<<BANKCON_TCAH_SHIFT));

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	LEDSTEP();

	/*
	 * Map I/O registers that are used in startup.  Now we are
	 * still using page table prepared by bootloader.  Later we'll
	 * map those registers at the same address in the kernel page
	 * table.
	 */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), smdk2410_devmap);

#undef	pdatf
#define pdatf (*(volatile uint8_t *)(GPIO_VBASE+GPIO_PFDAT))


	LEDSTEP();

	/* Disable all peripheral interrupts */
	ioreg_write32(INTCTL_VBASE + INTCTL_INTMSK, ~0);

	/* initialize some variables so that splfoo() doesn't
	   touch illegal address.  */
	s3c2xx0_intr_bootstrap(INTCTL_VBASE);

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
	printf("\nNetBSD/evbarm (SMDK2410) booting ...\n");
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
	 * 0x30000000 - 0x31ffffff    SDRAM (32MB)
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

#ifdef DO_MEMORY_DISK
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

	physical_freestart = SDRAM_START;	/* XXX */
	physical_freeend = SDRAM_START + 0x00200000;

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
	 * tables must be on 4K boundaries.  What we do is allocate the
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
	kernel_l1pt.pv_va = 0;
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
	if (!kernel_l1pt.pv_pa || (kernel_l1pt.pv_pa & (L1_TABLE_SIZE - 1)) != 0)
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
	pmap_map_chunk(l1pagetable, MEMORY_DISK_VADDR, md_root_start,
	    MD_ROOT_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	md_root_setconf((void *)md_root_start, MD_ROOT_SIZE);
#endif /* MEMORY_DISK_DYNAMIC */
	/*
	 * map integrated peripherals at same address in l1pagetable
	 * so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1pagetable, smdk2410_devmap);

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

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif
	{
		uint8_t  gpio = ~gpio_read8(GPIO_PFDAT);
		
		if (gpio & (1<<0)) /* SW1 (EINT0) */
			boothowto ^= RB_SINGLE;
		if (gpio & (1<<2)) /* SW2 (EINT2) */
			boothowto ^= RB_KDB;
#ifdef VERBOSE_INIT_ARM
		printf( "sw: %x boothowto: %x\n", gpio, boothowto );
#endif
	}

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
	bus_space_tag_t iot = &s3c2xx0_bs_tag;
	int pclk;

	if (consinit_done != 0)
		return;

	consinit_done = 1;

	s3c24x0_clock_freq2(CLKMAN_VBASE, NULL, NULL, &pclk);

#if NSSCOM > 0
#ifdef SSCOM0CONSOLE
	if (0 == s3c2410_sscom_cnattach(iot, 0, comcnspeed,
		pclk, comcnmode))
		return;
#endif
#ifdef SSCOM1CONSOLE
	if (0 == s3c2410_sscom_cnattach(iot, 1, comcnspeed,
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
	int pclk;

	if (strcmp(kgdb_devname, "sscom0") == 0)
		unit = 0;
	else if (strcmp(kgdb_devname, "sscom1") == 0)
		unit = 1;

	if (unit >= 0) {
		s3c24x0_clock_freq2(CLKMAN_VBASE, NULL, NULL, &pclk);

		s3c2410_sscom_kgdb_attach(&s3c2xx0_bs_tag,
		    unit, kgdb_rate, pclk, kgdb_sscom_mode);
	}
#endif
}
#endif

static inline void
writeback_dcache_line(vaddr_t va)
{
	/* writeback Dcache line */
	/* we can't use cpu_dcache_wb_range() here, because cpufuncs for ARM9
	 * assume write-through cache, and always flush Dcache instead of
	 * cleaning it. Since Boot loader maps page table with write-back
	 * cached, we really need to clean Dcache. */
	__asm("mcr	p15, 0, %0, c7, c10, 1"
	    : :	"r"(va));
}

static inline void
clean_dcache_line(vaddr_t va)
{
	/* writeback and invalidate Dcache line */
	__asm("mcr	p15, 0, %0, c7, c14, 1"
	    : : "r"(va));
}

static struct arm32_dma_range smdk2410_dma_ranges[1];

bus_dma_tag_t
s3c2xx0_bus_dma_init(struct arm32_bus_dma_tag *dma_tag_template)
{
	extern paddr_t physical_start, physical_end;
	struct arm32_bus_dma_tag *dmat;

	smdk2410_dma_ranges[0].dr_sysbase = physical_start;
	smdk2410_dma_ranges[0].dr_busbase = physical_start;
	smdk2410_dma_ranges[0].dr_len = physical_end - physical_start;

#if 1
	dmat = dma_tag_template;
#else
	dmat = malloc(sizeof *dmat, M_DEVBUF, M_NOWAIT);
	if (dmat == NULL)
		return NULL;
	*dmat =  *dma_tag_template;
#endif

	dmat->_ranges = smdk2410_dma_ranges;
	dmat->_nranges = 1;

	return dmat;
}
