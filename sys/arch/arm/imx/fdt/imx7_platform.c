/*	$NetBSD: imx7_platform.c,v 1.1 2020/06/10 17:57:50 jmcneill Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: imx7_platform.c,v 1.1 2020/06/10 17:57:50 jmcneill Exp $");

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

#include <arm/cortex/gtmr_var.h>
#include <arm/cortex/gic_reg.h>

#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7_srcreg.h>
#include <arm/imx/imx7_ccmreg.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxwdogreg.h>

#include <arm/imx/fdt/imx7_platform.h>

#include <libfdt.h>

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

static const struct pmap_devmap *
imx7_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(KERNEL_IO_IOREG_VBASE, IMX7_IOREG_PBASE, IMX7_IOREG_SIZE),
		DEVMAP_ENTRY(KERNEL_IO_ARMCORE_VBASE, IMX7_ARMCORE_PBASE, IMX7_ARMCORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
imx7_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_a4x_bst = &armv7_generic_a4x_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void imx7_platform_early_putchar(char);

void
imx7_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	((CONSADDR - IMX7_IOREG_PBASE) + KERNEL_IO_IOREG_VBASE)

	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	while ((le32toh(uartaddr[(IMX_USR2/4)]) & IMX_USR2_TXDC) == 0)
		;

	uartaddr[(IMX_UTXD/4)] = htole32(c);
#endif
}

static void
imx7_platform_device_register(device_t self, void *aux)
{
	prop_dictionary_t prop = device_properties(self);

	if (device_is_a(self, "atphy")) {
		const char * compat[] = {
			"fsl,imx7dl-sabresd",
			"fsl,imx7q-sabresd",
			"fsl,imx7qp-sabresd",
			NULL
		};
		if (of_match_compatible(OF_finddevice("/"), compat))
			prop_dictionary_set_uint32(prop, "clk_25m", 125000000);
	}
}

static u_int
imx7_platform_uart_freq(void)
{
	return IMX7_OSC_FREQ;
}

static int
imx7_platform_mpstart(void)
{
#if defined(MULTIPROCESSOR)
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	if (bus_space_map(bst, IMX7_IOREG_PBASE + AIPS1_SRC_BASE, AIPS1_SRC_SIZE, 0, &bsh) != 0)
		panic("couldn't map aips1 src registers");
	const paddr_t mpstart = KERN_VTOPHYS((vaddr_t)cpu_mpstart);

	bus_space_write_4(bst, bsh, SRC_GPR3, mpstart);

	uint32_t rcr1 = bus_space_read_4(bst, bsh, SRC_A7RCR1);
	rcr1 |= SRC_A7RCR1_A7_CORE1_ENABLE;
	bus_space_write_4(bst, bsh, SRC_A7RCR1, rcr1);

	arm_dsb();
	__asm __volatile("sev" ::: "memory");

	/* Wait for AP to start */
	for (u_int i = 0x1000000; i > 0; i--) {
		if (cpu_hatched_p(1))
			break;
	}
	if (!cpu_hatched_p(1))
		printf("cpu1: WARNING: failed to start\n");

	bus_space_unmap(bst, bsh, AIPS1_SRC_SIZE);
#endif

	return 0;
}

static void
imx7_platform_reset(void)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	if (bus_space_map(bst, IMX7_IOREG_PBASE + AIPS1_WDOG1_BASE, AIPS1_WDOG_SIZE, 0, &bsh))
		panic("couldn't map wdog1 registers");

	delay(1000);	/* wait for flushing FIFO of serial console */

	cpsid(I32_bit|F32_bit);

	/* software reset signal on wdog */
	bus_space_write_2(bst, bsh, IMX_WDOG_WCR, WCR_WDE);

	for (;;)
		__asm("wfi");
}

const struct arm_platform imx7_platform = {
	.ap_devmap = imx7_platform_devmap,
	.ap_bootstrap = arm_fdt_cpu_bootstrap,
	.ap_init_attach_args = imx7_platform_init_attach_args,
	.ap_device_register = imx7_platform_device_register,
	.ap_reset = imx7_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = imx7_platform_uart_freq,
	.ap_mpstart = imx7_platform_mpstart,
};

ARM_PLATFORM(imx7d, "fsl,imx7d", &imx7_platform);
