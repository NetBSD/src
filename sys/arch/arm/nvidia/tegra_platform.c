/* $NetBSD: tegra_platform.c,v 1.1 2017/05/28 23:39:30 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_tegra.h"
#include "opt_multiprocessor.h"

#include "com.h"
#include "ukbd.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_platform.c,v 1.1 2017/05/28 23:39:30 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <arm/fdt/armv7_fdtvar.h>

#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif

#if NCOM > 0
#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#endif

#define	DEVMAP_ALIGN(a)	((a) & ~L1_S_OFFSET)
#define	DEVMAP_SIZE(s)	roundup2((s), L1_S_SIZE)
#define	DEVMAP_ENTRY(va, pa, sz)			\
	{						\
		.pd_va = DEVMAP_ALIGN(va),		\
		.pd_pa = DEVMAP_ALIGN(pa),		\
		.pd_size = DEVMAP_SIZE(sz),		\
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,	\
		.pd_cache = PTE_NOCACHE			\
	}
#define	DEVMAP_ENTRY_END	{ 0 }

static const struct pmap_devmap *
tegra_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(TEGRA_HOST1X_VBASE,
			     TEGRA_HOST1X_BASE,
			     TEGRA_HOST1X_SIZE),
		DEVMAP_ENTRY(TEGRA_PPSB_VBASE,
			     TEGRA_PPSB_BASE,
			     TEGRA_PPSB_SIZE),
		DEVMAP_ENTRY(TEGRA_APB_VBASE,
			     TEGRA_APB_BASE,
			     TEGRA_APB_SIZE),
		DEVMAP_ENTRY(TEGRA_AHB_A2_VBASE,
			     TEGRA_AHB_A2_BASE,
			     TEGRA_AHB_A2_SIZE),
		DEVMAP_ENTRY_END
	};	

	return devmap;
}

static void
tegra_platform_bootstrap(void)
{
	tegra_bootstrap();
}

static void
tegra_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	(CONSADDR - TEGRA_APB_BASE + TEGRA_APB_VBASE)
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = c;
#endif
}

static void
tegra_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "tegrafb") &&
	    match_bootconf_option(boot_args, "console", "fb")) {
		prop_dictionary_set_bool(dict, "is_console", true);
#if NUKBD > 0
		ukbd_cnattach();
#endif
	}

	if (device_is_a(self, "tegradrm")) {
		const char *video = get_bootconf_string(boot_args, "video");
		if (video)
			prop_dictionary_set_cstring(dict, "HDMI-A-1", video);
		if (match_bootconf_option(boot_args, "hdmi.forcemode", "dvi"))
			prop_dictionary_set_bool(dict, "force-dvi", true);
	}

	if (device_is_a(self, "tegracec"))
		prop_dictionary_set_cstring(dict, "hdmi-device", "tegradrm0");

	if (device_is_a(self, "nouveau")) {
		const char *config = get_bootconf_string(boot_args,
		    "nouveau.config");
		if (config)
			prop_dictionary_set_cstring(dict, "config", config);
		const char *debug = get_bootconf_string(boot_args,
		    "nouveau.debug");
		if (debug)
			prop_dictionary_set_cstring(dict, "debug", debug);
	}

	if (device_is_a(self, "tegrapcie")) {
		const char * const jetsontk1_compat[] = {
		    "nvidia,jetson-tk1", NULL
		};
		const int phandle = OF_peer(0);
		if (of_match_compatible(phandle, jetsontk1_compat)) {
			/* rfkill GPIO at GPIO X7 */
			struct tegra_gpio_pin *pin =
			    tegra_gpio_acquire("X7", GPIO_PIN_OUTPUT);
			if (pin)
				tegra_gpio_write(pin, 1);
		}
	}
}

static void
tegra_platform_reset(void)
{
	tegra_pmc_reset();
}

static void
tegra_platform_consinit(void)
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
	fdtbus_get_reg(fdtbus_get_stdout_phandle(), 0, &addr, NULL);
#endif

#ifdef CONSPEED
	speed = CONSPEED;
#else
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
#endif

	const bus_space_tag_t bst = &armv7_generic_a4x_bs_tag;
	const u_int freq = 408000000;	/* 408MHz PLLP_OUT0 */
	if (comcnattach(bst, addr, speed, freq, COM_TYPE_TEGRA, CONMODE))
		panic("Serial console cannot be initialized.");
#endif
}

static const struct armv7_platform tegra_platform = {
	.devmap = tegra_platform_devmap,
	.bootstrap = tegra_platform_bootstrap,
	.early_putchar = tegra_platform_early_putchar,
	.device_register = tegra_platform_device_register,
	.reset = tegra_platform_reset,
	.consinit = tegra_platform_consinit,
};

ARMV7_PLATFORM(tegra124, "nvidia,tegra124", &tegra_platform);
ARMV7_PLATFORM(tegra210, "nvidia,tegra210", &tegra_platform);
