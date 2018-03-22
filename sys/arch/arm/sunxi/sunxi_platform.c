/* $NetBSD: sunxi_platform.c,v 1.20.2.1 2018/03/22 01:44:43 pgoyette Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"
#include "opt_multiprocessor.h"
#include "opt_fdt_arm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_platform.c,v 1.20.2.1 2018/03/22 01:44:43 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/cortex/gtmr_var.h>
#include <arm/cortex/gic_reg.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdt.h>

#include <arm/sunxi/sunxi_platform.h>

#include <libfdt.h>

#define	SUNXI_REF_FREQ	24000000

#define	SUN4I_TIMER_BASE	0x01c20c00
#define	SUN4I_TIMER_SIZE	0x90
#define	SUN4I_TIMER_1_CTRL	0x20
#define	 SUN4I_TIMER_1_CTRL_CLK_SRC	__BITS(3,2)
#define	 SUN4I_TIMER_1_CTRL_CLK_SRC_OSC24M	1
#define	 SUN4I_TIMER_1_CTRL_RELOAD	__BIT(1)
#define	 SUN4I_TIMER_1_CTRL_EN		__BIT(0)
#define	SUN4I_TIMER_1_INTV_VALUE 0x24
#define	SUN4I_TIMER_1_VAL	0x28

#define	SUN4I_WDT_BASE		0x01c20c90
#define	SUN4I_WDT_SIZE		0x10
#define	SUN4I_WDT_CTRL		0x00
#define	 SUN4I_WDT_CTRL_KEY	(0x333 << 1)
#define	 SUN4I_WDT_CTRL_RESTART	__BIT(0)
#define	SUN4I_WDT_MODE		0x04
#define	 SUN4I_WDT_MODE_RST_EN	__BIT(1)
#define	 SUN4I_WDT_MODE_EN	__BIT(0)

#define	SUN6I_WDT_BASE		0x01c20ca0
#define	SUN6I_WDT_SIZE		0x20
#define	SUN6I_WDT_CFG		0x14
#define	 SUN6I_WDT_CFG_SYS	__BIT(0)
#define	SUN6I_WDT_MODE		0x18
#define	 SUN6I_WDT_MODE_EN	__BIT(0)

#define	SUN9I_WDT_BASE		0x06000ca0
#define	SUN9I_WDT_SIZE		0x20
#define	SUN9I_WDT_CFG		0x14
#define	 SUN9I_WDT_CFG_SYS	__BIT(0)
#define	SUN9I_WDT_MODE		0x18
#define	 SUN9I_WDT_MODE_EN	__BIT(0)

#define	SUN50I_H6_WDT_BASE	0x01c20ca0
#define	SUN50I_H6_WDT_SIZE	0x20
#define	SUN50I_H6_WDT_CFG	0x14
#define	 SUN50I_H6_WDT_CFG_SYS	__BIT(0)
#define	SUN50I_H6_WDT_MODE	0x18
#define	 SUN50I_H6_WDT_MODE_EN	__BIT(0)

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

static const struct pmap_devmap *
sunxi_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(SUNXI_CORE_VBASE,
			     SUNXI_CORE_PBASE,
			     SUNXI_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
sunxi_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_a4x_bst = &armv7_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

static void
sunxi_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA     ((CONSADDR - SUNXI_CORE_PBASE) + SUNXI_CORE_VBASE)
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#endif
}

static void
sunxi_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t prop = device_properties(self);

	if (device_is_a(self, "rgephy")) {
		/* Pine64+ and NanoPi NEO Plus2 gigabit ethernet workaround */
		const char * compat[] = {
			"pine64,pine64-plus",
			"friendlyarm,nanopi-neo-plus2",
			NULL
		};
		if (of_match_compatible(OF_finddevice("/"), compat)) {
			prop_dictionary_set_bool(prop, "no-rx-delay", true);
		}
	}
}

static u_int
sunxi_platform_uart_freq(void)
{
	return SUNXI_REF_FREQ;
}

static void
sunxi_platform_bootstrap(void)
{
	if (match_bootconf_option(boot_args, "console", "fb")) {
		void *fdt_data = __UNCONST(fdtbus_get_data());
		const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
		const int framebuffer_off =
		    fdt_path_offset(fdt_data, "/chosen/framebuffer");
		if (chosen_off >= 0 && framebuffer_off >= 0)
			fdt_setprop_string(fdt_data, chosen_off, "stdout-path",
			    "/chosen/framebuffer");
	}
}

static void
sunxi_platform_psci_bootstrap(void)
{
	psci_fdt_bootstrap();
	sunxi_platform_bootstrap();
}

static void
sun4i_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, SUN4I_WDT_BASE, SUN4I_WDT_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, SUN4I_WDT_CTRL,
	    SUN4I_WDT_CTRL_KEY | SUN4I_WDT_CTRL_RESTART);
	for (;;) {
		bus_space_write_4(bst, bsh, SUN4I_WDT_MODE,
		    SUN4I_WDT_MODE_EN | SUN4I_WDT_MODE_RST_EN);
	}
}

static void
sun4i_platform_delay(u_int n)
{
	static bus_space_tag_t bst = &armv7_generic_bs_tag;
	static bus_space_handle_t bsh = 0;
	const long incs_per_us = SUNXI_REF_FREQ / 1000000;
	long ticks = n * incs_per_us;
	uint32_t cur, prev;

	if (bsh == 0) {
		bus_space_map(bst, SUN4I_TIMER_BASE, SUN4I_TIMER_SIZE, 0, &bsh);

		/* Enable Timer 1 */
		bus_space_write_4(bst, bsh, SUN4I_TIMER_1_INTV_VALUE, ~0U);
		bus_space_write_4(bst, bsh, SUN4I_TIMER_1_CTRL,
		    SUN4I_TIMER_1_CTRL_EN |
		    SUN4I_TIMER_1_CTRL_RELOAD |
		    __SHIFTIN(SUN4I_TIMER_1_CTRL_CLK_SRC_OSC24M,
			      SUN4I_TIMER_1_CTRL_CLK_SRC));
	}

	prev = ~bus_space_read_4(bst, bsh, SUN4I_TIMER_1_VAL);
	while (ticks > 0) {
		cur = ~bus_space_read_4(bst, bsh, SUN4I_TIMER_1_VAL);
		if (cur > prev)
			ticks -= (cur - prev);
		else
			ticks -= (~0U - cur + prev);
		prev = cur;
	}
}

static void
sun6i_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, SUN6I_WDT_BASE, SUN6I_WDT_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, SUN6I_WDT_CFG, SUN6I_WDT_CFG_SYS);
	bus_space_write_4(bst, bsh, SUN6I_WDT_MODE, SUN6I_WDT_MODE_EN);
}

static void
sun9i_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, SUN9I_WDT_BASE, SUN9I_WDT_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, SUN9I_WDT_CFG, SUN9I_WDT_CFG_SYS);
	bus_space_write_4(bst, bsh, SUN9I_WDT_MODE, SUN9I_WDT_MODE_EN);
}

static void
sun50i_h6_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, SUN50I_H6_WDT_BASE, SUN50I_H6_WDT_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, SUN50I_H6_WDT_CFG, SUN50I_H6_WDT_CFG_SYS);
	bus_space_write_4(bst, bsh, SUN50I_H6_WDT_MODE, SUN50I_H6_WDT_MODE_EN);
}

static const struct arm_platform sun4i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun4i_platform_reset,
	.delay = sun4i_platform_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun4i_a10, "allwinner,sun4i-a10", &sun4i_platform);

static const struct arm_platform sun5i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun4i_platform_reset,
	.delay = sun4i_platform_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun5i_a13, "allwinner,sun5i-a13", &sun5i_platform);
ARM_PLATFORM(sun5i_gr8, "nextthing,gr8", &sun5i_platform);

static const struct arm_platform sun6i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_psci_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun6i_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun6i_a31, "allwinner,sun6i-a31", &sun6i_platform);

static const struct arm_platform sun7i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_psci_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun4i_platform_reset,
	.delay = sun4i_platform_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun7i_a20, "allwinner,sun7i-a20", &sun7i_platform);

static const struct arm_platform sun8i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_psci_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun6i_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun8i_h2plus, "allwinner,sun8i-h2-plus", &sun8i_platform);
ARM_PLATFORM(sun8i_h3, "allwinner,sun8i-h3", &sun8i_platform);
ARM_PLATFORM(sun8i_a83t, "allwinner,sun8i-a83t", &sun8i_platform);

static const struct arm_platform sun9i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun9i_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun9i_a80, "allwinner,sun9i-a80", &sun9i_platform);

static const struct arm_platform sun50i_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun6i_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun50i_a64, "allwinner,sun50i-a64", &sun50i_platform);
ARM_PLATFORM(sun50i_h5, "allwinner,sun50i-h5", &sun50i_platform);

static const struct arm_platform sun50i_h6_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = sunxi_platform_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun50i_h6_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun50i_h6, "allwinner,sun50i-h6", &sun50i_h6_platform);
