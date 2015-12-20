/*	$NetBSD: exynos_machdep.c,v 1.6 2015/12/20 05:25:01 marty Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: exynos_machdep.c,v 1.6 2015/12/20 05:25:01 marty Exp $");

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
#include <evbarm/exynos/platform.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/usb/ukbdvar.h>
#include <net/if_ether.h>

/* serial console stuff */
#include "sscom.h"
#include "opt_sscom.h"

#include <arm/samsung/sscom_var.h>
#include <arm/samsung/sscom_reg.h>

/* so we can load the device tree. NOTE: This requires the kernel to be
 * made into a linux (not netbsd) uboot image.
 */
#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#define FDT_BUF_SIZE	(128*1024)
static uint8_t fdt_data[FDT_BUF_SIZE];

extern const int num_exynos_uarts_entries;
extern const struct sscom_uart_info exynos_uarts[];

#ifndef CONSPEED
#define CONSPEED 115200
#endif	/* CONSPEED */
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB | HUPCL)) | CS8) /* 8N1 */
#endif	/* CONMODE */

static const int conspeed = CONSPEED;
static const int conmode = CONMODE;

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


void
odroid_device_register(device_t self, void *aux);

/*
 * kernel start and end from the linker
 */
extern char KERNEL_BASE_phys[];	/* physical start of kernel */
extern char _end[];		/* physical end of kernel */
#define KERNEL_BASE_PHYS	((paddr_t)KERNEL_BASE_phys)

#define EXYNOS_IOPHYSTOVIRT(a) \
    ((vaddr_t)(((a) - EXYNOS_CORE_PBASE) + EXYNOS_CORE_VBASE))

static void exynos_reset(void);
static void exynos_powerdown(void);
/* XXX we have no framebuffer implementation yet so com is console XXX */
int use_fb_console = false;


/* prototypes */
void consinit(void);
#ifdef KGDB
static void kgdb_port_init(void);
#endif
static void exynos_extract_mac_adress(void);
void exynos_device_register(device_t self, void *aux);
void exynos_device_register_post_config(device_t self, void *aux);

/*
 * Our static device mappings at fixed virtual addresses so we can use them
 * while booting the kernel.
 *
 * Map the extents segment-aligned and segment-rounded in size to avoid L2
 * page tables
 */

#define _A(a)	((a) & ~L1_S_OFFSET)
#define _S(s)	(((s) + L1_S_SIZE - 1) & (~(L1_S_SIZE-1)))

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

static struct gpio_pin_entry {
	const char *pin_name;
	const char *pin_user;
} gpio_pin_entries[] = {
	/* mux@13400000 (muxa) */
	{ "gpx3-7", "hdmi-hpd-irq"},
	{ "gpx3-6", "hdmi_cec" },
	{ "gpx0-7", "dp_hpd_gpio" },
	{ "gpx0-4", "pmic-irq" },
	{ "gpx3-2", "audio-irq" },
	{ "gpx3-4", "b-sess1-irq" },
	{ "gpx3-5", "b-sess0-irq" },
	{ "gpx1-1", "id2-irq" },
	/* mux@134100000 (muxb) */
	{ "gpc0-0", "sd0-clk" },
	{ "gpc0-1", "sd0-cmd" },
	{ "gpc0-7", "sd0-rdqs" },
	{ "gpd1-3", "sd0-qrdy" },
	{ "gpc0-3", "sd0-bus-width1" },
	{ "gpc0-3", "sd0-bus-width4-bit1" },
	{ "gpc0-4", "sd0-bus-width4-bit2" },
	{ "gpc0-5", "sd0-bus-width4-bit3" },
	{ "gpc0-6", "sd0-bus-width4-bit4" },
	{ "gpc1-0", "sd1-clk" },
	{ "gpc1-1", "sd1-cmd" },
	{ "gpc1-3", "sd1-bus-width1" },
	{ "gpc1-3", "sd1-bus-width4-bit1" },
	{ "gpc1-4", "sd1-bus-width4-bit2" },
	{ "gpc1-5", "sd1-bus-width4-bit3" },
	{ "gpc1-6", "sd1-bus-width4-bit4" },
	/* TODO: muxc and muxd as needed */
	{ 0, 0}
};
	
#ifdef VERBOSE_INIT_ARM
extern void exynos_putchar(int);

static void
exynos_putstr(const char *s)
{
	for (const char *p = s; *p; p++) {
		exynos_putchar(*p);
	}
}

static void
exynos_printn(u_int n, int base)
{
	char *p, buf[(sizeof(u_int) * NBBY / 3) + 1 + 2 /* ALT + SIGN */];

	p = buf;
	do {
		*p++ = hexdigits[n % base];
	} while (n /= base);

	do {
		exynos_putchar(*--p);
	} while (p > buf);
}
#define DPRINTF(...)		printf(__VA_ARGS__)
#define DPRINT(x)		exynos_putstr(x)
#define DPRINTN(x,b)		exynos_printn((x), (b))
#else
#define DPRINTF(...)
#define DPRINT(x)
#define DPRINTN(x,b)
#endif

extern void cortex_mpstart(void);

/*
 * void init_gpio_dictionary(...)
 *
 * Setup the dictionary of gpio pin names for the drivers to use
 */
static void init_gpio_dictionary(struct gpio_pin_entry *pins,
				 prop_dictionary_t dict)
{
	while (pins->pin_name) {
		prop_dictionary_set_cstring(dict, pins->pin_user,
					    pins->pin_name);
		pins++;
	}
}
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
	psize_t ram_size;
	DPRINT("initarm:");

	DPRINT(" mpstart<0x");
	DPRINTN((uint32_t)cortex_mpstart, 16);
	DPRINT(">");

	/* allocate/map our basic memory mapping */
    	switch (EXYNOS_PRODUCT_FAMILY(exynos_soc_id)) {
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

	/* bootstrap soc. uart_address is determined in exynos_start */
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

	cpu_reset_address = exynos_reset;
	cpu_powerdown_address = exynos_powerdown;

#ifdef VERBOSE_INIT_ARM
	printf("\nNetBSD/evbarm (Exynnos 5422) booting ...\n");
#endif

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);
	exynos_extract_mac_adress();

	/* Don't map the DMA reserved region */
//	ram_size = (psize_t) 0xC0000000 - 0x40000000;
	ram_size = (psize_t) 0xb0000000 - 0x40000000;

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

	/* Load the dtb */
	const uint8_t *fdt_addr_r = (const uint8_t *)uboot_args[2];
	printf("fdt addr 0x%08x\n", (uint)fdt_addr_r);
	int error = fdt_check_header(fdt_addr_r);
	printf("fdt check header returns %d\n", error);
	if (error == 0) {
		error = fdt_move(fdt_addr_r, fdt_data, sizeof(fdt_data));
		printf("fdt move returns %d\n", error);
		if (error != 0) {
			panic("fdt_move failed: %s", fdt_strerror(error));
		}
		fdtbus_set_data(fdt_data);
	} else {
		panic("fdt_check_header failed: %s", fdt_strerror(error));
	}

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = rambase;
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;
	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, devmap,
	    mapallmem_p);

	/* we've a specific device_register routine */
	evbarm_device_register = odroid_device_register;
//	evbarm_device_register_post_config = exynos_device_register_post_config;
	/*
	 * If we couldn't map all of memory via TTBR1, limit the memory the
	 * kernel can allocate from to be from the highest available 1GB.
	 */
#ifdef PMAP_NEED_ALLOC_POOLPAGE
	if (atop(ram_size) > bp_highgig.bp_pages) {
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

	/* go through all entries */
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

void
odroid_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);
	exynos_device_register(self, aux);
	if (device_is_a(self, "exyogpio")) {
		init_gpio_dictionary(gpio_pin_entries, dict);
	} else if (device_is_a(self, "exyowdt")) {
		prop_dictionary_set_uint32(dict, "frequency",
					   EXYNOS_F_IN_FREQ);
	}
}

/*
 * Exynos specific tweaks
 */
/*
 * The external USB devices are clocked trough the DEBUG clkout
 * XXX is this Odroid specific? XXX
 */
void
exynos_init_clkout_for_usb(void)
{
	/* Select XUSBXTI as source for CLKOUT */
	bus_space_write_4(&exynos_bs_tag, exynos_pmu_bsh,
		EXYNOS_PMU_DEBUG_CLKOUT, 0x1000);
}

static void
exynos_reset(void)
{
}

static void
exynos_powerdown(void)
{
}
