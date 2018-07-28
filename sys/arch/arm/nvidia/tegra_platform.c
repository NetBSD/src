/* $NetBSD: tegra_platform.c,v 1.9.2.3 2018/07/28 04:37:28 pgoyette Exp $ */

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
#include "opt_fdt_arm.h"

#include "ukbd.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_platform.c,v 1.9.2.3 2018/07/28 04:37:28 pgoyette Exp $");

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
#include <arm/nvidia/tegra_platform.h>

#include <arm/fdt/arm_fdtvar.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdt.h>

#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#define	PLLP_OUT0_FREQ	408000000

void tegra_platform_early_putchar(char);

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

#ifdef SOC_TEGRA124
static void
tegra124_platform_bootstrap(void)
{
	tegra_bootstrap();

#ifdef MULTIPROCESSOR
	tegra124_mpinit();
#endif
}
#endif

#ifdef SOC_TEGRA210
static void
tegra210_platform_bootstrap(void)
{
	tegra_bootstrap();

#if defined(MULTIPROCESSOR) && defined(__aarch64__)
	psci_fdt_bootstrap();
#endif
}
#endif

static void
tegra_platform_init_attach_args(struct fdt_attach_args *faa)
{
	extern struct bus_space arm_generic_bs_tag;
	extern struct bus_space arm_generic_a4x_bs_tag;
	extern struct arm32_bus_dma_tag arm_generic_dma_tag;

	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void
tegra_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	(CONSADDR - TEGRA_APB_BASE + TEGRA_APB_VBASE)

	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

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
tegra_platform_delay(u_int us)
{
	tegra_timer_delay(us);
}

static u_int
tegra_platform_uart_freq(void)
{
	return PLLP_OUT0_FREQ;
}

#ifdef SOC_TEGRA124
static const struct arm_platform tegra124_platform = {
	.devmap = tegra_platform_devmap,
	.bootstrap = tegra124_platform_bootstrap,
	.init_attach_args = tegra_platform_init_attach_args,
	.early_putchar = tegra_platform_early_putchar,
	.device_register = tegra_platform_device_register,
	.reset = tegra_platform_reset,
	.delay = tegra_platform_delay,
	.uart_freq = tegra_platform_uart_freq,
};

ARM_PLATFORM(tegra124, "nvidia,tegra124", &tegra124_platform);
#endif

#ifdef SOC_TEGRA210
static const struct arm_platform tegra210_platform = {
	.devmap = tegra_platform_devmap,
	.bootstrap = tegra210_platform_bootstrap,
	.init_attach_args = tegra_platform_init_attach_args,
	.early_putchar = tegra_platform_early_putchar,
	.device_register = tegra_platform_device_register,
	.reset = tegra_platform_reset,
	.delay = tegra_platform_delay,
	.uart_freq = tegra_platform_uart_freq,
};

ARM_PLATFORM(tegra210, "nvidia,tegra210", &tegra210_platform);
#endif
