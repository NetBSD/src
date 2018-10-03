/*	$NetBSD: thunderx_platform.c,v 1.1 2018/10/03 09:24:36 skrll Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_console.h"
#include "opt_soc.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: thunderx_platform.c,v 1.1 2018/10/03 09:24:36 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

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
#include <arm/fdt/psci_fdtvar.h>

#include <arm/cavium/thunderx_platform.h>

void thunderx_platform_early_putchar(char);

static const struct pmap_devmap *
thunderx_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(THUNDERX_CORE_VBASE, THUNDERX_CORE_PBASE, THUNDERX_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
thunderx_platform_init_attach_args(struct fdt_attach_args *faa)
{
	extern struct arm32_bus_dma_tag arm_generic_dma_tag;
	extern struct bus_space arm_generic_bs_tag;
	extern struct bus_space arm_generic_a4x_bs_tag;

	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void
thunderx_platform_early_putchar(char c)
{
#ifdef CONSADDR
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFF) != 0)
		continue;

	uartaddr[PL01XCOM_DR / 4] = htole32(c);
	arm_dsb();

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFE) == 0)
		continue;
#endif
}

static void
thunderx_platform_device_register(device_t self, void *aux)
{
}

static u_int
thunderx_platform_uart_freq(void)
{

	return 133315200;
}

static const struct arm_platform thunderx_platform = {
	.ap_devmap = thunderx_platform_devmap,
	.ap_bootstrap = arm_fdt_cpu_bootstrap,
	.ap_init_attach_args = thunderx_platform_init_attach_args,
	.ap_early_putchar = thunderx_platform_early_putchar,
	.ap_device_register = thunderx_platform_device_register,
	.ap_reset = psci_fdt_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = thunderx_platform_uart_freq,
};

ARM_PLATFORM(thunderx, "cavium,thunder-88xx", &thunderx_platform);
