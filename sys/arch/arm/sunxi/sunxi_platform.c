/* $NetBSD: sunxi_platform.c,v 1.46 2022/06/28 05:19:03 skrll Exp $ */

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
#include "opt_console.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_platform.c,v 1.46 2022/06/28 05:19:03 skrll Exp $");

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
#include <arm/fdt/psci_fdtvar.h>

#include <arm/sunxi/sunxi_platform.h>

#if defined(SOC_SUNXI_MC)
#include <arm/sunxi/sunxi_mc_smp.h>
#endif

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

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;

#define	sunxi_dma_tag		arm_generic_dma_tag
#define	sunxi_bs_tag		arm_generic_bs_tag

static bus_space_handle_t reset_bsh;

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

#define	SUNXI_MC_CPU_VBASE	(SUNXI_CORE_VBASE + SUNXI_CORE_SIZE)
#define	SUNXI_MC_CPU_PBASE	0x01700000
#define	SUNXI_MC_CPU_SIZE	0x00100000

static const struct pmap_devmap *
sun8i_a83t_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(SUNXI_CORE_VBASE,
			     SUNXI_CORE_PBASE,
			     SUNXI_CORE_SIZE),
		DEVMAP_ENTRY(SUNXI_MC_CPU_VBASE,
			     SUNXI_MC_CPU_PBASE,
			     SUNXI_MC_CPU_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

#define	SUN9I_A80_PRCM_VBASE	(SUNXI_MC_CPU_VBASE + SUNXI_MC_CPU_PBASE)
#define	SUN9I_A80_PRCM_PBASE	0x08000000
#define	SUN9I_A80_PRCM_SIZE	0x00100000

static const struct pmap_devmap *
sun9i_a80_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(SUNXI_CORE_VBASE,
			     SUNXI_CORE_PBASE,
			     SUNXI_CORE_SIZE),
		DEVMAP_ENTRY(SUNXI_MC_CPU_VBASE,
			     SUNXI_MC_CPU_PBASE,
			     SUNXI_MC_CPU_SIZE),
		DEVMAP_ENTRY(SUN9I_A80_PRCM_VBASE,
			     SUN9I_A80_PRCM_PBASE,
			     SUN9I_A80_PRCM_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}


static void
sunxi_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &sunxi_bs_tag;
	faa->faa_dmat = &sunxi_dma_tag;
}

void sunxi_platform_early_putchar(char);

void __noasan
sunxi_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - SUNXI_CORE_PBASE) + SUNXI_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#endif
}

static void
sunxi_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t prop = device_properties(self);
	int val;

	if (device_is_a(self, "rgephy")) {
		/* Pine64+ and NanoPi NEO Plus2 gigabit ethernet workaround */
		static const struct device_compatible_entry compat_data[] = {
			{ .compat = "pine64,pine64-plus" },
			{ .compat = "friendlyarm,nanopi-neo-plus2" },
			DEVICE_COMPAT_EOL
		};
		if (of_compatible_match(OF_finddevice("/"), compat_data)) {
			prop_dictionary_set_bool(prop, "no-rx-delay", true);
		}
	}

	if (device_is_a(self, "armgtmr")) {
		/* Allwinner A64 has an unstable architectural timer */
		static const struct device_compatible_entry compat_data[] = {
			{ .compat = "allwinner,sun50i-a64" },
			/* Cubietruck Plus triggers this problem as well. */
			{ .compat = "allwinner,sun8i-a83t" },
			DEVICE_COMPAT_EOL
		};
		if (of_compatible_match(OF_finddevice("/"), compat_data)) {
			prop_dictionary_set_bool(prop, "sun50i-a64-unstable-timer", true);
		}
	}

	if (device_is_a(self, "sunxidrm") || device_is_a(self, "dwhdmi")) {
		if (get_bootconf_option(boot_args, "nomodeset", BOOTOPT_TYPE_BOOLEAN, &val))
			if (val)
				prop_dictionary_set_bool(prop, "disabled", true);
	}

	if (device_is_a(self, "sun50ia64ccu0")) {
		if (get_bootconf_option(boot_args, "nomodeset", BOOTOPT_TYPE_BOOLEAN, &val))
			if (val)
				prop_dictionary_set_bool(prop, "nomodeset", true);
	}

	if (device_is_a(self, "com")) {
		static const struct device_compatible_entry compat_data[] = {
			{ .compat = "allwinner,sun4i-a10",	.value = 64 },
			{ .compat = "allwinner,sun5i-a13",	.value = 64 },
			{ .compat = "allwinner,sun6i-a31",	.value = 64 },
			{ .compat = "allwinner,sun7i-a20",	.value = 64 },
			{ .compat = "allwinner,sun8i-h2-plus",	.value = 64 },
			{ .compat = "allwinner,sun8i-h3",	.value = 64 },
			{ .compat = "allwinner,sun8i-a83t",	.value = 64 },
			{ .compat = "allwinner,sun9i-a80",	.value = 64 },
			{ .compat = "allwinner,sun50i-a64",	.value = 64 },
			{ .compat = "allwinner,sun50i-h5",	.value = 64 },
			{ .compat = "allwinner,sun50i-h6",	.value = 256 },
			DEVICE_COMPAT_EOL
		};
		const struct device_compatible_entry *dce =
		    of_compatible_lookup(OF_finddevice("/"), compat_data);
		if (dce != NULL)
			prop_dictionary_set_uint(prop, "fifolen", dce->value);
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
	arm_fdt_cpu_bootstrap();

	void *fdt_data = __UNCONST(fdtbus_get_data());
	const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
	if (chosen_off < 0)
		return;

	if (match_bootconf_option(boot_args, "console", "fb")) {
		const int framebuffer_off =
		    fdt_path_offset(fdt_data, "/chosen/framebuffer");
		if (framebuffer_off >= 0) {
			const char *status = fdt_getprop(fdt_data,
			    framebuffer_off, "status", NULL);
			if (status == NULL || strncmp(status, "ok", 2) == 0) {
				fdt_setprop_string(fdt_data, chosen_off,
				    "stdout-path", "/chosen/framebuffer");
			}
		}
	} else if (match_bootconf_option(boot_args, "console", "serial")) {
		fdt_setprop_string(fdt_data, chosen_off,
		    "stdout-path", "serial0:115200n8");
	}
}

static void
sun4i_platform_bootstrap(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	sunxi_platform_bootstrap();
	bus_space_map(bst, SUN4I_WDT_BASE, SUN4I_WDT_SIZE, 0, &reset_bsh);
}

static void
sun6i_platform_bootstrap(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	sunxi_platform_bootstrap();
	bus_space_map(bst, SUN6I_WDT_BASE, SUN6I_WDT_SIZE, 0, &reset_bsh);
}

static void
sun9i_platform_bootstrap(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	sunxi_platform_bootstrap();
	bus_space_map(bst, SUN9I_WDT_BASE, SUN9I_WDT_SIZE, 0, &reset_bsh);
}

static void
sun50i_h6_platform_bootstrap(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	sunxi_platform_bootstrap();
	bus_space_map(bst, SUN50I_H6_WDT_BASE, SUN50I_H6_WDT_SIZE, 0, &reset_bsh);
}

#if defined(SOC_SUNXI_MC)
static int
cpu_enable_sun8i_a83t(int phandle)
{
	uint64_t mpidr;

	fdtbus_get_reg64(phandle, 0, &mpidr, NULL);

	return sun8i_a83t_smp_enable(mpidr);
}
ARM_CPU_METHOD(sun8i_a83t, "allwinner,sun8i-a83t-smp", cpu_enable_sun8i_a83t);

static int
cpu_enable_sun9i_a80(int phandle)
{
	uint64_t mpidr;

	fdtbus_get_reg64(phandle, 0, &mpidr, NULL);

	return sun9i_a80_smp_enable(mpidr);
}
ARM_CPU_METHOD(sun9i_a80, "allwinner,sun9i-a80-smp", cpu_enable_sun9i_a80);
#endif

static void
sun4i_platform_reset(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	bus_space_write_4(bst, reset_bsh, SUN4I_WDT_CTRL,
	    SUN4I_WDT_CTRL_KEY | SUN4I_WDT_CTRL_RESTART);
	for (;;) {
		bus_space_write_4(bst, reset_bsh, SUN4I_WDT_MODE,
		    SUN4I_WDT_MODE_EN | SUN4I_WDT_MODE_RST_EN);
	}
}

static void
sun4i_platform_delay(u_int n)
{
	static bus_space_tag_t bst = &sunxi_bs_tag;
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
	bus_space_tag_t bst = &sunxi_bs_tag;

	bus_space_write_4(bst, reset_bsh, SUN6I_WDT_CFG, SUN6I_WDT_CFG_SYS);
	bus_space_write_4(bst, reset_bsh, SUN6I_WDT_MODE, SUN6I_WDT_MODE_EN);
}

static void
sun9i_platform_reset(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	bus_space_write_4(bst, reset_bsh, SUN9I_WDT_CFG, SUN9I_WDT_CFG_SYS);
	bus_space_write_4(bst, reset_bsh, SUN9I_WDT_MODE, SUN9I_WDT_MODE_EN);
}

static void
sun50i_h6_platform_reset(void)
{
	bus_space_tag_t bst = &sunxi_bs_tag;

	bus_space_write_4(bst, reset_bsh, SUN50I_H6_WDT_CFG, SUN50I_H6_WDT_CFG_SYS);
	bus_space_write_4(bst, reset_bsh, SUN50I_H6_WDT_MODE, SUN50I_H6_WDT_MODE_EN);
}

static const struct arm_platform sun4i_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun4i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun4i_platform_reset,
	.ap_delay = sun4i_platform_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun4i_a10, "allwinner,sun4i-a10", &sun4i_platform);

static const struct arm_platform sun5i_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun4i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun4i_platform_reset,
	.ap_delay = sun4i_platform_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun5i_a13, "allwinner,sun5i-a13", &sun5i_platform);
ARM_PLATFORM(sun5i_gr8, "nextthing,gr8", &sun5i_platform);

static const struct arm_platform sun6i_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun6i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun6i_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun6i_a31, "allwinner,sun6i-a31", &sun6i_platform);

static const struct arm_platform sun7i_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun4i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun4i_platform_reset,
	.ap_delay = sun4i_platform_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun7i_a20, "allwinner,sun7i-a20", &sun7i_platform);

static const struct arm_platform sun8i_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun6i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun6i_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun8i_h2plus, "allwinner,sun8i-h2-plus", &sun8i_platform);
ARM_PLATFORM(sun8i_h3, "allwinner,sun8i-h3", &sun8i_platform);
ARM_PLATFORM(sun8i_v3s, "allwinner,sun8i-v3s", &sun8i_platform);

static const struct arm_platform sun8i_a83t_platform = {
	.ap_devmap = sun8i_a83t_platform_devmap,
	.ap_bootstrap = sun6i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun6i_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun8i_a83t, "allwinner,sun8i-a83t", &sun8i_a83t_platform);

static const struct arm_platform sun9i_platform = {
	.ap_devmap = sun9i_a80_platform_devmap,
	.ap_bootstrap = sun9i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun9i_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun9i_a80, "allwinner,sun9i-a80", &sun9i_platform);

static const struct arm_platform sun50i_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun6i_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun6i_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun50i_a64, "allwinner,sun50i-a64", &sun50i_platform);
ARM_PLATFORM(sun50i_h5, "allwinner,sun50i-h5", &sun50i_platform);

static const struct arm_platform sun50i_h6_platform = {
	.ap_devmap = sunxi_platform_devmap,
	.ap_bootstrap = sun50i_h6_platform_bootstrap,
	.ap_init_attach_args = sunxi_platform_init_attach_args,
	.ap_device_register = sunxi_platform_device_register,
	.ap_reset = sun50i_h6_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = sunxi_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(sun50i_h6, "allwinner,sun50i-h6", &sun50i_h6_platform);
