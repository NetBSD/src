/*	$NetBSD: lubbock_machdep.c,v 1.10 2005/12/24 20:07:03 perry Exp $ */

/*
 * Copyright (c) 2002, 2003, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Machine dependant functions for kernel setup for 
 * Intel DBPXA250 evaluation board (a.k.a. Lubbock).
 * Based on iq80310_machhdep.c
 */
/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
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
 * Machine dependant functions for kernel setup for Intel IQ80310 evaluation
 * boards using RedBoot firmware.
 */

/*
 * DIP switches:
 *
 * S19: no-dot: set RB_KDB.  enter kgdb session.
 * S20: no-dot: set RB_SINGLE. don't go multi user mode.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lubbock_machdep.c,v 1.10 2005/12/24 20:07:03 perry Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_pmap_debug.h"
#include "opt_md.h"
#include "opt_com.h"
#include "md.h"
#include "lcd.h"

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

#include <sys/conf.h>
#include <dev/cons.h>
#include <dev/md.h>
#include <dev/ic/smc91cxxreg.h>

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
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <evbarm/lubbock/lubbock_reg.h>
#include <evbarm/lubbock/lubbock_var.h>

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
pv_addr_t minidataclean;

vm_offset_t msgbufphys;

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

#define KERNEL_PT_SYS		0	/* Page table for mapping proc0 zero page */
#define KERNEL_PT_KERNEL	1	/* Page table for mapping kernel */
#define	KERNEL_PT_KERNEL_NUM	4
#define KERNEL_PT_VMDATA	(KERNEL_PT_KERNEL+KERNEL_PT_KERNEL_NUM)
				        /* Page tables for mapping kernel VM */
#define	KERNEL_PT_VMDATA_NUM	4	/* start with 16MB of KVM */
#define NUM_KERNEL_PTS		(KERNEL_PT_VMDATA + KERNEL_PT_VMDATA_NUM)

pv_addr_t kernel_pt_table[NUM_KERNEL_PTS];

struct user *proc0paddr;

/* Prototypes */

#if 0
void	process_kernel_args(char *);
void	parse_mi_bootargs(char *args);
#endif

void	consinit(void);
void	kgdb_port_init(void);
void	change_clock(uint32_t v);

bs_protos(bs_notimpl);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifndef CONSPEED
#define CONSPEED B115200	/* What RedBoot uses */
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
	/*NOTREACHED*/
}

static inline
pd_entry_t *
read_ttb(void)
{
  long ttb;

  __asm volatile("mrc	p15, 0, %0, c2, c0, 0" : "=r" (ttb));


  return (pd_entry_t *)(ttb & ~((1<<14)-1));
}

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

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap lubbock_devmap[] = {
    {
	    LUBBOCK_OBIO_VBASE,
	    _A(LUBBOCK_OBIO_PBASE),
	    _S(LUBBOCK_OBIO_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    LUBBOCK_GPIO_VBASE,
	    _A(PXA2X0_GPIO_BASE),
	    _S(PXA250_GPIO_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    LUBBOCK_CLKMAN_VBASE,
	    _A(PXA2X0_CLKMAN_BASE),
	    _S(PXA2X0_CLKMAN_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    LUBBOCK_INTCTL_VBASE,
	    _A(PXA2X0_INTCTL_BASE),
	    _S(PXA2X0_INTCTL_SIZE),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    LUBBOCK_FFUART_VBASE,
	    _A(PXA2X0_FFUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },
    {
	    LUBBOCK_BTUART_VBASE,
	    _A(PXA2X0_BTUART_BASE),
	    _S(4 * COM_NPORTS),
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE,
    },

    {0, 0, 0, 0,}
};

#undef	_A
#undef	_S

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
	extern vaddr_t xscale_cache_clean_addr;
	int loop;
	int loop1;
	u_int l1pagetable;
	pv_addr_t kernel_l1pt;
	paddr_t memstart;
	psize_t memsize;
	int led_data = 0;
#ifdef DIAGNOSTIC
	extern vsize_t xscale_minidata_clean_size; /* used in KASSERT */
#endif
#define LEDSTEP_P() 	ioreg_write(LUBBOCK_OBIO_PBASE+LUBBOCK_HEXLED, led_data++)
#define LEDSTEP() hex_led(led_data++)

	/* use physical address until pagetable is set */
	LEDSTEP_P();

	/* map some peripheral registers at static I/O area */
	pmap_devmap_bootstrap((vaddr_t)read_ttb(), lubbock_devmap);

	LEDSTEP_P();

	/* start 32.768KHz OSC */
	ioreg_write(LUBBOCK_CLKMAN_VBASE + 0x08, 2);
	/* Get ready for splfoo() */
	pxa2x0_intr_bootstrap(LUBBOCK_INTCTL_VBASE);

	LEDSTEP();

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	LEDSTEP();


#if 0
	/* Calibrate the delay loop. */
#endif

	/*
	 * Okay, RedBoot has provided us with the following memory map:
	 *
	 * Physical Address Range     Description 
	 * -----------------------    ---------------------------------- 
	 * 0x00000000 - 0x01ffffff    flash Memory   (32MB)
	 * 0x04000000 - 0x05ffffff    Application flash Memory  (32MB)
	 * 0x08000000 - 0x080000ff    I/O baseboard registers
	 * 0x0a000000 - 0x0a0fffff    SRAM (1MB)
	 * 0x0c000000 - 0x0c0fffff    Ethernet Controller
	 * 0x0e000000 - 0x0e0fffff    Ethernet Controller (Attribute)
	 * 0x10000000 - 0x103fffff    SA-1111 Companion Chip
	 * 0x14000000 - 0x17ffffff    Expansion Card (64MB)
	 * 0x40000000 - 0x480fffff    Processor Registers
	 * 0xa0000000 - 0xa3ffffff    SDRAM Bank 0 (64MB)
	 *
	 *
	 * Virtual Address Range    X C B  Description 
	 * -----------------------  - - -  ---------------------------------- 
	 * 0x00000000 - 0x00003fff  N Y Y  SDRAM 
	 * 0x00004000 - 0x000fffff  N Y N  Boot ROM
	 * 0x00100000 - 0x01ffffff  N N N  Application Flash
	 * 0x04000000 - 0x05ffffff  N N N  Exp Application Flash
	 * 0x08000000 - 0x080fffff  N N N  I/O baseboard registers
	 * 0x0a000000 - 0x0a0fffff  N N N  SRAM
	 * 0x40000000 - 0x480fffff  N N N  Processor Registers
	 * 0xa0000000 - 0xa000ffff  N Y N  RedBoot SDRAM 
	 * 0xa0017000 - 0xa3ffffff  Y Y Y  SDRAM
	 * 0xc0000000 - 0xcfffffff  Y Y Y  Cache Flush Region 
	 * (done by this routine)
	 * 0xfd000000 - 0xfd0000ff  N N N  I/O baseboard registers
	 * 0xfd100000 - 0xfd3fffff  N N N  Processor Registers.
	 * 0xfd400000 - 0xfd4fffff  N N N  FF-UART
	 * 0xfd500000 - 0xfd5fffff  N N N  BT-UART
	 *
	 * RedBoot's first level page table is at 0xa0004000.  There
	 * are also 2 second-level tables at 0xa0008000 and
	 * 0xa0008400.  We will continue to use them until we switch to
	 * our pagetable by setttb().
	 *
	 */

	/* setup GPIO for BTUART, in case bootloader doesn't take care of it */
	pxa2x0_gpio_bootstrap(LUBBOCK_GPIO_VBASE);
	pxa2x0_gpio_set_function(42, GPIO_ALT_FN_1_IN);
	pxa2x0_gpio_set_function(43, GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(44, GPIO_ALT_FN_1_IN);
	pxa2x0_gpio_set_function(45, GPIO_ALT_FN_2_OUT);

	/* turn on clock to UART block.
	   XXX: this should not be done here. */
	ioreg_write(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN, CKEN_FFUART|CKEN_BTUART |
	    ioreg_read(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN));

	LEDSTEP();

	consinit();
	LEDSTEP();
#ifdef KGDB
	kgdb_port_init();
	LEDSTEP();
#endif


	/* Talk to the user */
	printf("\nNetBSD/evbarm (lubbock) booting ...\n");

	/* Tweak memory controller */
	{
		/* Modify access timing for CS3 (91c96) */

		uint32_t tmp = 
			ioreg_read(PXA2X0_MEMCTL_BASE+MEMCTL_MSC1);
		ioreg_write(PXA2X0_MEMCTL_BASE+MEMCTL_MSC1,
			     (tmp & 0xffff) | (0x3881<<16));
		/* RRR=3, RDN=8, RDF=8
		 * XXX: can be faster?
		 */
	}


	/* Initialize for PCMCIA/CF sockets */
	{
		uint32_t tmp;

		/* Activate two sockets.
		   XXX: This code segment should be moved to
		        pcmcia MD attach routine.
		   XXX: These bits should be toggled based on
		        existene of PCMCIA/CF cards
		*/
		ioreg_write(PXA2X0_MEMCTL_BASE+MEMCTL_MECR,
			     MECR_NOS|MECR_CIT);

		tmp = ioreg_read(LUBBOCK_SACC_PBASE+SACCSBI_SKCR);
		ioreg_write(LUBBOCK_SACC_PBASE+SACCSBI_SKCR,
			     (tmp & ~(1<<4)) | (1<<0));
	}

#if 0
	/*
	 * Examine the boot args string for options we need to know about
	 * now.
	 */
	process_kernel_args((char *)nwbootinfo.bt_args);
#endif

	{
		int processor_card_id;

		processor_card_id = 0x000f & 
			ioreg_read(LUBBOCK_OBIO_VBASE+LUBBOCK_MISCRD);
		switch(processor_card_id){
		case 0:
			/* Cotulla */
			memstart = 0xa0000000;
			memsize =  0x04000000; /* 64MB */
			break;
		case 1:
			/* XXX: Sabiani */
			memstart = 0xa0000000;
			memsize = 0x04000000; /* 64MB */
			break;
		default:
			/* XXX: Unknown  */
			memstart = 0xa0000000;
			memsize = 0x04000000; /* 64MB */
		}
	}

	printf("initarm: Configuring system ...\n");

	/* Fake bootconfig structure for the benefit of pmap.c */
	/* XXX must make the memory description h/w independant */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memstart;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;

	/*
	 * Set up the variables that define the availablilty of
	 * physical memory.  For now, we're going to set
	 * physical_freestart to 0xa0200000 (where the kernel
	 * was loaded), and allocate the memory we need downwards.
	 * If we get too close to the page tables that RedBoot
	 * set up, we will panic.  We will update physical_freestart
	 * and physical_freeend later to reflect what pmap_bootstrap()
	 * wants to see.
	 *
	 * XXX pmap_bootstrap() needs an enema.
	 */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + (bootconfig.dram[0].pages * PAGE_SIZE);

	physical_freestart = 0xa0009000UL;
	physical_freeend = 0xa0200000UL;

	physmem = (physical_end - physical_start) / PAGE_SIZE;

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
		panic("initarm: Failed to align the kernel page directory");

	LEDSTEP();

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

	/* Allocate enough pages for cleaning the Mini-Data cache. */
	KASSERT(xscale_minidata_clean_size <= PAGE_SIZE);
	valloc_pages(minidataclean, 1);

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

	/*
	 * XXX Defer this to later so that we can reclaim the memory
	 * XXX used by the RedBoot page tables.
	 */
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
		extern char etext[], _end[];
		size_t textsize = (uintptr_t) etext - KERNEL_TEXT_BASE;
		size_t totalsize = (uintptr_t) _end - KERNEL_TEXT_BASE;
		u_int logical;

		textsize = (textsize + PGOFSET) & ~PGOFSET;
		totalsize = (totalsize + PGOFSET) & ~PGOFSET;
		
		logical = 0x00200000;	/* offset of kernel in RAM */

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
	    UPAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_CACHE);

	pmap_map_chunk(l1pagetable, kernel_l1pt.pv_va, kernel_l1pt.pv_pa,
	    L1_TABLE_SIZE, VM_PROT_READ | VM_PROT_WRITE, PTE_PAGETABLE);

	for (loop = 0; loop < NUM_KERNEL_PTS; ++loop) {
		pmap_map_chunk(l1pagetable, kernel_pt_table[loop].pv_va,
		    kernel_pt_table[loop].pv_pa, L2_TABLE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_PAGETABLE);
	}

	/* Map the Mini-Data cache clean area. */
	xscale_setup_minidata(l1pagetable, minidataclean.pv_va,
	    minidataclean.pv_pa);

	/* Map the vector page. */
#if 1
	/* MULTI-ICE requires that page 0 is NC/NB so that it can download the
	 * cache-clean code there.  */
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);
#else
	pmap_map_entry(l1pagetable, vector_page, systempage.pv_pa,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_CACHE);
#endif

	/*
	 * map integrated peripherals at same address in l1pagetable
	 * so that we can continue to use console.
	 */
	pmap_devmap_bootstrap(l1pagetable, lubbock_devmap);

	/*
	 * Give the XScale global cache clean code an appropriately
	 * sized chunk of unmapped VA space starting at 0xff000000
	 * (our device mappings end before this address).
	 */
	xscale_cache_clean_addr = 0xff000000U;

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
		extern char _end[];

		physical_freestart = physical_start +
		    (((((uintptr_t) _end) + PGOFSET) & ~PGOFSET) -
		     KERNEL_BASE);
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
	LEDSTEP();

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init
	 */
	proc0paddr = (struct user *)kernelstack.pv_va;
	lwp0.l_addr = proc0paddr;

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

	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

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
	uvm_setpagesize();        /* initialize PAGE_SIZE-dependent variables */
	uvm_page_physload(atop(physical_freestart), atop(physical_freeend),
	    atop(physical_freestart), atop(physical_freeend),
	    VM_FREELIST_DEFAULT);

	/* Boot strap pmap telling it where the kernel page table is */
	printf("pmap ");
	LEDSTEP();
	pmap_bootstrap((pd_entry_t *)kernel_l1pt.pv_va, KERNEL_VM_BASE,
	    KERNEL_VM_BASE + KERNEL_VM_SIZE);
	LEDSTEP();

#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

	{
		uint16_t sw = ioreg16_read(LUBBOCK_OBIO_VBASE+LUBBOCK_USERSW);

		if (0 == (sw & (1<<13))) /* check S19 */
			boothowto |= RB_KDB;
		if (0 == (sw & (1<<12))) /* S20 */
			boothowto |= RB_SINGLE;
	}

	LEDSTEP();

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

#ifdef DDB
	db_machine_init();

	/* Firmware doesn't load symbols. */
	ddb_init(0, NULL, NULL);

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* We return the new stack pointer address */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}

#if 0
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
#endif

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "ffuart"
#endif
const char kgdb_devname[] = KGDB_DEVNAME;

#if (NCOM > 0)
#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comkgdbmode = KGDB_DEVMODE;
#endif /* NCOM */

#endif /* KGDB */


void
consinit(void)
{
	static int consinit_called = 0;
	uint32_t ckenreg = ioreg_read(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN);
#if 0
	char *console = CONSDEVNAME;
#endif

	if (consinit_called != 0)
		return;

	consinit_called = 1;

#if NCOM > 0

#ifdef FFUARTCONSOLE
	/* Check switch. */
	if (0 == (ioreg_read(LUBBOCK_OBIO_VBASE+LUBBOCK_USERSW) & (1<<15))) {
		/* We don't use FF serial when S17=no-dot position */
	}
#ifdef KGDB
	else if (0 == strcmp(kgdb_devname, "ffuart")) {
		/* port is reserved for kgdb */
	}
#endif
	else if (0 == comcnattach(&pxa2x0_a4x_bs_tag, PXA2X0_FFUART_BASE, 
		     comcnspeed, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode)) {
#if 0
		/* XXX: can't call pxa2x0_clkman_config yet */
		pxa2x0_clkman_config(CKEN_FFUART, 1);
#else
		ioreg_write(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN,
		    ckenreg|CKEN_FFUART);
#endif

		return;
	}
#endif /* FFUARTCONSOLE */

#ifdef BTUARTCONSOLE
#ifdef KGDB
	if (0 == strcmp(kgdb_devname, "btuart")) {
		/* port is reserved for kgdb */
	} else
#endif
	if (0 == comcnattach(&pxa2x0_a4x_bs_tag, PXA2X0_BTUART_BASE,
		comcnspeed, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode)) {
		ioreg_write(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN,
		    ckenreg|CKEN_BTUART);
		return;
	}
#endif /* BTUARTCONSOLE */


#endif /* NCOM */

}

#ifdef KGDB
void
kgdb_port_init(void)
{
#if (NCOM > 0) && defined(COM_PXA2X0)
	paddr_t paddr = 0;
	uint32_t ckenreg = ioreg_read(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN);

	if (0 == strcmp(kgdb_devname, "ffuart")) {
		paddr = PXA2X0_FFUART_BASE;
		ckenreg |= CKEN_FFUART;
	}
	else if (0 == strcmp(kgdb_devname, "btuart")) {
		paddr = PXA2X0_BTUART_BASE;
		ckenreg |= CKEN_BTUART;
	}

	if (paddr &&
	    0 == com_kgdb_attach(&pxa2x0_a4x_bs_tag, paddr,
		kgdb_rate, PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comkgdbmode)) {

		ioreg_write(LUBBOCK_CLKMAN_VBASE+CLKMAN_CKEN, ckenreg);
	}
#endif
}
#endif

#if 0
/*
 * display a number in hex LED.
 * a digit is blank when the corresponding bit in arg blank is 1
 */
unsigned short led_control_value = 0;

void
hex_led_blank(uint32_t value, int blank)
{
	int save = disable_interrupts(I32_bit);

	ioreg_write(LUBBOCK_OBIO_VBASE+0x10, value);
	led_control_value = (led_control_value & 0xff)
		| ((blank & 0xff)<<8);
	ioreg_write(LUBBOCK_OBIO_VBASE+0x40, led_control_value);
	restore_interrupts(save);
}
#endif
