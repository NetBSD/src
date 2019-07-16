/*	$NetBSD: imx7_machdep.c,v 1.11 2019/07/16 14:41:46 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx7_machdep.c,v 1.11 2019/07/16 14:41:46 skrll Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_arm_debug.h"
#include "opt_console.h"
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

#include <arm/imx/imx7var.h>
#include <arm/imx/imx7_srcreg.h>
#include <arm/imx/imxuartvar.h>
#include <arm/imx/imxuartreg.h>

#include <evbarm/imx7/platform.h>

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

#ifndef CONADDR
#define CONADDR	(IMX7_AIPS_BASE + AIPS3_UART1_BASE)
#endif
#ifndef CONSPEED
#define CONSPEED B115200
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

void imx7_setup_iomux(void);
void imx7_setup_gpio(void);
void imx7board_device_register(device_t, void *);
void imx7_mpstart(void);
void imx7_platform_early_putchar(char);

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
		IMX7_IOREG_PBASE,
		IMX7_IOREG_SIZE,
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{
		KERNEL_IO_ARMCORE_VBASE,
		IMX7_ARMCORE_PBASE,
		IMX7_ARMCORE_SIZE,
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE,
	},
	{ 0, 0, 0, 0, 0 }
};

#ifdef PMAP_NEED_ALLOC_POOLPAGE
static struct boot_physmem bp_highgig = {
	.bp_start = IMX7_MEM_BASE / NBPG,
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0,
};
#endif

void
imx7_platform_early_putchar(char c)
{
#define CONADDR_VA (CONADDR - IMX7_IOREG_PBASE + KERNEL_IO_IOREG_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONADDR_VA :
	    (volatile uint32_t *)CONADDR;

	int timo = 150000;

	while ((uartaddr[IMX_USR2 / 4] & IMX_USR2_TXDC) == 0) {
		if (--timo == 0)
			break;
	}

	uartaddr[IMX_UTXD / 4] = c;

	timo = 150000;
	while ((uartaddr[IMX_USR2 / 4] & IMX_USR2_TXDC) == 0) {
		if (--timo == 0)
			break;
	}
}

void
imx7_mpstart(void)
{
#if defined(MULTIPROCESSOR)
	if (arm_cpu_max <= 1)
		return;

	const bus_space_tag_t bst = &armv7_generic_bs_tag;

	const bus_space_handle_t bsh = KERNEL_IO_IOREG_VBASE + AIPS1_SRC_BASE;
	const paddr_t mpstart = KERN_VTOPHYS((vaddr_t)cpu_mpstart);

	bus_space_write_4(bst, bsh, SRC_GPR3, mpstart);

	uint32_t rcr1 = bus_space_read_4(bst, bsh, SRC_A7RCR1);
	rcr1 |= SRC_A7RCR1_A7_CORE1_ENABLE;
	bus_space_write_4(bst, bsh, SRC_A7RCR1, rcr1);

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
#endif
}


/*
 * vaddr_t initarm(...)
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
vaddr_t
initarm(void *arg)
{
	psize_t memsize;

	/*
	 * Heads up ... Setup the CPU / MMU / TLB functions
	 */
	if (set_cpufuncs())		// starts PMC counter
		panic("cpu not recognized!");

	cn_tab = &earlycons;

	extern char ARM_BOOTSTRAP_LxPT[];
	pmap_devmap_bootstrap((vaddr_t)ARM_BOOTSTRAP_LxPT, devmap);

	imx7_bootstrap(KERNEL_IO_IOREG_VBASE);

	imx7_setup_iomux();
	imx7_setup_gpio();

	consinit();

	cpu_domains((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT);

#ifdef NO_POWERSAVE
	cpu_do_powersave = 0;
#endif

	cortex_pmc_ccnt_init();

	printf("\nuboot arg = %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = imx7_reset;

	/* Talk to the user */
	printf("\nNetBSD/evbarm (" ___STRING(EVBARM_BOARDTYPE)
	    ", digprog=0x%08x) booting ...\n", imx7_chip_id());

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
/*
 * Ugh... but this is better than proceeding using an uninitialized
 * value in memsize, as the code did before I added this branch...
 */
#error "MEMSIZE not set"
#endif

	bootconfig.dramblocks = 1;
#ifdef MEMSIZE_RESERVED
	/* reserved for Cortex-M4 core */
	memsize -= MEMSIZE_RESERVED * 1024 * 1024;
	bootconfig.dram[0].address = KERN_VTOPHYS(KERNEL_BASE) +
	    MEMSIZE_RESERVED * 1024 * 1024;
#else
	bootconfig.dram[0].address = KERN_VTOPHYS(KERNEL_BASE);
#endif
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
	evbarm_device_register = imx7board_device_register;

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
	vaddr_t sp = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

	/*
	 * initarm_common flushes cache if required before AP start
	 */
	imx7_mpstart();

	return sp;
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
#define IMXUART_FREQ	24000000
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
		imxuart_cnattach(&armv7_generic_bs_tag, consaddr, consrate, consmode);
		return;
	}
# endif /* (NIMXUART > 0) && defined(IMXUARTCONSOLE) */
#endif /* CONSDEVNAME */
}
