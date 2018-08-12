/* $NetBSD: rk_platform.c,v 1.3 2018/08/12 16:48:05 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_platform.c,v 1.3 2018/08/12 16:48:05 jmcneill Exp $");

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
#include <arm/fdt/psci_fdt.h>

#include <libfdt.h>

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

static void
rk_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
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

	psci_fdt_bootstrap();

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

void
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

static const struct arm_platform rk3328_platform = {
	.ap_devmap = rk3328_platform_devmap,
	.ap_bootstrap = rk_platform_bootstrap,
	.ap_init_attach_args = rk_platform_init_attach_args,
	.ap_early_putchar = rk3328_platform_early_putchar,
	.ap_device_register = rk_platform_device_register,
	.ap_reset = psci_fdt_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = rk3328_platform_uart_freq,
};

ARM_PLATFORM(rk3328, "rockchip,rk3328", &rk3328_platform);

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

static const struct arm_platform rk3399_platform = {
	.ap_devmap = rk3399_platform_devmap,
	.ap_bootstrap = rk_platform_bootstrap,
	.ap_init_attach_args = rk_platform_init_attach_args,
	.ap_early_putchar = rk3399_platform_early_putchar,
	.ap_device_register = rk_platform_device_register,
	.ap_reset = psci_fdt_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = rk3399_platform_uart_freq,
};

ARM_PLATFORM(rk3399, "rockchip,rk3399", &rk3399_platform);

#endif /* SOC_RK3399 */
