/* $NetBSD: virt_platform.c,v 1.4.2.2 2018/06/25 07:25:40 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: virt_platform.c,v 1.4.2.2 2018/06/25 07:25:40 pgoyette Exp $");

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
#include <arm/locore.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <arm/cortex/gtmr_var.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdt.h>

#include <arm/virt/virt_platform.h>

#define	VIRT_UART_BASE	0x09000000

static const struct pmap_devmap *
virt_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(VIRT_CORE_VBASE,
			     VIRT_CORE_PBASE,
			     VIRT_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
virt_platform_init_attach_args(struct fdt_attach_args *faa)
{
	extern struct arm32_bus_dma_tag arm_generic_dma_tag;
	extern struct bus_space arm_generic_bs_tag;
	extern struct bus_space arm_generic_a4x_bs_tag;

	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void virt_platform_early_putchar(char);

void
virt_platform_early_putchar(char c)
{
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
		(volatile uint32_t *)VIRT_CORE_PTOV(VIRT_UART_BASE) :
		(volatile uint32_t *)VIRT_UART_BASE;

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFF) != 0)
		continue;

	uartaddr[PL01XCOM_DR / 4] = htole32(c);
	arm_dsb();

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFE) == 0)
		continue;
}

static void
virt_platform_device_register(device_t self, void *aux)
{
}

static u_int
virt_platform_uart_freq(void)
{
	return 24000000;
}

static const struct arm_platform virt_platform = {
	.devmap = virt_platform_devmap,
	.bootstrap = psci_fdt_bootstrap,
	.init_attach_args = virt_platform_init_attach_args,
	.early_putchar = virt_platform_early_putchar,
	.device_register = virt_platform_device_register,
	.reset = psci_fdt_reset,
	.delay = gtmr_delay,
	.uart_freq = virt_platform_uart_freq,
};

ARM_PLATFORM(virt, "linux,dummy-virt", &virt_platform);
