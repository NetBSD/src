/* $NetBSD: arm_platform.c,v 1.7 2023/04/07 08:55:30 skrll Exp $ */

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

#include "opt_console.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm_platform.c,v 1.7 2023/04/07 08:55:30 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>

#include <arm/cpufunc.h>

#include <arm/cortex/gtmr_var.h>

#include <arm/arm/psci.h>

#include <arm/fdt/arm_fdtvar.h>
#include <arm/fdt/psci_fdtvar.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <libfdt.h>

#include <arch/evbarm/fdt/platform.h>

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;

void plcom_platform_early_putchar(char);

#define	ARM_PTOV(p)       (((p) - DEVMAP_ALIGN(uart_base)) + KERNEL_IO_VBASE)

void __noasan
plcom_platform_early_putchar(char c)
{
#ifdef CONSADDR
	bus_addr_t uart_base = CONSADDR;

	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
		(volatile uint32_t *)ARM_PTOV(uart_base):
		(volatile uint32_t *)uart_base;

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFF) != 0)
		continue;

	uartaddr[PL01XCOM_DR / 4] = htole32(c);
	dsb(sy);

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFE) == 0)
		continue;
#endif
}

static void
arm_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
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

	const int phandle = fdtbus_get_stdout_phandle();
	if (phandle <= 0)
		return devmap_empty;

	bus_addr_t uart_base;
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

static const struct fdt_platform arm_platform = {
	.fp_devmap = arm_platform_devmap,
	.fp_bootstrap = arm_fdt_cpu_bootstrap,
	.fp_init_attach_args = arm_platform_init_attach_args,
	.fp_device_register = arm_platform_device_register,
	.fp_reset = psci_fdt_reset,
	.fp_delay = gtmr_delay,
	.fp_uart_freq = arm_platform_uart_freq,
	.fp_mpstart = arm_fdt_cpu_mpstart,
};

FDT_PLATFORM(arm, FDT_PLATFORM_DEFAULT, &arm_platform);
