/* $NetBSD: imx23_platform.c,v 1.1 2025/10/09 06:15:16 skrll Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

/*
 * Platform code for the NXP i.MX23 SOC.
 *
 * This code is partially based on imx23_olinuxino_machdep.c
 */

#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdt_platform.h>
#include <dev/fdt/fdtvar.h>

#include <arch/evbarm/fdt/platform.h>
#include <arm/cpufunc.h>
#include <arm/fdt/arm_fdtvar.h>
#include <arm/imx/imx23_clkctrlreg.h>
#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23var.h>
#include <evbarm/dev/plcomreg.h>

#include "opt_console.h"

#define REG_RD(reg) *(volatile uint32_t *)(reg)
#define REG_WR(reg, val)						\
do {									\
	*(volatile uint32_t *)((reg)) = val;				\
} while (0)

void imx23_platform_early_putchar(char);

static const struct pmap_devmap *imx23_platform_devmap(void);

void
imx23_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA ((CONSADDR - APBH_BASE) + KERNEL_IO_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p()
					  ? (volatile uint32_t *)CONSADDR_VA
					  : (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFF) != 0)
		continue;

	uartaddr[PL01XCOM_DR / 4] = htole32(c);
	dsb(sy);

	while ((le32toh(uartaddr[PL01XCOM_FR / 4]) & PL01X_FR_TXFE) == 0)
		continue;
#endif
}

static const struct pmap_devmap *
imx23_platform_devmap(void) {
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(
			APBH_BASE,    /* Virtual address. */
			APBH_BASE,    /* Physical address. */
			APBH_SIZE + APBX_SIZE  /* APBX located after APBH. */
		),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
imx23_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &imx23_bus_space;
	faa->faa_dmat = &imx23_bus_dma_tag;
}

static void
imx23_platform_device_register(device_t self, void *aux)
{
}

static void
imx23_platform_reset(void)
{
	// reboot
	REG_WR(HW_CLKCTRL_BASE + HW_CLKCTRL_RESET, HW_CLKCTRL_RESET_CHIP);
}

/*
 * Delay us microseconds.
 */
static void
imx23_platform_delay(unsigned int us)
{
	uint32_t start;
	uint32_t now;
	uint32_t elapsed;
	uint32_t total;
	uint32_t last;

	total = 0;
	last = 0;
	start = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

	do {
		now = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

		if (start <= now)
			elapsed = now - start;
		else	/* Take care of overflow. */
			elapsed = (UINT32_MAX - start) + 1 + now;

		total += elapsed - last;
		last = elapsed;

	} while (total < us);

	return;
}

static u_int
imx23_platform_uart_freq(void)
{
	/* The documentation mentions the uart is driven by a 24MHz clock. */
	return 24000000;
}

static const struct fdt_platform imx23_platform = {
	.fp_devmap = imx23_platform_devmap,
	.fp_bootstrap = arm_fdt_cpu_bootstrap,
	.fp_init_attach_args = imx23_platform_init_attach_args,
	.fp_device_register = imx23_platform_device_register,
	.fp_reset = imx23_platform_reset,
	.fp_delay = imx23_platform_delay,
	.fp_uart_freq = imx23_platform_uart_freq,
};

FDT_PLATFORM(imx23, "fsl,imx23", &imx23_platform);
