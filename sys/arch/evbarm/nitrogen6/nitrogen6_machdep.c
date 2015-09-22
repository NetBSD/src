/*	$NetBSD: nitrogen6_machdep.c,v 1.1.2.2 2015/09/22 12:05:40 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nitrogen6_machdep.c,v 1.1.2.2 2015/09/22 12:05:40 skrll Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_arm_debug.h"
#include "opt_kgdb.h"
#include "com.h"
#include "opt_machdep.h"
#include "opt_imxuart.h"
#include "imxuart.h"
#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/termios.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <arm/db_machdep.h>
#include <arm/arm32/machdep.h>

#include <machine/autoconf.h>
#include <machine/bootconfig.h>

#include <arm/cortex/scu_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imxuartvar.h>

#include <evbarm/nitrogen6/platform.h>

extern int _end[];
extern int KERNEL_BASE_phys[];
extern int KERNEL_BASE_virt[];

BootConfig bootconfig;
static char bootargs[MAX_BOOT_STRING];
char *boot_args = NULL;

u_int uboot_args[4] = { 0 };

/*
 * Macros to translate between physical and virtual for a subset of the
 * kernel address space.  *Not* for general use.
 */
#define KERN_VTOPDIFF	((vaddr_t)KERNEL_BASE_phys - (vaddr_t)KERNEL_BASE_virt)
#define KERN_VTOPHYS(va) ((paddr_t)((vaddr_t)va + (vaddr_t)KERN_VTOPDIFF))
#define KERN_PHYSTOV(pa) ((vaddr_t)((paddr_t)pa - (vaddr_t)KERN_VTOPDIFF))

#ifndef CONADDR
#define CONADDR	(IMX6_AIPS2_BASE + AIPS2_UART1_BASE)
#endif
#ifndef CONSPEED
#define CONSPEED B115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

void nitrogen6_setup_iomux(void);

static const bus_addr_t comcnaddr = (bus_addr_t)CONADDR;
static const int comcnspeed = CONSPEED;
static const int comcnmode = CONMODE | CLOCAL;

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
		IMX6_IOREG_PBASE,		/* 0x02000000 */
		IMX6_IOREG_SIZE,
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_ARMCORE_VBASE,
		IMX6_ARMCORE_PBASE,		/* 0x00a00000 */
		IMX6_ARMCORE_SIZE,
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{ 0, 0, 0, 0, 0 }
};

#ifdef PMAP_NEED_ALLOC_POOLPAGE
static struct boot_physmem bp_highgig = {
	.bp_start = IMX6_MEM_BASE / NBPG,
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0,
};
#endif

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
	psize_t memsize;

	pmap_devmap_register(devmap);
	imx6_bootstrap(KERNEL_IO_IOREG_VBASE);

#ifdef MULTIPROCESSOR
	uint32_t scu_cfg = bus_space_read_4(imx6_armcore_bst, imx6_armcore_bsh,
	    ARMCORE_SCU_BASE + SCU_CFG);
	arm_cpu_max = (scu_cfg & SCU_CFG_CPUMAX) + 1;
	membar_producer();
#endif /* MULTIPROCESSOR */

	nitrogen6_setup_iomux();

	consinit();

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())		// starts PMC counter
		panic("cpu not recognized!");

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

#ifdef NO_POWERSAVE
	cpu_do_powersave = 0;
#endif

	cortex_pmc_ccnt_init();

	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = imx6_reset;

	/* Talk to the user */
	printf("\nNetBSD/evbarm (" ___STRING(EVBARM_BOARDTYPE) ") booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif /* BOOT_ARGS */
	bootargs[0] = '\0';

#ifdef VERBOSE_INIT_ARM
	printf("initarm: Configuring system");
#ifdef MULTIPROCESSOR
	printf(" (%u cpu%s, hatched %#x)",
	    arm_cpu_max, arm_cpu_max ? "s" : "",
	    arm_cpu_hatched);
#endif /* MULTIPROCESSOR */
	printf(", CLIDR=%010o CTR=%#x",
	    armreg_clidr_read(), armreg_ctr_read());
	printf("\n");
#endif /* VERBOSE_INIT_ARM */


#ifdef MEMSIZE
	memsize = MEMSIZE * 1024 * 1024;
#else
	memsize = imx6_memprobe();
#endif

	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = KERN_VTOPHYS(KERNEL_BASE);
	bootconfig.dram[0].pages = memsize / PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (memsize > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		   __func__, (unsigned long) (memsize >> 20),
		   (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		memsize = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else /* !__HAVE_MM_MD_DIRECT_MAPPED_PHYS */
	const bool mapallmem_p = false;
#endif /* __HAVE_MM_MD_DIRECT_MAPPED_PHYS */

	arm32_bootmem_init(bootconfig.dram[0].address,
	    memsize, (paddr_t)KERNEL_BASE_phys);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, devmap,
	    mapallmem_p);

	/* we've a specific device_register routine */
	evbarm_device_register = imx6_device_register;

#ifdef PMAP_NEED_ALLOC_POOLPAGE
	/*
	 * If we couldn't map all of memory via TTBR1, limit the memory the
	 * kernel can allocate from to be from the highest available 1GB.
	 */
	if (atop(memsize) > bp_highgig.bp_pages) {
		bp_highgig.bp_start += atop(memsize) - bp_highgig.bp_pages;
		arm_poolpage_vmfreelist = bp_highgig.bp_freelist;
		return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
		    &bp_highgig, 1);
	}
#endif
	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}

#ifdef CONSDEVNAME
const char consdevname[] = CONSDEVNAME;

#ifndef CONMODE
#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#ifndef CONSPEED
#define CONSPEED	115200
#endif

int consmode = CONMODE;
int consrate = CONSPEED;

#endif /* CONSDEVNAME */

#ifndef IMXUART_FREQ
#define IMXUART_FREQ	80000000
#endif

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called)
		return;

	consinit_called = 1;

#ifdef CONSDEVNAME
# if NIMXUART > 0
	imxuart_set_frequency(IMXUART_FREQ, 2);
# endif
# if (NIMXUART > 0) && defined(IMXUARTCONSOLE)
	if (strcmp(consdevname, CONSDEVNAME) == 0) {
		paddr_t consaddr;

		consaddr = CONADDR;
		imxuart_cons_attach(&imx_bs_tag, consaddr, consrate, consmode);
		return;
	}
# endif /* (NIMXUART > 0) && defined(IMXUARTCONSOLE) */
#endif /* CONSDEVNAME */
}
