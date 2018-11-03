/*	$NetBSD: zynq_machdep.c,v 1.6 2018/11/03 15:02:32 skrll Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq_machdep.c,v 1.6 2018/11/03 15:02:32 skrll Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_arm_debug.h"
#include "opt_console.h"
#include "opt_kgdb.h"
#include "com.h"
#include "opt_zynq.h"
#include "opt_machdep.h"

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
#include <arm/arm32/machdep.h>

#include <machine/autoconf.h>
#include <machine/bootconfig.h>

#include <arm/cortex/scu_reg.h>
#include <arm/cortex/a9tmr_var.h>

#include <arm/zynq/zynq7000_var.h>
#include <arm/zynq/zynq_uartreg.h>
#include <arm/zynq/zynq_uartvar.h>

#include <evbarm/zynq/platform.h>

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

extern int _end[];
extern int KERNEL_BASE_phys[];
extern int KERNEL_BASE_virt[];

BootConfig bootconfig;
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

/* filled in before cleaning bss. keep in .data */
u_int uboot_args[4] __attribute__((__section__(".data")));

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#ifndef CONADDR
#define CONADDR	(UART1_BASE)
#endif
#ifndef CONSPEED
#define CONSPEED B115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

static const bus_addr_t comcnaddr = (bus_addr_t)CONADDR;
static const int comcnspeed = CONSPEED;
static const int comcnmode = CONMODE | CLOCAL;

void zynq_platform_early_putchar(char);

#ifdef KGDB
#include <sys/kgdb.h>
#endif

static void
earlyconsputc(dev_t dev, int c)
{
	uartputc(c);
}

static int
earlyconsgetc(dev_t dev)
{
	return 0;
}

static struct consdev earlycons = {
	.cn_putc = earlyconsputc,
	.cn_getc = earlyconsgetc,
	.cn_pollc = nullcnpollc,
};

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
		ZYNQ7000_IOREG_PBASE,		/* 0xe0000000 */
		ZYNQ7000_IOREG_SIZE,		/* 2MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_ARMCORE_VBASE,
		ZYNQ7000_ARMCORE_PBASE,		/* 0xf8f00000 */
		ZYNQ7000_ARMCORE_SIZE,		/* 1MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_OCM_VBASE,
		ZYNQ7000_OCM_PBASE,		/* 0xfff00000 */
		ZYNQ7000_OCM_SIZE,		/* 1MB */
		VM_PROT_READ|VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{ 0, 0, 0, 0, 0 }
};

void
zynq_platform_early_putchar(char c)
{
#define CONADDR_VA (CONADDR - ZYNQ7000_IOREG_PBASE + KERNEL_IO_IOREG_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONADDR_VA :
	    (volatile uint32_t *)CONADDR;

	int timo = 150000;

	while ((uartaddr[UART_CHANNEL_STS / 4] & STS_TEMPTY) == 0) {
		if (--timo == 0)
			break;
	}

	uartaddr[UART_TX_RX_FIFO / 4] = c;

	timo = 150000;
	while ((uartaddr[UART_CHANNEL_STS / 4] & STS_TEMPTY) == 0) {
		if (--timo == 0)
			break;
	}
}

static void
zynq_mpstart(void)
{
#ifdef MULTIPROCESSOR
	/*
	 * Invalidate all SCU cache tags. That is, for all cores (0-3)
	 */
	bus_space_write_4(zynq7000_armcore_bst, zynq7000_armcore_bsh,
	    ARMCORE_SCU_BASE + SCU_INV_ALL_REG, 0xffff);

	uint32_t scu_ctl = bus_space_read_4(zynq7000_armcore_bst,
	    zynq7000_armcore_bsh, ARMCORE_SCU_BASE + SCU_CTL);
	scu_ctl |= SCU_CTL_SCU_ENA;
	bus_space_write_4(zynq7000_armcore_bst, zynq7000_armcore_bsh,
	    ARMCORE_SCU_BASE + SCU_CTL, scu_ctl);

	armv7_dcache_wbinv_all();

	bus_space_tag_t bst = &zynq_bs_tag;
	bus_space_handle_t bsh;
	int error = bus_space_map(bst, ZYNQ7000_CPU1_ENTRY,
	    ZYNQ7000_CPU1_ENTRY_SZ, 0, &bsh);
	if (error)
		panic("%s: Couldn't map OCM", __func__);

	/* Write start address for CPU1. */
	bus_space_write_4(bst, bsh, 0, KERN_VTOPHYS((vaddr_t)cpu_mpstart));

	bus_space_unmap(bst, bsh, ZYNQ7000_CPU1_ENTRY_SZ);

	arm_dsb();
	__asm __volatile("sev" ::: "memory");


	for (int loop = 0; loop < 16; loop++) {
		VPRINTF("%u hatched %#x\n", loop, arm_cpu_hatched);
		if (arm_cpu_hatched == __BITS(arm_cpu_max - 1, 1))
			break;
		int timo = 1500000;
		while (arm_cpu_hatched != __BITS(arm_cpu_max - 1, 1))
			if (--timo == 0)
				break;
	}
	for (size_t i = 1; i < arm_cpu_max; i++) {
		if ((arm_cpu_hatched & __BIT(i)) == 0) {
		printf("%s: warning: cpu%zu failed to hatch\n",
			    __func__, i);
		}
	}

	VPRINTF(" (%u cpu%s, hatched %#x)",
	    arm_cpu_max, arm_cpu_max ? "s" : "",
	    arm_cpu_hatched);
#endif /* MULTIPROCESSOR */
}


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
	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())		// starts PMC counter
		panic("cpu not recognized!");

	cn_tab = &earlycons;

	extern char ARM_BOOTSTRAP_LxPT[];
	pmap_devmap_bootstrap((vaddr_t)ARM_BOOTSTRAP_LxPT, devmap);

	zynq7000_bootstrap(KERNEL_IO_IOREG_VBASE);

#ifdef MULTIPROCESSOR
	uint32_t scu_cfg = bus_space_read_4(zynq7000_armcore_bst,
	    zynq7000_armcore_bsh, ARMCORE_SCU_BASE + SCU_CFG);
	arm_cpu_max = (scu_cfg & SCU_CFG_CPUMAX) + 1;
	membar_producer();
#endif /* MULTIPROCESSOR */
	consinit();

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

#ifdef	NO_POWERSAVE
	cpu_do_powersave = 0;
#endif

	cortex_pmc_ccnt_init();

	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	/* Talk to the user */
	printf("\nNetBSD/evbarm (" ___STRING(EVBARM_BOARDTYPE) ") booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif /* BOOT_ARGS */
	bootargs[0] = '\0';

	VPRINTF("initarm: Configuring system");
	VPRINTF(", CLIDR=%010o CTR=%#x",
	    armreg_clidr_read(), armreg_ctr_read());
	VPRINTF("\n");

	psize_t memsize = zynq7000_memprobe();
#ifdef MEMSIZE
	if ((memsize >> 20) > MEMSIZE)
		memsize = MEMSIZE*1024*1024;
#endif

	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = ZYNQ7000_DDR_PBASE;
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;

	arm32_bootmem_init(bootconfig.dram[0].address,
	    bootconfig.dram[0].pages * PAGE_SIZE, (paddr_t)KERNEL_BASE_phys);

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
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap, false);

	/* we've a specific device_register routine */
	evbarm_device_register = zynq7000_device_register;

	u_int sp = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

	VPRINTF("mpstart\n");
	zynq_mpstart();

	return sp;
}

void
consinit(void)
{
	static bool consinit_called = false;

	if (consinit_called)
		return;
	consinit_called = true;

	zynquart_cons_attach(&zynq_bs_tag, comcnaddr, comcnspeed, comcnmode);
}
