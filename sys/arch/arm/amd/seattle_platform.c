/* $NetBSD: seattle_platform.c,v 1.2 2018/09/10 19:09:38 jmcneill Exp $ */

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

#define _ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: seattle_platform.c,v 1.2 2018/09/10 19:09:38 jmcneill Exp $");

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

#include <evbarm/fdt/platform.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdtvar.h>

#include <libfdt.h>

#define	SEATTLE_CORE_VBASE	KERNEL_IO_VBASE
#define	SEATTLE_CORE_PBASE	0xe0000000
#define	SEATTLE_CORE_SIZE	0x02000000

#define	SEATTLE_UART_FREQ	100000000

extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

static struct arm32_dma_range seattle_dma_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = 0,
		.dr_len = 0x10000000000,
		.dr_flags = _BUS_DMAMAP_COHERENT,
	}
};

static struct arm32_bus_dma_tag seattle_bus_dma_tag = {
	._ranges = seattle_dma_ranges,
	._nranges = __arraycount(seattle_dma_ranges),
        _BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

static void
seattle_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
	faa->faa_dmat = &seattle_bus_dma_tag;
}

static void
seattle_platform_device_register(device_t self, void *aux)
{
}

static const struct pmap_devmap *
seattle_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(SEATTLE_CORE_VBASE,
			     SEATTLE_CORE_PBASE,
			     SEATTLE_CORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

void seattle_platform_early_putchar(char);

void
seattle_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - SEATTLE_CORE_PBASE) + SEATTLE_CORE_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((uartaddr[PL01XCOM_FR / 4] & PL01X_FR_TXFF) != 0)
		continue;

	uartaddr[PL01XCOM_DR / 4] = c;

	while ((uartaddr[PL01XCOM_FR / 4] & PL01X_FR_TXFE) == 0)
		continue;
#undef CONSADDR_VA
#endif
}

static u_int
seattle_platform_uart_freq(void)
{
	return SEATTLE_UART_FREQ;
}

static const struct arm_platform seattle_platform = {
	.ap_devmap = seattle_platform_devmap,
	.ap_bootstrap = arm_fdt_cpu_bootstrap,
	.ap_init_attach_args = seattle_platform_init_attach_args,
	.ap_early_putchar = seattle_platform_early_putchar,
	.ap_device_register = seattle_platform_device_register,
	.ap_reset = psci_fdt_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = seattle_platform_uart_freq,
};

ARM_PLATFORM(seattle, "amd,seattle", &seattle_platform);
