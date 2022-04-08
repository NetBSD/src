/* $NetBSD: dw_apb_uart.c,v 1.12 2022/04/08 10:17:54 andvar Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: dw_apb_uart.c,v 1.12 2022/04/08 10:17:54 andvar Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <dev/ic/comvar.h>

#include <dev/fdt/fdtvar.h>

static int dw_apb_uart_match(device_t, cfdata_t, void *);
static void dw_apb_uart_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "snps,dw-apb-uart" },
	DEVICE_COMPAT_EOL
};

struct dw_apb_uart_softc {
	struct com_softc ssc_sc;
	void *ssc_ih;

	struct clk *ssc_clk;
	struct clk *ssc_pclk;
	struct fdtbus_reset *ssc_rst;
};

CFATTACH_DECL_NEW(dw_apb_uart, sizeof(struct dw_apb_uart_softc),
	dw_apb_uart_match, dw_apb_uart_attach, NULL, NULL);

static int
dw_apb_uart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
dw_apb_uart_attach(device_t parent, device_t self, void *aux)
{
	struct dw_apb_uart_softc * const ssc = device_private(self);
	struct com_softc * const sc = &ssc->ssc_sc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int reg_shift, reg_iowidth;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "reg-shift", &reg_shift)) {
		/* missing or bad reg-shift property, assume 2 */
		reg_shift = 2;
	}
	if (of_getprop_uint32(phandle, "reg-io-width", &reg_iowidth)) {
		/* missing or bad reg-io-width property, assume 1 */
		reg_iowidth = 1;
	}

	sc->sc_dev = self;

	ssc->ssc_clk = fdtbus_clock_get_index(phandle, 0);
	if (ssc->ssc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
	if (clk_enable(ssc->ssc_clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	ssc->ssc_pclk = fdtbus_clock_get(phandle, "apb_pclk");
	if (ssc->ssc_pclk != NULL && clk_enable(ssc->ssc_pclk) != 0) {
		aprint_error(": couldn't enable peripheral clock\n");
		return;
	}

	ssc->ssc_rst = fdtbus_reset_get_index(phandle, 0);
	if (ssc->ssc_rst && fdtbus_reset_deassert(ssc->ssc_rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	sc->sc_frequency = clk_get_rate(ssc->ssc_clk);
	sc->sc_type = COM_TYPE_DW_APB;

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIx64 ": %d", (uint64_t)addr, error);
		return;
	}

	com_init_regs_stride_width(&sc->sc_regs, bst, bsh, addr, reg_shift, reg_iowidth);

	com_attach_subr(sc);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ssc->ssc_ih = fdtbus_intr_establish_xname(phandle, 0,
	    IPL_SERIAL, FDT_INTR_MPSAFE, comintr, sc, device_xname(self));
	if (ssc->ssc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

/*
 * Console support
 */

static int
dw_apb_uart_console_match(int phandle)
{
	return of_compatible_match(phandle, compat_data);
}

static void
dw_apb_uart_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t dummy_bsh;
	struct com_regs regs;
	bus_addr_t addr;
	tcflag_t flags;
	u_int reg_shift, reg_iowidth;
	int speed;

	fdtbus_get_reg(phandle, 0, &addr, NULL);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	if (of_getprop_uint32(phandle, "reg-shift", &reg_shift)) {
		/* missing or bad reg-shift property, assume 2 */
		reg_shift = 2;
	}
	if (of_getprop_uint32(phandle, "reg-io-width", &reg_iowidth)) {
		/* missing or bad reg-io-width property, assume 1 */
		reg_iowidth = 1;
	}

	memset(&dummy_bsh, 0, sizeof(dummy_bsh));
	com_init_regs_stride_width(&regs, bst, dummy_bsh, addr, reg_shift, reg_iowidth);

	if (comcnattach1(&regs, speed, uart_freq, COM_TYPE_DW_APB, flags))
		panic("Cannot initialize dw-apb-uart console");
}

static const struct fdt_console dw_apb_uart_console = {
	.match = dw_apb_uart_console_match,
	.consinit = dw_apb_uart_console_consinit,
};

FDT_CONSOLE(dw_apb_uart, &dw_apb_uart_console);
