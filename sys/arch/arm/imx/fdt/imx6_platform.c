/*	$NetBSD: imx6_platform.c,v 1.11 2020/09/28 12:15:23 jmcneill Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: imx6_platform.c,v 1.11 2020/09/28 12:15:23 jmcneill Exp $");

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

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_srcreg.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxwdogreg.h>

#include <arm/imx/fdt/imx6_platform.h>

#include <libfdt.h>

#define	IMX_REF_FREQ	80000000

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
		const char * compat[] = {
			"fsl,imx6dl-sabresd",
			"fsl,imx6q-sabresd",
			"fsl,imx6qp-sabresd",
			"solidrun,hummingboard2/q",
			"solidrun,hummingboard2/dl",
			NULL
		};
		if (of_match_compatible(OF_finddevice("/"), compat))
			prop_dictionary_set_uint32(prop, "clk_25m", 125000000);
	}
}

static u_int
imx_platform_uart_freq(void)
{
	return IMX_REF_FREQ;
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

const struct arm_platform imx6_platform = {
	.ap_devmap = imx_platform_devmap,
	.ap_bootstrap = imx_platform_bootstrap,
	.ap_init_attach_args = imx_platform_init_attach_args,
	.ap_device_register = imx_platform_device_register,
	.ap_reset = imx6_platform_reset,
	.ap_delay = a9ptmr_delay,
	.ap_uart_freq = imx_platform_uart_freq,
	.ap_mpstart = imx_platform_mpstart,
};

ARM_PLATFORM(imx6q, "fsl,imx6q", &imx6_platform);
ARM_PLATFORM(imx6qp, "fsl,imx6qp", &imx6_platform);
