/*	$NetBSD: zynq7000_uart.c,v 1.3 2022/10/25 22:49:39 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: zynq7000_uart.c,v 1.3 2022/10/25 22:49:39 jmcneill Exp $");

#include "opt_soc.h"
#include "opt_console.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/xilinx/zynq_uartreg.h>
#include <arm/xilinx/zynq_uartvar.h>

#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "xlnx,xuartps" },
	{ .compat = "cdns,uart-r1p8" },
	DEVICE_COMPAT_EOL
};

int
zynquart_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
zynquart_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if (fdtbus_intr_establish(phandle, 0, IPL_SERIAL, IST_LEVEL,
				  zynquartintr, device_private(self)) == NULL) {
		aprint_error(": failed to establish interrupt on %s\n", intrstr);
		return;
	}

	zynquart_attach_common(parent, self, faa->faa_bst, addr, size, 0);
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

/*
 * Console support
 */

static int
zynq_uart_console_match(int phandle)
{
	return of_compatible_match(phandle, compat_data);
}

static void
zynq_uart_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	tcflag_t flags;
	int speed;

	fdtbus_get_reg(phandle, 0, &addr, NULL);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	if (zynquart_cons_attach(bst, addr, speed, flags))
		panic("Cannot initialize zynq uart console");
}

static const struct fdt_console zynq_uart_console = {
	.match = zynq_uart_console_match,
	.consinit = zynq_uart_console_consinit,
};

FDT_CONSOLE(zynq_uart, &zynq_uart_console);
