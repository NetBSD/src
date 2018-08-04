/*	$NetBSD: bcm53xx_machdep.c,v 1.12 2018/08/04 13:27:03 kre Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define CCA_PRIVATE
#define IDM_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm53xx_machdep.c,v 1.12 2018/08/04 13:27:03 kre Exp $");

#include "opt_arm_debug.h"
#include "opt_evbarm_boardtype.h"
#include "opt_broadcom.h"
#include "opt_kgdb.h"
#include "com.h"
#include "pci.h"
#include "bcmrng_ccb.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/autoconf.h>
#include <machine/bootconfig.h>

#define CCA_PRIVATE

#include <arm/cortex/scu_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

#include <evbarm/bcm53xx/platform.h>

#if NCOM == 0
#error missing COM device for console
#endif

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

extern int _end[];
extern int KERNEL_BASE_phys[];
extern int KERNEL_BASE_virt[];

BootConfig bootconfig;
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;     

/* filled in before cleaning bss. keep in .data */
u_int uboot_args[4] __attribute__((__section__(".data")));

static void bcm53xx_system_reset(void);

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#if !defined(KERN_VTOPHYS) || !defined(KERN_PHYSTOV)
#define	KERN_VTOPDIFF	((vaddr_t)KERNEL_BASE_phys - (vaddr_t)KERNEL_BASE_virt)
#endif
#ifndef KERN_VTOPHYS
#define KERN_VTOPHYS(va) ((paddr_t)((vaddr_t)va + KERN_VTOPDIFF))
#endif
#ifndef KERN_PHYSTOV
#define KERN_PHYSTOV(pa) ((vaddr_t)((paddr_t)pa - KERN_VTOPDIFF))
#endif

#ifndef CONADDR
#define CONADDR		(BCM53XX_IOREG_PBASE + CCA_UART0_BASE)
#endif
#ifndef CONSPEED
#define CONSPEED B115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

#if (NCOM > 0)
static const bus_addr_t comcnaddr = (bus_addr_t)CONADDR;

int comcnspeed = CONSPEED;
int comcnmode = CONMODE | CLOCAL;
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

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

static const struct pmap_devmap devmap[] = {
	{
		KERNEL_IO_IOREG_VBASE,
		BCM53XX_IOREG_PBASE,		/* 0x18000000 */
		BCM53XX_IOREG_SIZE,		/* 2MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_ARMCORE_VBASE,
		BCM53XX_ARMCORE_PBASE,		/* 0x19000000 */
		BCM53XX_ARMCORE_SIZE,		/* 1MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
#if NPCI > 0
	{
		KERNEL_IO_PCIE0_OWIN_VBASE,
		BCM53XX_PCIE0_OWIN_PBASE,	/* 0x08000000 */
		BCM53XX_PCIE0_OWIN_SIZE,	/* 4MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_PCIE1_OWIN_VBASE,
		BCM53XX_PCIE1_OWIN_PBASE,	/* 0x40000000 */
		BCM53XX_PCIE1_OWIN_SIZE,	/* 4MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_PCIE2_OWIN_VBASE,
		BCM53XX_PCIE2_OWIN_PBASE,	/* 0x48000000 */
		BCM53XX_PCIE2_OWIN_SIZE,	/* 4MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
#endif /* NPCI > 0 */
	{ 0, 0, 0, 0, 0 }
};

static const struct boot_physmem bp_first256 = {
	.bp_start = 0x80000000 / NBPG,
	.bp_pages = 0x10000000 / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0,
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
 */
u_int
initarm(void *arg)
{
	pmap_devmap_register(devmap);
	bcm53xx_bootstrap(KERNEL_IO_IOREG_VBASE);

#ifdef MULTIPROCESSOR
	uint32_t scu_cfg = bus_space_read_4(bcm53xx_armcore_bst, bcm53xx_armcore_bsh,
	    ARMCORE_SCU_BASE + SCU_CFG);
	arm_cpu_max = 1 + (scu_cfg & SCU_CFG_CPUMAX);
	membar_producer();
#endif
	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())		// starts PMC counter
		panic("cpu not recognized!");

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

	consinit();

	bcm53xx_cpu_softc_init(curcpu());
	bcm53xx_print_clocks();

#if NBCMRNG_CCB > 0
	/*
	 * Start this early since it takes a while to startup up.
	 */
	bcm53xx_rng_start(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh);
#endif

	printf("uboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);      

	/* Talk to the user */
	printf("\nNetBSD/evbarm (" ___STRING(EVBARM_BOARDTYPE) ") booting ...\n");

	bootargs[0] = '\0';

#if defined(VERBOSE_INIT_ARM) || 1
	printf("initarm: Configuring system");
#ifdef MULTIPROCESSOR
	printf(" (%u cpu%s, hatched %#x)",
	    arm_cpu_max + 1, arm_cpu_max + 1 ? "s" : "",
	    arm_cpu_hatched);
#endif
	printf(", CLIDR=%010o CTR=%#x PMUSERSR=%#x",
	    armreg_clidr_read(), armreg_ctr_read(), armreg_pmuserenr_read());
	printf("\n");
#endif

	psize_t memsize = bcm53xx_memprobe();
#ifdef MEMSIZE
	if ((memsize >> 20) > MEMSIZE)
		memsize = MEMSIZE*1024*1024;
#endif
	const bool bigmem_p = (memsize >> 20) > 256; 

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (memsize > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		   __func__, (unsigned long) (ram_size >> 20),
		   (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		memsize = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else
	const bool mapallmem_p = false;
#endif
	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);
	arm32_bootmem_init(KERN_VTOPHYS(KERNEL_BASE), memsize,
	    (paddr_t)KERNEL_BASE_phys);

	bcm53xx_dma_bootstrap(memsize);

	/*
	 * This is going to do all the hard work of setting up the first and
	 * and second level page tables.  Pages of memory will be allocated
	 * and mapped for other structures that are required for system
	 * operation.  When it returns, physical_freestart and free_pages will
	 * have been updated to reflect the allocations that were made.  In
	 * addition, kernel_l1pt, kernel_pt_table[], systempage, irqstack,
	 * abtstack, undstack, kernelstack, msgbufphys will be set to point to
	 * the memory that was allocated for them.
	 */
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap,
	    mapallmem_p);

	cpu_reset_address = bcm53xx_system_reset;
	/* we've a specific device_register routine */
	evbarm_device_register = bcm53xx_device_register;
	if (bigmem_p) {
		/*
		 * If we have more than 256MB
		 */
		arm_poolpage_vmfreelist = bp_first256.bp_freelist;
	}

	/*
	 * If we have more than 256MB of RAM, set aside the first 256MB for
	 * non-default VM allocations.
	 */
	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
	    (bigmem_p ? &bp_first256 : NULL), (bigmem_p ? 1 : 0));
}

void
consinit(void)
{
	static bool consinit_called = false;
	uint32_t v;
	if (consinit_called)
		return;

	consinit_called = true;

	/*
	 * Force UART clock to the reference clock
	 */
	v = bus_space_read_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
	    IDM_BASE + IDM_APBX_BASE + IDM_IO_CONTROL_DIRECT);
	v &= ~IO_CONTROL_DIRECT_UARTCLKSEL;
	bus_space_write_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
	    IDM_BASE + IDM_APBX_BASE + IDM_IO_CONTROL_DIRECT, v);

	/*
	 * Switch to the reference clock
	 */
	v = bus_space_read_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
	    CCA_MISC_BASE + MISC_CORECTL);
	v &= ~CORECTL_UART_CLK_OVERRIDE;
	bus_space_write_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
	    CCA_MISC_BASE + MISC_CORECTL, v);

        if (comcnattach(bcm53xx_ioreg_bst, comcnaddr, comcnspeed,
                        BCM53XX_REF_CLK, COM_TYPE_NORMAL, comcnmode))
                panic("Serial console can not be initialized.");
}

static void
bcm53xx_system_reset(void)
{
	bus_space_write_4(bcm53xx_ioreg_bst, bcm53xx_ioreg_bsh,
	    MISC_WATCHDOG, 1);
}
