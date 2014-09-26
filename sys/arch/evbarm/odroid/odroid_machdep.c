/*	$NetBSD: odroid_machdep.c,v 1.36 2014/09/26 18:59:29 reinoud Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: odroid_machdep.c,v 1.36 2014/09/26 18:59:29 reinoud Exp $");

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
#include <net/if_ether.h>

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

extern const int num_exynos_uarts_entries;
extern const struct sscom_uart_info exynos_uarts[];
KASSERT(sizeof(struct sscom_uart_info) == 8);

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
#endif /*defined(KGDB) || defined(SSCOM*CONSOLE) */

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
char bootargs[MAX_BOOT_STRING] = "";		/* copied string from uboot */
char *boot_args = NULL;				/* MI bootargs */
char *boot_file = NULL;				/* MI bootfile */
uint8_t uboot_enaddr[ETHER_ADDR_LEN] = {};


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
static void exynos_usb_powercycle_lan9730(device_t self);
static void exynos_extract_mac_adress(void);
void odroid_device_register(device_t self, void *aux);
void odroid_device_register_post_config(device_t self, void *aux);


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
	{
		/* map in audiocore IO space */
		.pd_va    = _A(EXYNOS4_AUDIOCORE_VBASE),
		.pd_pa    = _A(EXYNOS4_AUDIOCORE_PBASE),
		.pd_size  = _S(EXYNOS4_AUDIOCORE_SIZE),
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
	{
		/* map in audiocore IO space */
		.pd_va    = _A(EXYNOS5_AUDIOCORE_VBASE),
		.pd_pa    = _A(EXYNOS5_AUDIOCORE_PBASE),
		.pd_size  = _S(EXYNOS5_AUDIOCORE_SIZE),
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

	/* determine cpu0 clock rate */
	exynos_clocks_bootstrap();
#ifdef VERBOSE_INIT_ARM
	printf("CPU0 now running on %"PRIu64" Mhz\n", exynos_get_cpufreq()/(1000*1000));
#endif

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
	boot_args = bootargs;
	parse_mi_bootargs(boot_args);
	exynos_extract_mac_adress();

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

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = rambase;
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

	/* we've a specific device_register routine */
	evbarm_device_register = odroid_device_register;
	evbarm_device_register_post_config = odroid_device_register_post_config;

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
	 * No need to guess at the UART frequency since we can calculate it.
	 */
	uint32_t freq = conspeed
	   * (16 * (bus_space_read_4(bst, bsh, SSCOM_UBRDIV) + 1)
		 + bus_space_read_4(bst, bsh, SSCOM_UFRACVAL));
	freq = (freq + conspeed / 2) / 1000;
	freq *= 1000;

	/* go trough all entries */
	for (i = 0; i < num_exynos_uarts_entries; i++) {
		/* attach console */
		if (exynos_uarts[i].iobase + EXYNOS_CORE_PBASE == iobase)
			break;
	}
	KASSERT(i < num_exynos_uarts_entries);
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


/* extract ethernet mac address from bootargs */
static void
exynos_extract_mac_adress(void)
{
	char *str, *ptr;
	int i, v1, v2, val;

#define EXPECT_COLON() {\
		v1 = *ptr++; \
		if (v1 != ':') break; \
	}
#define EXPECT_HEX(v) {\
		(v) = (v) >= '0' && (v) <= '9'? (v) - '0' : \
		      (v) >= 'a' && (v) <= 'f'? (v) - 'a' + 10 : \
		      (v) >= 'A' && (v) <= 'F'? (v) - 'A' + 10 : -1; \
		if ((v) < 0) break; \
	}
#define EXPECT_2HEX(val) {\
		v1 = *ptr++; EXPECT_HEX(v1); \
		v2 = *ptr++; EXPECT_HEX(v2); \
		val = (v1 << 4) | v2; \
	}
	if (get_bootconf_option(boot_args, "ethaddr",
			BOOTOPT_TYPE_STRING, &str)) {
		for (i = 0, ptr = str; i < sizeof(uboot_enaddr); i++) {
			if (i)
				EXPECT_COLON();
			EXPECT_2HEX(val);
			uboot_enaddr[i] = val;
		}
		if (i != sizeof(uboot_enaddr)) {
			printf( "Ignoring invalid MAC address '%s' passed "
				"as boot paramter `ethaddr'\n", str);
			memset((char *) uboot_enaddr, 0, sizeof(uboot_enaddr));
		}
	}
#undef EXPECT_2HEX
#undef EXPECT_HEX
#undef EXPECT_COLON
}


#ifdef EXYNOS4
static void
odroid_exynos4_gpio_ncs(device_t self, prop_dictionary_t dict) {
	/*
	 * These pins have no connections; the 1st is a raw values that is
	 * generated by the gpio bootstrap and the values substracted are
	 * explicitly allowed
	 */
	/* i2c at pin 6,7 */
	prop_dictionary_set_uint32(dict, "nc-GPA0", 0xff - 0b11000000);
	/* p3v3 enable on pin 3 */
	prop_dictionary_set_uint32(dict, "nc-GPA1", 0x3f - 0b00001000);
	prop_dictionary_set_uint32(dict, "nc-GPB",  0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPC0", 0x1f - 0b00000000);
	/* blue led at bit 0 : heartbeat */
	prop_dictionary_set_uint32(dict, "nc-GPC1", 0x1f - 0b00000001);
	prop_dictionary_set_uint32(dict, "nc-GPD0", 0x0f - 0b00000000);
	/* i2c0 at pin 0,1 and i2c1 at pin 2,3 : */
	prop_dictionary_set_uint32(dict, "nc-GPD1", 0x0f - 0b00001111);
	prop_dictionary_set_uint32(dict, "nc-GPF0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPF1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPF2", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPF3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPJ0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPJ1", 0x1f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPK0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPK1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPK2", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPK3", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPL0", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPL1", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPL2", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY0", 0x3f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY1", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY2", 0x3f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY4", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY5", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY6", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-ETC0", 0x3f - 0b00000000);
	/* standard Xuhost bits at pin 6,7 */
	prop_dictionary_set_uint32(dict, "nc-ETC6", 0xff - 0b11000000);
	prop_dictionary_set_uint32(dict, "nc-GPM0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPM1", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPM2", 0x1f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPM3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPM4", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPX0", 0xff - 0b00000000);
	/* expansion connector bits at pin 0,1,5 : */
	prop_dictionary_set_uint32(dict, "nc-GPX1", 0xff - 0b00100011);
	prop_dictionary_set_uint32(dict, "nc-GPX2", 0xff - 0b00000000);
	/* usb hub communication at pin 0,4,5 : */
	prop_dictionary_set_uint32(dict, "nc-GPX3", 0xff - 0b00110001);
	prop_dictionary_set_uint32(dict, "nc-GPZ",  0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-ETC7", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV2", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-ETC8", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV4", 0x03 - 0b00000000);
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
	/* i2c2 at pin 6,7 */
	prop_dictionary_set_uint32(dict, "nc-GPA0", 0xff - 0b11000000);
	prop_dictionary_set_uint32(dict, "nc-GPA1", 0x3f - 0b00000000);
	/* i2c4 at pin 0,1 */
	prop_dictionary_set_uint32(dict, "nc-GPA2", 0xff - 0b00000011);
	prop_dictionary_set_uint32(dict, "nc-GPB0", 0x1f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPB1", 0x1f - 0b00000000);
	/* green led at bit 1 : eMMC activity */
	/* red   led at bit 2 : heartbeat */
	prop_dictionary_set_uint32(dict, "nc-GPB2", 0x0f - 0b00000110);
	/* i2c1 at pin 2,3 */
	prop_dictionary_set_uint32(dict, "nc-GPB3", 0x0f - 0b00001100);
	prop_dictionary_set_uint32(dict, "nc-GPC0", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPC1", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPC2", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPC3", 0x7f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPD0", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPD1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY0", 0x3f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY1", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY2", 0x3f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY4", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY5", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPY6", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-ETC0", 0x3f - 0b00000000);
	/* standard Xuhost bits at pin 5,6 */
	prop_dictionary_set_uint32(dict, "nc-ETC6", 0x7f - 0b01100000);
	prop_dictionary_set_uint32(dict, "nc-ETC7", 0x1f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPC4", 0x3f - 0b00000000);
	/* usb hub communication at bit 6,7 : */
	prop_dictionary_set_uint32(dict, "nc-GPX0", 0xff - 0b11000000);
	/* usb hub communication at bit 4 : */
	prop_dictionary_set_uint32(dict, "nc-GPX1", 0xff - 0b00010000);
	/* blue led at bit 3 : microSD activity */
	prop_dictionary_set_uint32(dict, "nc-GPX2", 0xff - 0b00001000);
	prop_dictionary_set_uint32(dict, "nc-GPX3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPE0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPE1", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPF0", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPF1", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPG0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPG1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPG2", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPH0", 0x0f - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPH1", 0xff - 0b00000000);

	prop_dictionary_set_uint32(dict, "nc-GPJ0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPJ1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPJ2", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPJ3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPJ4", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPK0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPK1", 0xff - 0b00000000);
	/* usb3 overcur1{2,3} at bits 4,5, vbus1 at pin 7 */
	prop_dictionary_set_uint32(dict, "nc-GPK2", 0xff - 0b10110000);
	/* usb3 overcur0{2,3} at bits 0,1, vbus0 at pin 3 */
	prop_dictionary_set_uint32(dict, "nc-GPK3", 0xff - 0b00001011);

	prop_dictionary_set_uint32(dict, "nc-GPV0", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV1", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-ETC5", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV2", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV3", 0xff - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-ETC8", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPV4", 0x03 - 0b00000000);
	prop_dictionary_set_uint32(dict, "nc-GPZ",  0x7f - 0b00000000);
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
		prop_dictionary_set_cstring(dict, "heartbeat", "led1");

	if (device_is_a(self, "exyousb")) {
		prop_dictionary_set_cstring(dict, "nreset", "hub_nreset");
		prop_dictionary_set_cstring(dict, "hubconnect", "hub_connect");
		prop_dictionary_set_cstring(dict, "nint", "hub_nint");
		prop_dictionary_set_cstring(dict, "lan_power", "p3v3_en");
	}

	if (device_is_a(self, "usmsc")) {
		prop_data_t blob =
		    prop_data_create_data(uboot_enaddr, ETHER_ADDR_LEN);
		if (prop_dictionary_set(dict, "mac-address", blob) == false) {
			aprint_error_dev(self,
			    "WARNING: unable to set mac-address property\n");
		}
		prop_object_release(blob);
	}

#ifdef EXYNOS4
	if (device_is_a(self, "exyogpio") && (IS_EXYNOS4_P())) {
		/* unused bits */
		odroid_exynos4_gpio_ncs(self, dict);

		/* explicit pin settings */
		prop_dictionary_set_cstring(dict, "led1", ">GPC1[0]");

		prop_dictionary_set_cstring(dict, "hub_nreset", ">GPX3[5]");
		prop_dictionary_set_cstring(dict, "hub_connect", ">GPX3[4]");
		prop_dictionary_set_cstring(dict, "hub_nint", "<GPX3[0]");

		prop_dictionary_set_cstring(dict, "p3v3_en", ">GPA1[3]");
	}
	if (device_is_a(self, "exyoiic") && (IS_EXYNOS4_P())) {
		prop_dictionary_set_bool(dict, "iic0_enable", true);
		prop_dictionary_set_bool(dict, "iic1_enable", true);
		prop_dictionary_set_bool(dict, "iic2_enable", true);
		/* IIC3 not used (NC) */
		/* IIC4 not used (NC) */
		/* IIC5 not used (NC) */
		/* IIC6 used differently (SCLK used as led1) */
		/* IIC7 used differently (PWM, though NC)    */
		/* IIC8 HDMI, not possible trough GPIO */
	}
#endif
#ifdef EXYNOS5
	if (device_is_a(self, "exyogpio") && (IS_EXYNOS5_P())) {
		/* unused bits */
		odroid_exynos5_gpio_ncs(self, dict);

		/* explicit pin settings */
		prop_dictionary_set_cstring(dict, "hub_nreset", ">GPX1[4]");
		prop_dictionary_set_cstring(dict, "hub_connect", ">GPX0[6]");
		prop_dictionary_set_cstring(dict, "hub_nint", "<GPX0[7]");

		/* internal hub IIRC, unknown if this line exists */
		//prop_dictionary_set_cstring(dict, "p3v3_en", ">GPA1[3]");
	}
	if (device_is_a(self, "exyoiic") && (IS_EXYNOS5_P())) {
		/* IIC0 not used (NC) */
		prop_dictionary_set_bool(dict, "iic1_enable", true);
		prop_dictionary_set_bool(dict, "iic2_enable", true);
		/* IIC3 not used (NC) */
		prop_dictionary_set_bool(dict, "iic4_enable", true);
		/* IIC5 not used (NC) */
		/* IIC6 used differently (SCLK used as led1) */
		/* IIC7 used differently (PWM, though NC)    */
		/* IIC8 HDMI, not possible trough GPIO */
	}
#endif
}


static void
exynos_usb_init_usb3503_hub(device_t self)
{
	prop_dictionary_t dict = device_properties(self);
	const char *pin_nreset, *pin_hubconnect, *pin_nint;
	struct exynos_gpio_pindata nreset_pin, hubconnect_pin, nint_pin;
	int ok1, ok2, ok3;

	prop_dictionary_get_cstring_nocopy(dict, "nreset", &pin_nreset);
	prop_dictionary_get_cstring_nocopy(dict, "hubconnect", &pin_hubconnect);
	prop_dictionary_get_cstring_nocopy(dict, "nint", &pin_nint);
	if (!(pin_nreset && pin_hubconnect && pin_nint)) {
		aprint_error_dev(self,
			"failed to lookup GPIO pins for usb3503 hub init");
		return;
	}

	ok1 = exynos_gpio_pin_reserve(pin_nreset, &nreset_pin);
	ok2 = exynos_gpio_pin_reserve(pin_hubconnect, &hubconnect_pin);
	ok3 = exynos_gpio_pin_reserve(pin_nint, &nint_pin);
	if (!ok1)
		aprint_error_dev(self,
		    "can't reserve GPIO pin %s\n", pin_nreset);
	if (!ok2)
		aprint_error_dev(self,
		    "can't reserve GPIO pin %s\n", pin_hubconnect);
	if (!ok3)
		aprint_error_dev(self,
		    "can't reserve GPIO pin %s\n", pin_nint);
	if (!(ok1 && ok2 && ok3))
		return;

	/* reset pin to zero */
	exynos_gpio_pindata_write(&nreset_pin, 0);
	DELAY(10000);

	/* pull intn low */
	exynos_gpio_pindata_ctl(&nint_pin, GPIO_PIN_PULLDOWN);

	/* set hubconnect low */
	exynos_gpio_pindata_write(&hubconnect_pin, 0);

	/* reset pin up again, hub enters RefClk stage */
	exynos_gpio_pindata_write(&nreset_pin, 1);
	DELAY(10000);

	/* release intn */
	exynos_gpio_pindata_ctl(&nint_pin, GPIO_PIN_TRISTATE);

	/* set hubconnect high */
	exynos_gpio_pindata_write(&hubconnect_pin, 1);
	DELAY(40000);

	/* DONE! */
}


#if 0
static void
exynos_max77686_dump(struct i2c_controller *i2c)
{
	int error;

	printf("%s:\n", __func__);
	for (int i = 0; i < 0x80; i++) {
		error = iic_exec(i2c, I2C_OP_READ_WITH_STOP, 0x09, &i, 1,
				&rdata, sizeof(rdata), 0);
		KASSERT(!error);
		if (i % 16 == 0)
			printf("\n%02x: ", i);
		printf("%02x ", rdata);
	}
	printf("\n");
}
#endif


static void
exynos_usb_powercycle_lan9730(device_t self)
{
#ifdef EXYNOS4
	prop_dictionary_t dict = device_properties(self);
	struct exynos_gpio_pindata enable_pin;
	struct i2c_controller *i2c;
	const char *pin_enable;
	uint8_t rdata, wdata, reg;
	int error __diagused;
	bool ok;

	/*
	 * XXX fixed for now:
	 *    Odroid-U2/U3 : Max77686 chip is attached to iic0 chipid 0x09
	 */
	const int iicbus      = 0;
	const int chipid      = 0x09;
	const int buck_outreg = 0x37;	/* buck 8 output voltage */
	const int buck_ctlreg = 0x38;	/* buck 8 power control */

	i2c = exynos_i2cbus[iicbus];
	KASSERT(i2c);
	iic_acquire_bus(i2c, 0);

	/* set power level to 0v */
	reg = buck_outreg;
	wdata = 0x0;
	error = iic_exec(i2c, I2C_OP_WRITE_WITH_STOP, chipid, &reg, 1,
			&wdata, sizeof(wdata), 0);
	KASSERT(!error);
	DELAY(20000);

	/* set power level back to 3.3v */
	wdata = 0x33;
	error = iic_exec(i2c, I2C_OP_WRITE_WITH_STOP, chipid, &reg, 1,
			&wdata, sizeof(wdata), 0);
	KASSERT(!error);
	DELAY(20000);

	/* enable the bucket explicitly */
	reg = buck_ctlreg;
        error = iic_exec(i2c, I2C_OP_READ_WITH_STOP, chipid, &reg, 1,
			&rdata, sizeof(rdata), 0);
	KASSERT(!error);
	rdata |= 3;
	error = iic_exec(i2c, I2C_OP_WRITE_WITH_STOP, chipid, &reg, 1,
			&rdata, sizeof(rdata), 0);
	KASSERT(!error);
	DELAY(30000);

	iic_release_bus(i2c, 0);

	/* enable lan chip power */
	prop_dictionary_get_cstring_nocopy(dict, "lan_power", &pin_enable);
	if (pin_enable)  {
		ok = exynos_gpio_pin_reserve(pin_enable, &enable_pin);
		if (!ok) {
			aprint_error_dev(self,
				"can't reserve GPIO pin %s\n", pin_enable);
		} else {
			exynos_gpio_pindata_write(&enable_pin, 0);
			DELAY(30000);
			exynos_gpio_pindata_write(&enable_pin, 1);
			DELAY(30000);
		}
	} else {
		aprint_error_dev(self, "failed to lookup lan_power GPIO pin");
	}
#endif
}


void
odroid_device_register_post_config(device_t self, void *aux)
{
	exynos_device_register_post_config(self, aux);

	if (device_is_a(self, "exyousb")) {
		exynos_usb_powercycle_lan9730(self);
		exynos_usb_init_usb3503_hub(self);
	}
}

