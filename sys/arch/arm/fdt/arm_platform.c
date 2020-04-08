/* $NetBSD: arm_platform.c,v 1.2.6.2 2020/04/08 14:07:29 martin Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * This is the default Arm FDT platform implementation. It assumes the
 * following:
 *
 *  - Generic timer
 *  - PSCI support
 *  - Console UART is pre-configured by firmware
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm_platform.c,v 1.2.6.2 2020/04/08 14:07:29 martin Exp $");

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

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdtvar.h>

#include <libfdt.h>

#include <arch/evbarm/fdt/platform.h>

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

static void
arm_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

static void
arm_platform_device_register(device_t self, void *aux)
{
}

static const struct pmap_devmap *
arm_platform_devmap(void)
{
	static const struct pmap_devmap devmap_empty[] = {
		DEVMAP_ENTRY_END
	};
	static struct pmap_devmap devmap_uart[] = {
		DEVMAP_ENTRY(KERNEL_IO_VBASE, 0, PAGE_SIZE),
		DEVMAP_ENTRY_END
	};
	bus_addr_t uart_base;

	const int phandle = fdtbus_get_stdout_phandle();
	if (phandle <= 0)
		return devmap_empty;

	if (fdtbus_get_reg(phandle, 0, &uart_base, NULL) != 0)
		return devmap_empty;

	devmap_uart[0].pd_pa = DEVMAP_ALIGN(uart_base);

	return devmap_uart;
}

static u_int
arm_platform_uart_freq(void)
{
	return 0;
}

static const struct arm_platform arm_platform = {
	.ap_devmap = arm_platform_devmap,
	.ap_bootstrap = arm_fdt_cpu_bootstrap,
	.ap_init_attach_args = arm_platform_init_attach_args,
	.ap_device_register = arm_platform_device_register,
	.ap_reset = psci_fdt_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = arm_platform_uart_freq,
	.ap_mpstart = arm_fdt_cpu_mpstart,
};

ARM_PLATFORM(arm, ARM_PLATFORM_DEFAULT, &arm_platform);
