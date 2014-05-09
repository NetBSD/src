/*	$NetBSD: odroid_machdep.c,v 1.10 2014/05/09 22:02:10 reinoud Exp $ */

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: odroid_machdep.c,v 1.10 2014/05/09 22:02:10 reinoud Exp $");

#include "opt_evbarm_boardtype.h"
#include "opt_exynos.h"
#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_md.h"
#include "opt_sscom.h"
#include "opt_arm_debug.h"

#include "ukbd.h"
#include "arml2cc.h"	// RPZ why is it not called opt_l2cc.h?

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/gpio.h>

#include <uvm/uvm_extern.h>

#include <sys/conf.h>
#include <dev/cons.h>
#include <dev/md.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/bootconfig.h>
#include <arm/armreg.h>
#include <arm/undefined.h>
#include <arm/cortex/pl310_var.h>

#include <arm/arm32/machdep.h>
#include <arm/mainbus/mainbus.h>

#include <arm/samsung/exynos4_reg.h>
#include <arm/samsung/exynos5_reg.h>
#include <arm/samsung/exynos_var.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/odroid/platform.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>

/* serial console stuff */
#include "sscom.h"
#include "opt_sscom.h"

#if defined(KGDB) || \
    defined(SSCOM0CONSOLE) || defined(SSCOM1CONSOLE) || \
    defined(SSCOM2CONSOLE) || defined(SSCOM3CONSOLE)
#if NSSCOM == 0
#error must define sscom for use with serial console or KGD
#endif

#include <arm/samsung/sscom_var.h>
#include <arm/samsung/sscom_reg.h>

static const struct sscom_uart_info exynos_uarts[] = {
#ifdef EXYNOS5
	{
		.unit    = 0,
		.iobase = EXYNOS5_UART0_OFFSET
	},
	{
		.unit    = 1,
		.iobase = EXYNOS5_UART1_OFFSET
	},
	{
		.unit    = 2,
		.iobase = EXYNOS5_UART2_OFFSET
	},
	{
		.unit    = 3,
		.iobase = EXYNOS5_UART3_OFFSET
	},
#endif
#ifdef EXYNOS4
	{
		.unit    = 0,
		.iobase = EXYNOS4_UART0_OFFSET
	},
	{
		.unit    = 1,
		.iobase = EXYNOS4_UART1_OFFSET
	},
	{
		.unit    = 2,
		.iobase = EXYNOS4_UART2_OFFSET
	},
	{
		.unit    = 3,
		.iobase = EXYNOS4_UART3_OFFSET
	},
#endif
};

/* sanity checks for serial console */
#ifndef CONSPEED
#define CONSPEED 115200
#endif	/* CONSPEED */
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB | HUPCL)) | CS8) /* 8N1 */
#endif	/* CONMODE */

// __CTASSERT(EXYNOS_CORE_PBASE + EXYNOS_UART0_OFFSET <= CONADDR);
// __CTASSERT(CONADDR <= EXYNOS_CORE_PBASE + EXYNOS_UART4_OFFSET);
// __CTASSERT(CONADDR % EXYNOS_BLOCK_SIZE == 0);
//static const bus_addr_t conaddr = CONADDR;
static const int conspeed = CONSPEED;
static const int conmode = CONMODE;
#endif /*defined(KGDB) || defined(SSCOM0CONSOLE) || defined(SSCOM1CONSOLE) */

/*
 * uboot passes 4 arguments to us.
 *
 * arg0 arg1 arg2 arg3 : the `bootargs' environment variable from the uboot
 * context (in PA!)
 *
 * Note that the storage has to be in .data and not in .bss. On kernel start
 * the .bss is cleared and this information would get lost.
 */
uintptr_t uboot_args[4] = { 0 };


/*
 * argument and boot configure storage
 */
BootConfig bootconfig;				/* for pmap's sake */
//static char bootargs[MAX_BOOT_STRING];	/* copied string from uboot */
char *boot_args = NULL;				/* MI bootargs */
char *boot_file = NULL;				/* MI bootfile */


/*
 * kernel start and end from the linker
 */
extern char KERNEL_BASE_phys[];	/* physical start of kernel */
extern char _end[];		/* physical end of kernel */
#define KERNEL_BASE_PHYS	((paddr_t)KERNEL_BASE_phys)

#define EXYNOS_IOPHYSTOVIRT(a) \
    ((vaddr_t)(((a) - EXYNOS_CORE_PBASE) + EXYNOS_CORE_VBASE))

/* XXX we have no framebuffer implementation yet so com is console XXX */
int use_fb_console = false;


/* prototypes */
void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif
void odroid_device_register(device_t self, void *aux);


/*
 * Our static device mappings at fixed virtual addresses so we can use them
 * while booting the kernel.
 *
 * Map the extents segment-aligned and segment-rounded in size to avoid L2
 * page tables
 */

#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & (~(L1_S_SIZE-1)))

static const struct pmap_devmap e4_devmap[] = {
	{
		/* map in core IO space */
		.pd_va    = _A(EXYNOS_CORE_VBASE),
		.pd_pa    = _A(EXYNOS_CORE_PBASE),
		.pd_size  = _S(EXYNOS4_CORE_SIZE),
		.pd_prot  = VM_PROT_READ | VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

static const struct pmap_devmap e5_devmap[] = {
	{
		/* map in core IO space */
		.pd_va    = _A(EXYNOS_CORE_VBASE),
		.pd_pa    = _A(EXYNOS_CORE_PBASE),
		.pd_size  = _S(EXYNOS5_CORE_SIZE),
		.pd_prot  = VM_PROT_READ | VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};
#undef _A
#undef _S

#ifdef PMAP_NEED_ALLOC_POOLPAGE
static struct boot_physmem bp_highgig = {
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0,
};
#endif

/*
 * u_int initarm(...)
 *
 * Our entry point from the assembly before main() is called.
 * - take a copy of the config we got from uboot
 * - init the physical console
 * - setting up page tables for the kernel
 */

u_int
initarm(void *arg)
{
	const struct pmap_devmap const *devmap;
	bus_addr_t rambase;
	const psize_t ram_reserve = 0x200000;
	psize_t ram_size;

	/* allocate/map our basic memory mapping */
    	switch (EXYNOS_PRODUCT_FAMILY(exynos_soc_id)) {
#if defined(EXYNOS4)
	case EXYNOS4_PRODUCT_FAMILY:
		devmap = e4_devmap;
		rambase = EXYNOS4_SDRAM_PBASE;
		break;
#endif
#if defined(EXYNOS5)
	case EXYNOS5_PRODUCT_FAMILY:
		devmap = e5_devmap;
		rambase = EXYNOS5_SDRAM_PBASE;
		rambase = EXYNOS4_SDRAM_PBASE;
		break;
#endif
	default:
		/* Won't work, but... */
		panic("Unknown product family %llx",
		   EXYNOS_PRODUCT_FAMILY(exynos_soc_id));
	}
	pmap_devmap_register(devmap);

	/* bootstrap soc. uart_address is determined in odroid_start */
	paddr_t uart_address = armreg_tpidruro_read();
	exynos_bootstrap(EXYNOS_CORE_VBASE, EXYNOS_IOPHYSTOVIRT(uart_address));

	/* set up CPU / MMU / TLB functions */
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* get normal console working */
 	consinit();

#ifdef KGDB
	kgdb_port_init();
#endif

#ifdef VERBOSE_INIT_ARM
	printf("\nuboot arg = %#"PRIxPTR", %#"PRIxPTR", %#"PRIxPTR", %#"PRIxPTR"\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);
	printf("Exynos SoC ID %08x\n", exynos_soc_id);

	printf("initarm: cbar=%#x\n", armreg_cbar_read());
#endif

	/* init clocks */
	/* determine cpu clock source */
curcpu()->ci_data.cpu_cc_freq = 1*1000*1000*1000;	/* XXX hack XXX */

#if NARML2CC > 0
	if (CPU_ID_CORTEX_A9_P(curcpu()->ci_arm_cpuid)) {
		/* probe and enable the PL310 L2CC */
		const bus_space_handle_t pl310_bh =
			EXYNOS_IOPHYSTOVIRT(armreg_cbar_read());

#ifdef ARM_TRUSTZONE_FIRMWARE
		exynos_l2cc_init();
#endif
		arml2cc_init(&exynos_bs_tag, pl310_bh, 0x2000);
	}
#endif

	cpu_reset_address = exynos_wdt_reset;

#ifdef VERBOSE_INIT_ARM
	printf("\nNetBSD/evbarm (odroid) booting ...\n");
#endif

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = rambase;

	/*
	 * Determine physical memory by looking at the PoP package. This PoP
	 * package ID seems to be only available on Exynos4
	 *
	 * First assume the default 2Gb of memory, dictated by mapping too
	 */
	ram_size = (psize_t) 0xC0000000 - 0x40000000;

#if defined(EXYNOS4)
	if (IS_EXYNOS4_P()) {
		switch (exynos_pop_id) {
		case EXYNOS_PACKAGE_ID_2_GIG:
			KASSERT(ram_size <= 2UL*1024*1024*1024);
			break;
		default:
			printf("Unknown PoP package id 0x%08x, assuming 1Gb\n",
				exynos_pop_id);
			ram_size = (psize_t) 0x10000000;
		}
	}
#endif

	/*
	 * Configure DMA tags
	 */
	/* XXX dma not implemented yet */
	// exynos_dma_bootstrap(ram_size);

	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (ram_size > KERNEL_VM_BASE - KERNEL_BASE) {
		printf("%s: dropping RAM size from %luMB to %uMB\n",
		   __func__, (unsigned long) (ram_size >> 20),
		   (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		ram_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else
	const bool mapallmem_p = false;
#endif
	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size - ram_reserve,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, devmap,
	    mapallmem_p);

	/* forget about passed bootargs for now */
	/* TODO translate `uboot_args' phys->virt to bootargs */
char tmp[1000];
strcpy(tmp, "-v");
	boot_args = tmp; // bootargs;
	parse_mi_bootargs(boot_args);

	/* we've a specific device_register routine */
	evbarm_device_register = odroid_device_register;

	/*
	 * If we couldn't map all of memory via TTBR1, limit the memory the
	 * kernel can allocate from to be from the highest available 1GB.
	 */
#ifdef PMAP_NEED_ALLOC_POOLPAGE
	if (atop(ram_size) > bp_highgig.bp_pages) {
		bp_highgig.bp_start = rambase / NBPG
		     + atop(ram_size) - bp_highgig.bp_pages;
		bp_highgig.bp_pages -= atop(ram_reserve); 
		arm_poolpage_vmfreelist = bp_highgig.bp_freelist;
		return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
		    &bp_highgig, 1);
	}
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);
}


void
consinit(void)
{
	static bool consinit_called;

	if (consinit_called)
		return;
	consinit_called = true;

#if NSSCOM > 0
	bus_space_tag_t bst = &exynos_bs_tag;
	bus_addr_t iobase = armreg_tpidruro_read();
	bus_space_handle_t bsh = EXYNOS_IOPHYSTOVIRT(iobase);
	u_int i;
	/*	
	 * No need to guess at the UART frequency since we can caclulate it.
	 */
	uint32_t freq = conspeed
	   * (16 * (bus_space_read_4(bst, bsh, SSCOM_UBRDIV) + 1)
		 + bus_space_read_4(bst, bsh, SSCOM_UFRACVAL));
	freq = (freq + conspeed / 2) / 1000;
	freq *= 1000;

	for (i = 0; i < __arraycount(exynos_uarts); i++) {
		/* attach console */
		if (exynos_uarts[i].iobase + EXYNOS_CORE_PBASE == iobase)
			break;
	}
	KASSERT(i < __arraycount(exynos_uarts));
	printf("%s: attaching console @ %#"PRIxPTR" (%u HZ, %u bps)\n",
	    __func__, iobase, freq, conspeed);
	if (sscom_cnattach(bst, exynos_core_bsh, &exynos_uarts[i],
			   conspeed, freq, conmode))
		panic("Serial console can not be initialized");
#ifdef VERBOSE_INIT_ARM
	printf("Console initialized\n");
#endif
#else
#error only serial console is supported
#if NUKBD > 0
	/* allow USB keyboards to become console */
	ukbd_cnattach();
#endif /* NUKBD */
#endif
}


#ifdef EXYNOS4
static void
odroid_exynos4_gpio_ncs(device_t self, prop_dictionary_t dict) {
	/*
	 * These pins have no connections; the 1st is a raw values that is
	 * generated by the gpio bootstrap and the values substracted are
	 * explicitly allowed
	 */
	prop_dictionary_set_uint32(dict, "nc-GPA0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPA1", 0x3f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPB",  0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPC0", 0x1f - 0x00);
	/* blue led at bit 0 : */
	prop_dictionary_set_uint32(dict, "nc-GPC1", 0x1f - 0x01);
	prop_dictionary_set_uint32(dict, "nc-GPD0", 0x0f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPD1", 0x0f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPF0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPF1", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPF2", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPF3", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPJ0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPJ1", 0x1f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPK0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPK1", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPK2", 0x7f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPK3", 0x7f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPL0", 0x7f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPL1", 0x03 - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPL2", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY0", 0x3f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY1", 0x0f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY2", 0x3f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY3", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY4", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY5", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPY6", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPM0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPM1", 0x7f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPM2", 0x1f - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPM3", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPM4", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPX0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPX1", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPX2", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPX3", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPZ",  0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPV0", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPV1", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPV2", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPV3", 0xff - 0x00);
	prop_dictionary_set_uint32(dict, "nc-GPV4", 0x03 - 0x00);
}
#endif


#ifdef EXYNOS5
static void
odroid_exynos5_gpio_ncs(device_t self, prop_dictionary_t dict) {
	/*
	 * These pins have no connections; the 1st is a raw values that is
	 * generated by the gpio bootstrap and the values substracted are
	 * explicitly allowed
	 */
	/* TBD: generate these values, see exynos_gpio.c boostrap */
}
#endif


void
odroid_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	exynos_device_register(self, aux);

	if (device_is_a(self, "sscom")) {
		if (use_fb_console)
			prop_dictionary_set_bool(dict, "is_console", false);
	}

	if (device_is_a(self, "mct"))
		prop_dictionary_set_cstring(dict, "blinkled", "led1");

	if (device_is_a(self, "exyogpio")) {
#ifdef EXYNOS4
		if (IS_EXYNOS4_P()) {
			/* unused bits */
			odroid_exynos4_gpio_ncs(self, dict);
			/* explicit pin settings */
			prop_dictionary_set_cstring(dict, "led1", ">GPC1[0]");
		}
#endif
#ifdef EXYNOS5
		if (IS_EXYNOS5_P()) {
			/* unused bits */
			odroid_exynos5_gpio_ncs(self, dict);
			/* explicit pin settings */
		}
#endif
	}
}

