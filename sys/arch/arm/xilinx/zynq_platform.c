/*	$NetBSD: zynq_platform.c,v 1.3 2020/09/28 11:54:24 jmcneill Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#include "arml2cc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq_platform.c,v 1.3 2020/09/28 11:54:24 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>

#include <arm/cortex/a9tmr_var.h>
#include <arm/xilinx/zynq_uartreg.h>

#include <evbarm/fdt/platform.h>

#include <libfdt.h>

#include <arm/cortex/pl310_var.h>

#define	ZYNQ_REF_FREQ	24000000

#define	ZYNQ7000_DDR_PBASE	0x00000000
#define	ZYNQ7000_DDR_SIZE	0x40000000

#define	ZYNQ_IOREG_VBASE	KERNEL_IO_VBASE
#define	ZYNQ_IOREG_PBASE	0xe0000000
#define ZYNQ_IOREG_SIZE		0x00200000

#define ZYNQ_GPV_VBASE		(ZYNQ_IOREG_VBASE + ZYNQ_IOREG_SIZE)
#define ZYNQ_GPV_PBASE		0xf8900000
#define ZYNQ_GPV_SIZE		0x00100000

#define ZYNQ_ARMCORE_VBASE	(ZYNQ_GPV_VBASE + ZYNQ_GPV_SIZE)
#define ZYNQ_ARMCORE_PBASE	0xf8f00000
#define ZYNQ_ARMCORE_SIZE	0x00003000

extern struct bus_space arm_generic_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;

void zynq_platform_early_putchar(char);

static const struct pmap_devmap *
zynq_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(ZYNQ_IOREG_VBASE,
			     ZYNQ_IOREG_PBASE,
			     ZYNQ_IOREG_SIZE),
		DEVMAP_ENTRY(ZYNQ_GPV_VBASE,
			     ZYNQ_GPV_PBASE,
			     ZYNQ_GPV_SIZE),
		DEVMAP_ENTRY(ZYNQ_ARMCORE_VBASE,
			     ZYNQ_ARMCORE_PBASE,
			     ZYNQ_ARMCORE_SIZE),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
zynq_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_dmat = &arm_generic_dma_tag;
}

void __noasan
zynq_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA     ((CONSADDR - ZYNQ_IOREG_PBASE) + ZYNQ_IOREG_VBASE)
	volatile uint32_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint32_t *)CONSADDR_VA :
	    (volatile uint32_t *)CONSADDR;

	/* QEMU needs CR_TXEN to be set and CR_TXDIS to be unset */
	uartaddr[UART_CONTROL / 4] = CR_TXEN;
	while ((le32toh(uartaddr[UART_CHNL_INT_STS / 4]) & STS_TEMPTY) == 0)
		;

	uartaddr[UART_TX_RX_FIFO / 4] = htole32(c);
#endif
}

static void
zynq_platform_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

	if (device_is_a(dev, "arma9tmr")) {
		prop_dictionary_set_uint32(dict, "frequency",
			ZYNQ_REF_FREQ / 4);
	}
}

static u_int
zynq_platform_uart_freq(void)
{
	return ZYNQ_REF_FREQ;
}

#define ZYNQ_ARMCORE_L2C_BASE	0x00002000
#define ZYNQ_ARM_PL310_BASE	ZYNQ_ARMCORE_VBASE + ZYNQ_ARMCORE_L2C_BASE

static void
zynq_platform_bootstrap(void)
{
#if NARML2CC > 0
	const bus_space_handle_t pl310_bh = ZYNQ_ARM_PL310_BASE;
	arml2cc_init(&arm_generic_bs_tag, pl310_bh, 0);
#endif

	arm_fdt_cpu_bootstrap();

	void *fdt_data = __UNCONST(fdtbus_get_data());
	const int chosen_off = fdt_path_offset(fdt_data, "/chosen");
	if (chosen_off < 0)
		return;

	if (match_bootconf_option(boot_args, "console", "fb")) {
		const int framebuffer_off =
		    fdt_path_offset(fdt_data, "/chosen/framebuffer");
		if (framebuffer_off >= 0) {
			const char *status = fdt_getprop(fdt_data,
			    framebuffer_off, "status", NULL);
			if (status == NULL || strncmp(status, "ok", 2) == 0) {
				fdt_setprop_string(fdt_data, chosen_off,
				    "stdout-path", "/chosen/framebuffer");
			}
		}
	} else if (match_bootconf_option(boot_args, "console", "serial")) {
		fdt_setprop_string(fdt_data, chosen_off,
		    "stdout-path", "serial0:115200n8");
	}
}

static void
zynq_platform_reset(void)
{

}

static const struct arm_platform zynq_platform = {
	.ap_devmap = zynq_platform_devmap,
	.ap_bootstrap = zynq_platform_bootstrap,
	.ap_init_attach_args = zynq_platform_init_attach_args,
	.ap_device_register = zynq_platform_device_register,
	.ap_reset = zynq_platform_reset,
	.ap_delay = a9tmr_delay,
	.ap_uart_freq = zynq_platform_uart_freq,
#if 0
	.ap_mpstart = arm_fdt_cpu_mpstart,
#endif
};


ARM_PLATFORM(zynq, "xlnx,zynq-7000", &zynq_platform);
