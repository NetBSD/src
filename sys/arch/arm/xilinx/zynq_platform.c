/*	$NetBSD: zynq_platform.c,v 1.11 2023/04/07 08:55:31 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: zynq_platform.c,v 1.11 2023/04/07 08:55:31 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>

#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/scu_reg.h>
#include <arm/xilinx/zynq_uartreg.h>

#include <evbarm/fdt/platform.h>

#include <libfdt.h>

#include <arm/cortex/pl310_var.h>

#include <arm/arm32/machdep.h>

#define	ZYNQ_REF_FREQ	24000000
#define	ZYNQ7000_DDR_PBASE	0x00000000
#define	ZYNQ7000_DDR_SIZE	0x40000000

#define	ZYNQ_IOREG_VBASE	KERNEL_IO_VBASE
#define	ZYNQ_IOREG_PBASE	0xe0000000
#define ZYNQ_IOREG_SIZE		0x00200000

#define	ZYNQ_SLCR_VBASE		(ZYNQ_IOREG_VBASE + ZYNQ_IOREG_SIZE)
#define	ZYNQ_SLCR_PBASE		0xf8000000
#define	ZYNQ_SLCR_SIZE		0x00100000

#define ZYNQ_GPV_VBASE		(ZYNQ_SLCR_VBASE + ZYNQ_SLCR_SIZE)
#define ZYNQ_GPV_PBASE		0xf8900000
#define ZYNQ_GPV_SIZE		0x00100000

#define ZYNQ_ARMCORE_VBASE	(ZYNQ_GPV_VBASE + ZYNQ_GPV_SIZE)
#define ZYNQ_ARMCORE_PBASE	0xf8f00000
#define ZYNQ_ARMCORE_SIZE	0x00100000

#define	ZYNQ_OCM_VBASE		(ZYNQ_ARMCORE_VBASE + ZYNQ_ARMCORE_SIZE)
#define	ZYNQ_OCM_PBASE		0xfff00000
#define	ZYNQ_OCM_SIZE		0x00100000

#define	ZYNQ_ARMCORE_SCU_BASE	0x00000000
#define	ZYNQ_ARMCORE_L2C_BASE	0x00002000

#define	ZYNQ7000_CPU1_ENTRY	0xfffffff0
#define	ZYNQ7000_CPU1_ENTRY_SZ	4

/* SLCR registers */
#define	SLCR_UNLOCK		0x008
#define	 UNLOCK_KEY		0xdf0d
#define	PSS_RST_CTRL		0x200
#define	 SOFT_RST		__BIT(0)

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
		DEVMAP_ENTRY(ZYNQ_SLCR_VBASE,
			     ZYNQ_SLCR_PBASE,
			     ZYNQ_SLCR_SIZE),
		DEVMAP_ENTRY(ZYNQ_GPV_VBASE,
			     ZYNQ_GPV_PBASE,
			     ZYNQ_GPV_SIZE),
		DEVMAP_ENTRY(ZYNQ_ARMCORE_VBASE,
			     ZYNQ_ARMCORE_PBASE,
			     ZYNQ_ARMCORE_SIZE),
		DEVMAP_ENTRY(ZYNQ_OCM_VBASE,
			     ZYNQ_OCM_PBASE,
			     ZYNQ_OCM_SIZE),
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
}

static u_int
zynq_platform_uart_freq(void)
{
	return ZYNQ_REF_FREQ;
}

#ifdef MULTIPROCESSOR
static int
zynq_platform_mpstart(void)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t val;
	int error;
	u_int i;

	/* Invalidate all SCU cache tags and enable SCU. */
	bsh = ZYNQ_ARMCORE_VBASE + ZYNQ_ARMCORE_SCU_BASE;
	bus_space_write_4(bst, bsh, SCU_INV_ALL_REG, 0xffff);
	val = bus_space_read_4(bst, bsh, SCU_CTL);
	bus_space_write_4(bst, bsh, SCU_CTL, val | SCU_CTL_SCU_ENA);
	armv7_dcache_wbinv_all();

	/* Write start address for CPU1. */
	error = bus_space_map(bst, ZYNQ7000_CPU1_ENTRY,
	    ZYNQ7000_CPU1_ENTRY_SZ, 0, &bsh);
	if (error) {
		panic("%s: Couldn't map OCM: %d", __func__, error);
	}
	bus_space_write_4(bst, bsh, 0, KERN_VTOPHYS((vaddr_t)cpu_mpstart));
	bus_space_unmap(bst, bsh, ZYNQ7000_CPU1_ENTRY_SZ);

	dsb(sy);
	sev();

	const u_int cpuindex = 1;
	for (i = 0x10000000; i > 0; i--) {
		if (cpu_hatched_p(cpuindex)) {
			break;
		}
	}
	if (i == 0) {
		aprint_error("cpu%d: WARNING: AP failed to start\n",
		    cpuindex);
		return EIO;
	}

	return 0;
}
#endif

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
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh = ZYNQ_SLCR_VBASE;

	bus_space_write_4(bst, bsh, SLCR_UNLOCK, UNLOCK_KEY);
	bus_space_write_4(bst, bsh, PSS_RST_CTRL, SOFT_RST);
}

static const struct fdt_platform zynq_platform = {
	.fp_devmap = zynq_platform_devmap,
	.fp_bootstrap = zynq_platform_bootstrap,
	.fp_init_attach_args = zynq_platform_init_attach_args,
	.fp_device_register = zynq_platform_device_register,
	.fp_reset = zynq_platform_reset,
	.fp_delay = a9tmr_delay,
	.fp_uart_freq = zynq_platform_uart_freq,
#ifdef MULTIPROCESSOR
	.fp_mpstart = zynq_platform_mpstart,
#endif
};


FDT_PLATFORM(zynq, "xlnx,zynq-7000", &zynq_platform);
