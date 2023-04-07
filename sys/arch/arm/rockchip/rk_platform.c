/* $NetBSD: rk_platform.c,v 1.17 2023/04/07 08:55:30 skrll Exp $ */

/*-
 * Copyright (c) 2018,2021 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_platform.c,v 1.17 2023/04/07 08:55:30 skrll Exp $");

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

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdtvar.h>

#include <libfdt.h>

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;

static void
rk_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

static void
rk_platform_device_register(device_t self, void *aux)
{
}

static void
rk_platform_bootstrap(void)
{
	void *fdt_data = __UNCONST(fdtbus_get_data());

	arm_fdt_cpu_bootstrap();

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

#ifdef SOC_RK3288

#define	RK3288_WDT_BASE		0xff800000
#define	RK3288_WDT_SIZE		0x10000

#define	RK3288_WDT_CR		0x0000
#define	 RK3288_WDT_CR_WDT_EN		__BIT(0)
#define	RK3288_WDT_TORR		0x0004
#define	RK3288_WDT_CRR		0x000c
#define	 RK3288_WDT_MAGIC		0x76

static bus_space_handle_t rk3288_wdt_bsh;

#include <arm/rockchip/rk3288_platform.h>

static const struct pmap_devmap *
rk3288_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(RK3288_CORE_VBASE,
			     RK3288_CORE_PBASE,
			     RK3288_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

void rk3288_platform_early_putchar(char);

void __noasan
rk3288_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - RK3288_CORE_PBASE) + RK3288_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#undef CONSADDR_VA
#endif
}

static void
rk3288_platform_bootstrap(void)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;

	rk_platform_bootstrap();
	bus_space_map(bst, RK3288_WDT_BASE, RK3288_WDT_SIZE, 0, &rk3288_wdt_bsh);
}

static void
rk3288_platform_reset(void)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;

	bus_space_write_4(bst, rk3288_wdt_bsh, RK3288_WDT_TORR, 0);
	bus_space_write_4(bst, rk3288_wdt_bsh, RK3288_WDT_CRR, RK3288_WDT_MAGIC);
	for (;;) {
		bus_space_write_4(bst, rk3288_wdt_bsh, RK3288_WDT_CR, RK3288_WDT_CR_WDT_EN);
	}
}

static u_int
rk3288_platform_uart_freq(void)
{
	return RK3288_UART_FREQ;
}

static const struct fdt_platform rk3288_platform = {
	.fp_devmap = rk3288_platform_devmap,
	.fp_bootstrap = rk3288_platform_bootstrap,
	.fp_init_attach_args = rk_platform_init_attach_args,
	.fp_device_register = rk_platform_device_register,
	.fp_reset = rk3288_platform_reset,
	.fp_delay = gtmr_delay,
	.fp_uart_freq = rk3288_platform_uart_freq,
	.fp_mpstart = arm_fdt_cpu_mpstart,
};

FDT_PLATFORM(rk3288, "rockchip,rk3288", &rk3288_platform);
#endif /* SOC_RK3288 */


#ifdef SOC_RK3328

#include <arm/rockchip/rk3328_platform.h>

static const struct pmap_devmap *
rk3328_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(RK3328_CORE_VBASE,
			     RK3328_CORE_PBASE,
			     RK3328_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

void rk3328_platform_early_putchar(char);

void __noasan
rk3328_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - RK3328_CORE_PBASE) + RK3328_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#undef CONSADDR_VA
#endif
}

static u_int
rk3328_platform_uart_freq(void)
{
	return RK3328_UART_FREQ;
}

static const struct fdt_platform rk3328_platform = {
	.fp_devmap = rk3328_platform_devmap,
	.fp_bootstrap = rk_platform_bootstrap,
	.fp_init_attach_args = rk_platform_init_attach_args,
	.fp_device_register = rk_platform_device_register,
	.fp_reset = psci_fdt_reset,
	.fp_delay = gtmr_delay,
	.fp_uart_freq = rk3328_platform_uart_freq,
	.fp_mpstart = arm_fdt_cpu_mpstart,
};

FDT_PLATFORM(rk3328, "rockchip,rk3328", &rk3328_platform);

#endif /* SOC_RK3328 */


#ifdef SOC_RK3399

#include <arm/rockchip/rk3399_platform.h>

static const struct pmap_devmap *
rk3399_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(RK3399_CORE_VBASE,
			     RK3399_CORE_PBASE,
			     RK3399_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

void rk3399_platform_early_putchar(char);

void
rk3399_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - RK3399_CORE_PBASE) + RK3399_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#undef CONSADDR_VA
#endif
}

static u_int
rk3399_platform_uart_freq(void)
{
	return RK3399_UART_FREQ;
}

static const struct fdt_platform rk3399_platform = {
	.fp_devmap = rk3399_platform_devmap,
	.fp_bootstrap = rk_platform_bootstrap,
	.fp_init_attach_args = rk_platform_init_attach_args,
	.fp_device_register = rk_platform_device_register,
	.fp_reset = psci_fdt_reset,
	.fp_delay = gtmr_delay,
	.fp_uart_freq = rk3399_platform_uart_freq,
	.fp_mpstart = arm_fdt_cpu_mpstart,
};

FDT_PLATFORM(rk3399, "rockchip,rk3399", &rk3399_platform);

#endif /* SOC_RK3399 */


#ifdef SOC_RK3588

#include <arm/rockchip/rk3588_platform.h>

static const struct pmap_devmap *
rk3588_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(
		    RK3588_CORE_VBASE,
		    RK3588_CORE_PBASE,
		    RK3588_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

void rk3588_platform_early_putchar(char);

void
rk3588_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - RK3588_CORE_PBASE) + RK3588_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#undef CONSADDR_VA
#endif
}

static u_int
rk3588_platform_uart_freq(void)
{
	return RK3588_UART_FREQ;
}

static const struct fdt_platform rk3588_platform = {
	.fp_devmap = rk3588_platform_devmap,
	.fp_bootstrap = rk_platform_bootstrap,
	.fp_init_attach_args = rk_platform_init_attach_args,
	.fp_device_register = rk_platform_device_register,
	.fp_reset = psci_fdt_reset,
	.fp_delay = gtmr_delay,
	.fp_uart_freq = rk3588_platform_uart_freq,
	.fp_mpstart = arm_fdt_cpu_mpstart,
};

FDT_PLATFORM(rk3588, "rockchip,rk3588", &rk3588_platform);

#endif /* SOC_RK3588 */

