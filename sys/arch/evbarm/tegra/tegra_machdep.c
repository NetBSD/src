/* $NetBSD: tegra_machdep.c,v 1.23 2015/10/21 20:02:13 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_machdep.c,v 1.23 2015/10/21 20:02:13 jmcneill Exp $");

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

#ifndef TEGRA_MAX_BOOT_STRING
#define TEGRA_MAX_BOOT_STRING 1024
#endif

BootConfig bootconfig;
char bootargs[TEGRA_MAX_BOOT_STRING] = "";
char *boot_args = NULL;
#ifdef TEGRA_UBOOT
u_int uboot_args[4] = { 0 };	/* filled in by tegra_start.S (not in bss) */
#endif

extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

static void tegra_device_register(device_t, void *);

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
	.bp_start = TEGRA_EXTMEM_BASE / NBPG,
	.bp_pages = (KERNEL_VM_BASE - KERNEL_BASE) / NBPG,
	.bp_freelist = VM_FREELIST_ISADMA,
	.bp_flags = 0
};
#endif

#ifdef VERBOSE_INIT_ARM
static void
tegra_putchar(char c)
{
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;
	int timo = 150000;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0) {
		if (--timo == 0)
			break;
	}

	uartaddr[com_data] = c;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0) {
		if (--timo == 0)
			break;
	}
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

	DPRINT(" consinit");
	consinit();

	DPRINTF(" cbar=%#x", armreg_cbar_read());

	DPRINTF(" ok\n");

#ifdef TEGRA_UBOOT
	DPRINTF("uboot: args %#x, %#x, %#x, %#x\n",
	    uboot_args[0], uboot_args[1], uboot_args[2], uboot_args[3]);
#endif

	cpu_reset_address = tegra_pmc_reset;

	/* Talk to the user */
	DPRINTF("\nNetBSD/evbarm (tegra) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	DPRINTF("KERNEL_BASE=0x%x, KERNEL_VM_BASE=0x%x, KERNEL_VM_BASE - KERNEL_BASE=0x%x, KERNEL_BASE_VOFFSET=0x%x\n",
		KERNEL_BASE, KERNEL_VM_BASE, KERNEL_VM_BASE - KERNEL_BASE, KERNEL_BASE_VOFFSET);

	ram_size = tegra_mc_memsize();

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
	bootconfig.dram[0].address = TEGRA_EXTMEM_BASE; /* DDR PHY addr */
	bootconfig.dram[0].pages = ram_size / PAGE_SIZE;

	KASSERT((armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0);

	arm32_bootmem_init(bootconfig.dram[0].address, ram_size,
	    KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0, devmap,
	    mapallmem_p);

	DPRINTF("bootargs: %s\n", bootargs);

	boot_args = bootargs;
	parse_mi_bootargs(boot_args);

	evbarm_device_register = tegra_device_register;

#ifdef PMAP_NEED_ALLOC_POOLPAGE
	if (atop(ram_size) > bp_lowgig.bp_pages) {
		arm_poolpage_vmfreelist = bp_lowgig.bp_freelist;
		return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
		    &bp_lowgig, 1);
	}
#endif

	return initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE, NULL, 0);

}

#if NCOM > 0
#ifndef CONSADDR
#error Specify the address of the console UART with the CONSADDR option.
#endif
#ifndef CONSPEED
#define CONSPEED 115200
#endif
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
	const bus_space_tag_t bst = &armv7_generic_a4x_bs_tag;
	const u_int freq = tegra_car_uart_rate(3);
	if (comcnattach(bst, CONSADDR, CONSPEED, freq,
			COM_TYPE_TEGRA, CONMODE)) {
		panic("Serial console cannot be initialized.");
	}
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

	strncpy(ret, s, i + 1);
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

	if (device_is_a(self, "cpu") && device_unit(self) == 0) {
		tegra_cpuinit();
	}

	if (device_is_a(self, "tegradc")
	    && tegra_bootconf_match("console", "fb")) {
		prop_dictionary_set_bool(dict, "is_console", true);
#if NUKBD > 0
		ukbd_cnattach();
#endif
	}

	if (device_is_a(self, "tegracec")) {
		prop_dictionary_set_cstring(dict, "hdmi-device", "tegrahdmi0");
	}

	if (device_is_a(self, "nouveau")) {
		const char *config = tegra_bootconf_strdup("nouveau.config");
		const char *debug = tegra_bootconf_strdup("nouveau.debug");
		if (config)
			prop_dictionary_set_cstring(dict, "config", config);
		if (debug)
			prop_dictionary_set_cstring(dict, "debug", debug);
	}

#ifdef SOC_TEGRA124
	if (device_is_a(self, "tegrausbphy")) {
		prop_dictionary_set_uint8(dict, "nvidia,hssync-start-delay", 0);
		prop_dictionary_set_uint8(dict, "nvidia,idle-wait-delay", 17);
		prop_dictionary_set_uint8(dict, "nvidia,elastic-limit", 16);
		prop_dictionary_set_uint8(dict, "nvidia,term-range-adj", 6);
		prop_dictionary_set_uint8(dict, "nvidia,xcvr-setup", 9);
		prop_dictionary_set_uint8(dict, "nvidia,xcvr-lsfslew", 0);
		prop_dictionary_set_uint8(dict, "nvidia,xcvr-lsrslew", 3);
		prop_dictionary_set_uint8(dict, "nvidia,hssquelch-level", 2);
		prop_dictionary_set_uint8(dict, "nvidia,hsdiscon-level", 5);
		prop_dictionary_set_uint8(dict, "nvidia,xcvr-hsslew", 12);
	}
#endif

#ifdef BOARD_JETSONTK1
	if (device_is_a(self, "sdhc")
	    && device_is_a(device_parent(self), "tegraio")) {
		struct tegraio_attach_args * const tio = aux;
		const struct tegra_locators * const loc = &tio->tio_loc;

		if (loc->loc_port == 2) {
			prop_dictionary_set_cstring(dict, "cd-gpio", "V2");
			prop_dictionary_set_cstring(dict, "power-gpio", "R0");
			prop_dictionary_set_cstring(dict, "wp-gpio", "Q4");
		}
	}

	if (device_is_a(self, "ahcisata")
	    && device_is_a(device_parent(self), "tegraio")) {
		prop_dictionary_set_cstring(dict, "power-gpio", "EE2");
	}

	if (device_is_a(self, "ehci")
	    && device_is_a(device_parent(self), "tegraio")) {
		struct tegraio_attach_args * const tio = aux;
		const struct tegra_locators * const loc = &tio->tio_loc;

		if (loc->loc_port == 0) {
			prop_dictionary_set_cstring(dict, "vbus-gpio", "N4");
		} else if (loc->loc_port == 2) {
			prop_dictionary_set_cstring(dict, "vbus-gpio", "N5");
		}
	}

	if (device_is_a(self, "tegrahdmi")) {
		prop_dictionary_set_cstring(dict, "hpd-gpio", "N7");
		prop_dictionary_set_cstring(dict, "pll-gpio", "H7");
		prop_dictionary_set_cstring(dict, "power-gpio", "K6");
		prop_dictionary_set_cstring(dict, "ddc-device", "ddc0");
		prop_dictionary_set_cstring(dict, "display-device", "tegradc1");
		if (tegra_bootconf_match("hdmi.forcemode", "dvi")) {
			prop_dictionary_set_bool(dict, "force-dvi", true);
		}
	}
#endif

#ifdef BOARD_NYAN_BIG
	if (device_is_a(self, "sdhc")
	    && device_is_a(device_parent(self), "tegraio")) {
		struct tegraio_attach_args * const tio = aux;
		const struct tegra_locators * const loc = &tio->tio_loc;

		if (loc->loc_port == 2) {
			prop_dictionary_set_cstring(dict, "cd-gpio", "V2");
			prop_dictionary_set_cstring(dict, "power-gpio", "R0");
		}
	}

	if (device_is_a(self, "ehci")
	    && device_is_a(device_parent(self), "tegraio")) {
		struct tegraio_attach_args * const tio = aux;
		const struct tegra_locators * const loc = &tio->tio_loc;

		if (loc->loc_port == 0) {
			prop_dictionary_set_cstring(dict, "vbus-gpio", "N4");
		} else if (loc->loc_port == 2) {
			prop_dictionary_set_cstring(dict, "vbus-gpio", "N5");
		}
	}

	if (device_is_a(self, "tegrahdmi")) {
		prop_dictionary_set_cstring(dict, "hpd-gpio", "N7");
		prop_dictionary_set_cstring(dict, "pll-gpio", "H7");
		prop_dictionary_set_cstring(dict, "power-gpio", "K6");
		prop_dictionary_set_cstring(dict, "ddc-device", "ddc0");
		prop_dictionary_set_cstring(dict, "display-device", "tegradc1");
	}
#endif
}
