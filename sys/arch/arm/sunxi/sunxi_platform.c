/* $NetBSD: sunxi_platform.c,v 1.2 2017/06/30 09:05:52 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_platform.c,v 1.2 2017/06/30 09:05:52 jmcneill Exp $");

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

#define	SUNXI_REF_FREQ	24000000

#define	SUN8I_WDT_BASE		0x01c20ca0
#define	SUN8I_WDT_SIZE		0x20
#define	SUN8I_WDT_CFG		0x14
#define	 SUN8I_WDT_CFG_SYS	1
#define	SUN8I_WDT_MODE		0x18
#define	 SUN8I_WDT_MODE_EN	1


#define	DEVMAP_ALIGN(a)	((a) & ~L1_S_OFFSET)
#define	DEVMAP_SIZE(s)	roundup2((s), L1_S_SIZE)
#define	DEVMAP_ENTRY(va, pa, sz)			\
	{						\
		.pd_va = DEVMAP_ALIGN(va),		\
		.pd_pa = DEVMAP_ALIGN(pa),		\
		.pd_size = DEVMAP_SIZE(sz),		\
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,	\
		.pd_cache = PTE_NOCACHE			\
	}
#define	DEVMAP_ENTRY_END	{ 0 }

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag armv7_generic_dma_tag;

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
	faa->faa_dmat = &armv7_generic_dma_tag;
}

static void
sunxi_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA     ((CONSADDR - SUNXI_CORE_PBASE) + SUNXI_CORE_VBASE)
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = c;
#endif
}

static void
sunxi_platform_device_register(device_t self, void *aux)
{
}

static void
sun8i_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_map(bst, SUN8I_WDT_BASE, SUN8I_WDT_SIZE, 0, &bsh);

	bus_space_write_4(bst, bsh, SUN8I_WDT_CFG, SUN8I_WDT_CFG_SYS);
	bus_space_write_4(bst, bsh, SUN8I_WDT_MODE, SUN8I_WDT_MODE_EN);
}

static u_int
sunxi_platform_uart_freq(void)
{
	return SUNXI_REF_FREQ;
}

static const struct arm_platform sun8i_h3_platform = {
	.devmap = sunxi_platform_devmap,
	.bootstrap = psci_fdt_bootstrap,
	.init_attach_args = sunxi_platform_init_attach_args,
	.early_putchar = sunxi_platform_early_putchar,
	.device_register = sunxi_platform_device_register,
	.reset = sun8i_platform_reset,
	.delay = gtmr_delay,
	.uart_freq = sunxi_platform_uart_freq,
};

ARM_PLATFORM(sun8i_h3, "allwinner,sun8i-h3", &sun8i_h3_platform);
