/* $NetBSD: tegra_machdep.c,v 1.38.2.1 2017/04/26 02:53:02 pgoyette Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_machdep.c,v 1.38.2.1 2017/04/26 02:53:02 pgoyette Exp $");

#include "opt_tegra.h"
#include "opt_machdep.h"
#include "opt_ddb.h"
#include "opt_md.h"
#include "opt_arm_debug.h"
#include "opt_multiprocessor.h"

#include "com.h"
#include "ukbd.h"
#include "genfb.h"
#include "ether.h"
#include "as3722pmic.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
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

#include <machine/bootconfig.h>
#include <arm/armreg.h>
#include <arm/undefined.h>

#include <arm/arm32/machdep.h>
#include <arm/mainbus/mainbus.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <arm/cortex/gtmr_var.h>

#include <evbarm/include/autoconf.h>
#include <evbarm/tegra/platform.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/usb/ukbdvar.h>
#include <net/if_ether.h>

#if NAS3722PMIC > 0
#include <dev/i2c/as3722.h>
#endif

#ifndef TEGRA_MAX_BOOT_STRING
#define TEGRA_MAX_BOOT_STRING 1024
#endif

BootConfig bootconfig;
char bootargs[TEGRA_MAX_BOOT_STRING] = "";
char *boot_args = NULL;
u_int uboot_args[4] = { 0 };	/* filled in by tegra_start.S (not in bss) */

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#define FDT_BUF_SIZE	(128*1024)
static uint8_t fdt_data[FDT_BUF_SIZE];

extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

static void tegra_device_register(device_t, void *);
static void tegra_reset(void);
static void tegra_powerdown(void);

bs_protos(bs_notimpl);

#define	_A(a)	((a) & ~L1_S_OFFSET)
#define	_S(s)	(((s) + L1_S_SIZE - 1) & ~(L1_S_SIZE-1))

static const struct pmap_devmap devmap[] = {
	{
		.pd_va = _A(TEGRA_HOST1X_VBASE),
		.pd_pa = _A(TEGRA_HOST1X_BASE),
		.pd_size = _S(TEGRA_HOST1X_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{
		.pd_va = _A(TEGRA_PPSB_VBASE),
		.pd_pa = _A(TEGRA_PPSB_BASE),
		.pd_size = _S(TEGRA_PPSB_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{
		.pd_va = _A(TEGRA_APB_VBASE),
		.pd_pa = _A(TEGRA_APB_BASE),
		.pd_size = _S(TEGRA_APB_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{
		.pd_va = _A(TEGRA_AHB_A2_VBASE),
		.pd_pa = _A(TEGRA_AHB_A2_BASE),
		.pd_size = _S(TEGRA_AHB_A2_SIZE),
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,
		.pd_cache = PTE_NOCACHE
	},
	{0}
};

#undef	_A
#undef	_S

#ifdef PMAP_NEED_ALLOC_POOLPAGE
static struct boot_physmem bp_lowgig = {
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0
};
#endif

#ifdef VERBOSE_INIT_ARM
static void
tegra_putchar(char c)
{
#ifdef CONSADDR
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = c;
#endif
}
static void
tegra_putstr(const char *s)
{
	for (const char *p = s; *p; p++) {
		tegra_putchar(*p);
	}
}

static void
tegra_printn(u_int n, int base)
{
	char *p, buf[(sizeof(u_int) * NBBY / 3) + 1 + 2 /* ALT + SIGN */];

	p = buf;
	do {
		*p++ = hexdigits[n % base];
	} while (n /= base);

	do {
		tegra_putchar(*--p);
	} while (p > buf);
}
#define DPRINTF(...)		printf(__VA_ARGS__)
#define DPRINT(x)		tegra_putstr(x)
#define DPRINTN(x,b)		tegra_printn((x), (b))
#else
#define DPRINTF(...)
#define DPRINT(x)
#define DPRINTN(x,b)
#endif

extern void cortex_mpstart(void);

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
	bus_addr_t memory_addr;
	bus_size_t memory_size;
	psize_t ram_size = 0;
	DPRINT("initarm:");

	DPRINT(" mpstart<0x");
	DPRINTN((uint32_t)cortex_mpstart, 16);
	DPRINT(">");

	DPRINT(" devmap");
	pmap_devmap_register(devmap);

	DPRINT(" bootstrap");
	tegra_bootstrap();

	/* Heads up ... Setup the CPU / MMU / TLB functions. */
	DPRINT(" cpufunc");
	if (set_cpufuncs())
		panic("cpu not recognized!");

	/* Load FDT */
	const uint8_t *fdt_addr_r = (const uint8_t *)uboot_args[2];
	int error = fdt_check_header(fdt_addr_r);
	if (error == 0) {
		error = fdt_move(fdt_addr_r, fdt_data, sizeof(fdt_data));
		if (error != 0) {
			DPRINT(" (fdt_move failed!)\n");
			panic("fdt_move failed: %s", fdt_strerror(error));
		}
		fdtbus_set_data(fdt_data);
	} else {
		DPRINT(" (fdt_check_header failed!)\n");
		panic("fdt_check_header failed: %s", fdt_strerror(error));
	}

	DPRINT(" consinit");
	consinit();

	DPRINTF(" cbar=%#x", armreg_cbar_read());

	DPRINTF(" ok\n");

	DPRINTF("uboot: args %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);

	cpu_reset_address = tegra_reset;
	cpu_powerdown_address = tegra_powerdown;

	/* Talk to the user */
	DPRINTF("\nNetBSD/evbarm (tegra) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	const u_int chip_id = tegra_chip_id();
	switch (chip_id) {
#ifdef SOC_TEGRA124
        case CHIP_ID_TEGRA124: {
		const char * const tegra124_compatible_strings[] = {
			"nvidia,tegra124",
			NULL
		};
		const int node = OF_peer(0);
                if (of_compatible(node, tegra124_compatible_strings) < 0) {
			panic("FDT is not compatible with Tegra124");
		}
                break;
	}
#endif
	default:
		panic("Kernel does not support Tegra SOC ID %#x", chip_id);
	}

	DPRINTF("KERNEL_BASE=0x%x, KERNEL_VM_BASE=0x%x, KERNEL_VM_BASE - KERNEL_BASE=0x%x, KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE, KERNEL_VM_BASE, KERNEL_VM_BASE - KERNEL_BASE, KERNEL_BASE_VOFFSET);

	const int memory = OF_finddevice("/memory");
	if (fdtbus_get_reg(memory, 0, &memory_addr, &memory_size) != 0)
		panic("Cannot determine memory size");

	DPRINTF("FDT memory node = %d, addr %llx, size %llu\n",
	    memory, (unsigned long long)memory_addr,
	    (unsigned long long)memory_size);

	ram_size = memory_size;

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

	/*
	 * If MEMSIZE specified less than what we really have, limit ourselves
	 * to that.
	 */
#ifdef MEMSIZE
	if (ram_size == 0 || ram_size > (unsigned)MEMSIZE * 1024 * 1024)
		ram_size = (unsigned)MEMSIZE * 1024 * 1024;
	DPRINTF("ram_size = 0x%x\n", (int)ram_size);
#else
	KASSERTMSG(ram_size > 0, "RAM size unknown and MEMSIZE undefined");
#endif

	/* DMA tag setup */
	tegra_dma_bootstrap(ram_size);

	/* Fake bootconfig structure for the benefit of pmap.c. */
	bootconfig.dramblocks = 1;
	bootconfig.dram[0].address = memory_addr;
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap,
	    mapallmem_p);

	const int chosen = OF_finddevice("/chosen");
	if (chosen >= 0) {
		OF_getprop(chosen, "bootargs", bootargs, sizeof(bootargs));
	}

	DPRINTF("bootargs: %s\n", bootargs);

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	evbarm_device_register = tegra_device_register;

#ifdef PMAP_NEED_ALLOC_POOLPAGE
	bp_lowgig.bp_start = memory_addr / NBPG;
	if (atop(ram_size) > bp_lowgig.bp_pages) {
		arm_poolpage_vmfreelist = bp_lowgig.bp_freelist;
		return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
		    &bp_lowgig, 1);
	}
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

}

#if NCOM > 0
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#endif

void
consinit(void)
{
	static bool consinit_called = false;

	if (consinit_called)
		return;
	consinit_called = true;

#if NCOM > 0
	bus_addr_t addr;
	int speed;

#ifdef CONSADDR
	addr = CONSADDR;
#else
	const char *stdout_path = fdtbus_get_stdout_path();
	if (stdout_path == NULL) {
		DPRINT(" (can't find stdout-path!)\n");
		panic("Cannot find console device");
	}
	DPRINT(" ");
	DPRINT(stdout_path);
	fdtbus_get_reg(fdtbus_get_stdout_phandle(), 0, &addr, NULL);
#endif
	DPRINT(" 0x");
	DPRINTN((uint32_t)addr, 16);

#ifdef CONSPEED
	speed = CONSPEED;
#else
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
#endif
	DPRINT(" ");
	DPRINTN((uint32_t)speed, 10);

	const bus_space_tag_t bst = &armv7_generic_a4x_bs_tag;
	const u_int freq = 408000000;	/* 408MHz PLLP_OUT0 */
	if (comcnattach(bst, addr, speed, freq, COM_TYPE_TEGRA, CONMODE))
		panic("Serial console cannot be initialized.");
#else
#error only COM console is supported
#endif
}

static bool
tegra_bootconf_match(const char *key, const char *val)
{
	char *s;

	if (!get_bootconf_option(boot_args, key, BOOTOPT_TYPE_STRING, &s))
		return false;

	return strncmp(s, val, strlen(val)) == 0;
}

static char *
tegra_bootconf_strdup(const char *key)
{
	char *s, *ret;
	int i = 0;

	if (!get_bootconf_option(boot_args, key, BOOTOPT_TYPE_STRING, &s))
		return NULL;

	for (;;) {
		if (s[i] == ' ' || s[i] == '\t' || s[i] == '\0')
			break;
		++i;
	}

	ret = kmem_alloc(i + 1, KM_SLEEP);
	if (ret == NULL)
		return NULL;

	strlcpy(ret, s, i + 1);
	return ret;
}

void
tegra_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = &armv7_generic_bs_tag;
		return;
	}

	if (device_is_a(self, "armgtmr")) {
                prop_dictionary_set_uint32(dict, "frequency", TEGRA_REF_FREQ);
		return;
	}

	if (device_is_a(self, "tegrafb")
	    && tegra_bootconf_match("console", "fb")) {
		prop_dictionary_set_bool(dict, "is_console", true);
#if NUKBD > 0
		ukbd_cnattach();
#endif
	}

	if (device_is_a(self, "tegradrm")) {
		const char *video = tegra_bootconf_strdup("video");

		if (tegra_bootconf_match("hdmi.forcemode", "dvi")) {
			prop_dictionary_set_bool(dict, "force-dvi", true);
		}

		if (video) {
			prop_dictionary_set_cstring(dict, "HDMI-A-1", video);
		}
	}

	if (device_is_a(self, "tegracec")) {
		prop_dictionary_set_cstring(dict, "hdmi-device", "tegradrm0");
	}

	if (device_is_a(self, "nouveau")) {
		const char *config = tegra_bootconf_strdup("nouveau.config");
		const char *debug = tegra_bootconf_strdup("nouveau.debug");
		if (config)
			prop_dictionary_set_cstring(dict, "config", config);
		if (debug)
			prop_dictionary_set_cstring(dict, "debug", debug);
	}

	if (device_is_a(self, "tegrapcie")) {
		const char * const jetsontk1_compat[] = {
		    "nvidia,jetson-tk1", NULL
		};
		int phandle = OF_peer(0);
		if (of_match_compatible(phandle, jetsontk1_compat)) {
			/* rfkill GPIO at GPIO X7 */
			struct tegra_gpio_pin *pin;
			pin = tegra_gpio_acquire("X7", GPIO_PIN_OUTPUT);
			if (pin) {
				tegra_gpio_write(pin, 1);
			}
		}
	}
}

static void
tegra_reset(void)
{
#if NAS3722PMIC > 0
	device_t pmic = device_find_by_driver_unit("as3722pmic", 0);
	if (pmic != NULL) {
		delay(1000000);
		if (as3722_reboot(pmic) != 0) {
			printf("WARNING: AS3722 reset failed\n");
			return;
		}
	}
#endif
	tegra_pmc_reset();
}

static void
tegra_powerdown(void)
{
#if NAS3722PMIC > 0
	device_t pmic = device_find_by_driver_unit("as3722pmic", 0);
	if (pmic != NULL) {
		delay(1000000);
		if (as3722_poweroff(pmic) != 0) {
			printf("WARNING: AS3722 poweroff failed\n");
			return;
		}
	}
#endif
}
