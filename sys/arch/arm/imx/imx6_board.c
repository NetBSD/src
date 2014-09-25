/*	$NetBSD: imx6_board.c,v 1.1 2014/09/25 05:05:28 ryo Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: imx6_board.c,v 1.1 2014/09/25 05:05:28 ryo Exp $");

#include "opt_imx.h"
#include "arml2cc.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/locore.h>
#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/pl310_var.h>
#include <arm/mainbus/mainbus.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_mmdcreg.h>
#include <arm/imx/imxwdogreg.h>

bus_space_tag_t imx6_ioreg_bst = &imx_bs_tag;
bus_space_handle_t imx6_ioreg_bsh;
bus_space_tag_t imx6_armcore_bst = &imx_bs_tag;
bus_space_handle_t imx6_armcore_bsh;

void
imx6_bootstrap(vaddr_t iobase)
{
	int error;

	/* imx6_ioreg is mapped AIPS1+AIPS2 region */
	imx6_ioreg_bsh = (bus_space_handle_t)iobase;
	error = bus_space_map(imx6_ioreg_bst, IMX6_IOREG_PBASE,
	    IMX6_IOREG_SIZE, 0, &imx6_ioreg_bsh);
	if (error)
		panic("%s: failed to map Imx %s registers: %d",
		    __func__, "io", error);

	imx6_armcore_bsh = (bus_space_handle_t) iobase + IMX6_IOREG_SIZE;
	error = bus_space_map(imx6_armcore_bst, IMX6_ARMCORE_PBASE,
	    IMX6_ARMCORE_SIZE, 0, &imx6_armcore_bsh);
	if (error)
		panic("%s: failed to map Imx %s registers: %d",
		    __func__, "armcore", error);

#if NARML2CC > 0
	arml2cc_init(imx6_armcore_bst, imx6_armcore_bsh, ARMCORE_L2C_BASE);
#endif
}

/*
 * probe DDR size from DDR Controller register
 */
psize_t
imx6_memprobe(void)
{
	uint32_t ctrl, misc;
	int bitwidth;

	ctrl = bus_space_read_4(imx6_ioreg_bst, imx6_ioreg_bsh,
	    IMX6_AIPS1_SIZE + AIPS2_MMDC1_BASE + MMDC1_MDCTL);
	misc = bus_space_read_4(imx6_ioreg_bst, imx6_ioreg_bsh,
	    IMX6_AIPS1_SIZE + AIPS2_MMDC1_BASE + MMDC1_MDMISC);

	/* row */
	bitwidth = __SHIFTOUT(ctrl, MMDC1_MDCTL_ROW) + 11;

	/* column */
	switch (__SHIFTOUT(ctrl, MMDC1_MDCTL_COL)) {
	case 0:
		bitwidth += 9;
		break;
	case 1:
		bitwidth += 10;
		break;
	case 2:
		bitwidth += 11;
		break;
	case 3:
		bitwidth += 8;
		break;
	case 4:
		bitwidth += 12;
		break;
	default:
		printf("AIPS2_MMDC1:MDCTL[COL]: "
		    "unknown column address width: %llu\n",
		    __SHIFTOUT(ctrl, MMDC1_MDCTL_COL));
		return 0;
	}

	/* width */
	bitwidth += __SHIFTOUT(ctrl, MMDC1_MDCTL_DSIZ) + 1;

	/* bank */
	bitwidth += __SHIFTOUT(ctrl, MMDC1_MDCTL_SDE_1);
	bitwidth += (misc & MMDC1_MDMISC_DDR_4_BANK) ? 2 : 3;

	return (psize_t)1 << bitwidth;
}

void
imx6_reset(void)
{
	delay(1000);	/* wait for flushing FIFO of serial console */

	cpsid(I32_bit|F32_bit);

	/* software reset signal on wdog */
	bus_space_write_2(imx6_ioreg_bst, imx6_ioreg_bsh,
	    AIPS1_WDOG1_BASE + IMX_WDOG_WCR, WCR_WDE);

	/*
	 * write twice due to errata.
	 * Reference: ERR004346: IMX6DQCE Chip Errata for the i.MX 6Dual/6Quad
	 */
	bus_space_write_2(imx6_ioreg_bst, imx6_ioreg_bsh,
	    AIPS1_WDOG1_BASE + IMX_WDOG_WCR, WCR_WDE);

	for (;;)
		__asm("wfi");
}

void
imx6_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph") &&
	    device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore regisers (which armperiph uses).
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = imx6_armcore_bst;
		return;
	}

	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "a9tmr") || device_is_a(self, "a9wdt")) {
		prop_dictionary_set_uint32(dict, "frequency",
		   792000000 / 2);	/* XXX? */
		return;
	}
}

#ifdef MULTIPROCESSOR
void
imx6_cpu_hatch(struct cpu_info *ci)
{
	a9tmr_init_cpu_clock(ci);
}
#endif
