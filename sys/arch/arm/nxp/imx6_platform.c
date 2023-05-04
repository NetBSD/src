/*	$NetBSD: imx6_platform.c,v 1.8 2023/05/04 13:28:04 bouyer Exp $	*/

/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_platform.c,v 1.8 2023/05/04 13:28:04 bouyer Exp $");

#include "arml2cc.h"
#include "opt_console.h"
#include "opt_fdt.h"
#include "opt_multiprocessor.h"
#include "opt_soc.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/scu_reg.h>
#include <arm/cortex/gic_reg.h>
#include <arm/cortex/pl310_var.h>

#include <arm/nxp/imx6_reg.h>
#include <arm/nxp/imx6_srcreg.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxwdogreg.h>

#include <arm/nxp/imx6_platform.h>

#include <libfdt.h>

#define	IMX_REF_FREQ	80000000
#define	IMX6SX_REF_FREQ	24000000

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

extern struct bus_space armv7_generic_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

static const struct pmap_devmap *
imx_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(KERNEL_IO_IOREG_VBASE, IMX6_IOREG_PBASE, IMX6_IOREG_SIZE),
		DEVMAP_ENTRY(KERNEL_IO_ARMCORE_VBASE, IMX6_ARMCORE_PBASE, IMX6_ARMCORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static const struct pmap_devmap *
imx6sx_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(KERNEL_IO_IOREG_VBASE, IMX6_IOREG_PBASE, IMX6SX_IOREG_SIZE),
		DEVMAP_ENTRY(KERNEL_IO_ARMCORE_VBASE, IMX6_ARMCORE_PBASE, IMX6_ARMCORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
imx_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void imx_platform_early_putchar(char);

void __noasan
imx_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - IMX6_IOREG_PBASE) + KERNEL_IO_IOREG_VBASE)

	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[(IMX_USR2/4)]) & IMX_USR2_TXDC) == 0)
		;

	uartaddr[(IMX_UTXD/4)] = htole32(c);
#endif
}

static void
imx_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t prop = device_properties(self);

	if (device_is_a(self, "atphy")) {
		static const struct device_compatible_entry compat_data[] = {
			{ .compat = "fsl,imx6dl-sabresd" },
			{ .compat = "fsl,imx6q-sabresd" },
			{ .compat = "fsl,imx6qp-sabresd" },
			{ .compat = "solidrun,hummingboard2/q" },
			{ .compat = "solidrun,hummingboard2/dl" },
			DEVICE_COMPAT_EOL
		};
		if (of_compatible_match(OF_finddevice("/"), compat_data))
			prop_dictionary_set_uint32(prop, "clk_25m", 125000000);
	}
}

static u_int
imx_platform_uart_freq(void)
{
	return IMX_REF_FREQ;
}

static u_int
imx6sx_platform_uart_freq(void)
{
	return IMX6SX_REF_FREQ;
}


static void
imx_platform_bootstrap(void)
{
#if NARML2CC > 0
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	if (bus_space_map(bst, IMX6_ARMCORE_PBASE, IMX6_ARMCORE_SIZE, 0, &bsh))
		panic("couldn't map armcore registers");
	arml2cc_init(bst, bsh, ARMCORE_L2C_BASE);
	bus_space_unmap(bst, bsh, IMX6_ARMCORE_SIZE);
#endif

	arm_fdt_cpu_bootstrap();
}

static int
imx_platform_mpstart(void)
{
#if defined(MULTIPROCESSOR)
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	if (bus_space_map(bst, IMX6_ARMCORE_PBASE, IMX6_ARMCORE_SIZE, 0, &bsh) != 0)
		panic("couldn't map armcore registers");

	/* Enable Snoop Control Unit */
	bus_space_write_4(bst, bsh, SCU_INV_ALL_REG, 0xff);
	bus_space_write_4(bst, bsh, SCU_CTL,
	    bus_space_read_4(bst, bsh, SCU_CTL) | SCU_CTL_SCU_ENA);

	bus_space_unmap(bst, bsh, AIPS1_SRC_SIZE);

	if (bus_space_map(bst, IMX6_AIPS1_BASE + AIPS1_SRC_BASE, AIPS1_SRC_SIZE, 0, &bsh) != 0)
		panic("couldn't map SRC");

	uint32_t srcctl = bus_space_read_4(bst, bsh, SRC_SCR);
	const paddr_t mpstart = KERN_VTOPHYS((vaddr_t)cpu_mpstart);

	srcctl &= ~(SRC_SCR_CORE1_ENABLE | SRC_SCR_CORE2_ENABLE	 |
	    SRC_SCR_CORE3_ENABLE);
	bus_space_write_4(bst, bsh, SRC_SCR, srcctl);

	for (int i = 1; i < arm_cpu_max; i++) {
		bus_space_write_4(bst, bsh, SRC_GPRN_ENTRY(i), mpstart);
		srcctl |= SRC_SCR_COREN_RST(i);
		srcctl |= SRC_SCR_COREN_ENABLE(i);
	}
	bus_space_write_4(bst, bsh, SRC_SCR, srcctl);

	bus_space_unmap(bst, bsh, AIPS1_SRC_SIZE);

	return arm_fdt_cpu_mpstart();
#else
	return 0;
#endif
}

static void
imx6_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	if (bus_space_map(bst, IMX6_AIPS1_BASE + AIPS1_WDOG1_BASE, AIPS1_WDOG_SIZE, 0, &bsh))
		panic("couldn't map wdog1 registers");

	delay(1000);	/* wait for flushing FIFO of serial console */

	cpsid(I32_bit|F32_bit);

	/* software reset signal on wdog */
	bus_space_write_2(bst, bsh, IMX_WDOG_WCR, WCR_WDE);

	/*
	 * write twice due to errata.
	 * Reference: ERR004346: IMX6DQCE Chip Errata for the i.MX 6Dual/6Quad
	 */
	bus_space_write_2(bst, bsh, IMX_WDOG_WCR, WCR_WDE);

	for (;;)
		__asm("wfi");
}

static const struct fdt_platform imx6_platform = {
	.fp_devmap = imx_platform_devmap,
	.fp_bootstrap = imx_platform_bootstrap,
	.fp_init_attach_args = imx_platform_init_attach_args,
	.fp_device_register = imx_platform_device_register,
	.fp_reset = imx6_platform_reset,
	.fp_delay = a9ptmr_delay,
	.fp_uart_freq = imx_platform_uart_freq,
	.fp_mpstart = imx_platform_mpstart,
};

static const struct fdt_platform imx6sx_platform = {
	.fp_devmap = imx6sx_platform_devmap,
	.fp_bootstrap = imx_platform_bootstrap,
	.fp_init_attach_args = imx_platform_init_attach_args,
	.fp_device_register = imx_platform_device_register,
	.fp_reset = imx6_platform_reset,
	.fp_delay = a9ptmr_delay,
	.fp_uart_freq = imx6sx_platform_uart_freq,
	.fp_mpstart = imx_platform_mpstart,
};

FDT_PLATFORM(imx6dl, "fsl,imx6dl", &imx6_platform);
FDT_PLATFORM(imx6sx, "fsl,imx6sx", &imx6sx_platform);
FDT_PLATFORM(imx6q, "fsl,imx6q", &imx6_platform);
FDT_PLATFORM(imx6qp, "fsl,imx6qp", &imx6_platform);
