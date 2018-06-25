/*	$NetBSD: zynq7000_board.c,v 1.2.16.1 2018/06/25 07:25:40 pgoyette Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: zynq7000_board.c,v 1.2.16.1 2018/06/25 07:25:40 pgoyette Exp $");

#include "opt_zynq.h"
#include "arml2cc.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/locore.h>
#include <arm/cortex/a9tmr_var.h>
#include <arm/cortex/pl310_var.h>
#include <arm/mainbus/mainbus.h>

#include <arm/zynq/zynq7000_var.h>
#include <arm/zynq/zynq7000_reg.h>

/*
 * PERIPHCLK_N is an arm root clock divider for MPcore interupt controller.
 * PERIPHCLK_N is equal to, or greater than two.
 * see "Cortex-A9 MPCore Technical Reference Manual" -
 *     Chapter 5: Clocks, Resets, and Power Management, 5.1: Clocks.
 */
#ifndef PERIPHCLK_N
#define PERIPHCLK_N	2
#endif

bus_space_tag_t zynq7000_ioreg_bst = &zynq_bs_tag;
bus_space_handle_t zynq7000_ioreg_bsh;
bus_space_tag_t zynq7000_armcore_bst = &zynq_bs_tag;
bus_space_handle_t zynq7000_armcore_bsh;

struct zynq7000_clock_info clk_info = { 0 };

static void zynq7000_clock_init(struct zynq7000_clock_info *);

psize_t
zynq7000_memprobe(void)
{
	return MEMSIZE * 1024 * 1024;
}

void
zynq7000_bootstrap(vaddr_t iobase)
{
	int error;

	zynq7000_ioreg_bsh = (bus_space_handle_t) iobase;
	error = bus_space_map(zynq7000_ioreg_bst, ZYNQ7000_IOREG_PBASE,
	    ZYNQ7000_IOREG_SIZE, 0, &zynq7000_ioreg_bsh);
	if (error)
		panic("%s: failed to map Zynq %s registers: %d",
		    __func__, "io", error);

	zynq7000_armcore_bsh = (bus_space_handle_t) iobase + ZYNQ7000_IOREG_SIZE;
	error = bus_space_map(zynq7000_armcore_bst, ZYNQ7000_ARMCORE_PBASE,
	    ZYNQ7000_ARMCORE_SIZE, 0, &zynq7000_armcore_bsh);
	if (error)
		panic("%s: failed to map Zynq %s registers: %d",
		    __func__, "armcore", error);

	struct zynq7000_clock_info * const clk = &clk_info;
	zynq7000_clock_init(clk);

#if NARML2CC > 0
	arml2cc_init(zynq7000_armcore_bst, zynq7000_armcore_bsh, ARMCORE_L2C_BASE);
#endif
}

static void
zynq7000_clock_init(struct zynq7000_clock_info *clk)
{
	clk->clk_ps = ZYNQ7000_PS_CLK;
}

void
zynq7000_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph")
	    && device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore registers (which armperiph uses).
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = zynq7000_armcore_bst;
		return;
	}

	/*
	 * We need to tell the A9 Global/Watchdog Timer
	 * what frequency it runs at.
	 */
	if (device_is_a(self, "arma9tmr") || device_is_a(self, "a9wdt")) {
		prop_dictionary_set_uint32(dict, "frequency",
		    666666666 / PERIPHCLK_N);
		return;
	}
}

#ifdef MULTIPROCESSOR
void
zynq7000_cpu_hatch(struct cpu_info *ci)
{
	a9tmr_init_cpu_clock(ci);
}
#endif
