/* $NetBSD: ns8250_uart.c,v 1.7 2021/01/27 03:10:21 thorpej Exp $ */

/*-
 * Copyright (c) 2017-2020 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(1, "$NetBSD: ns8250_uart.c,v 1.7 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <dev/ic/comvar.h>

#include <dev/fdt/fdtvar.h>

static int ns8250_uart_match(device_t, cfdata_t, void *);
static void ns8250_uart_attach(device_t, device_t, void *);

struct ns8250_config {
	int			type;
	int			(*enable)(struct com_softc *);
	void			(*disable)(struct com_softc *);
};

static const struct ns8250_config ns8250_config = {
	.type = COM_TYPE_NORMAL,
};

static const struct ns8250_config ns16750_config = {
	.type = COM_TYPE_16750,
};

#define	NS8250_OCTEON_USR_REG		0x0138

static int
ns8250_octeon_enable(struct com_softc *sc)
{
	struct com_regs *regsp = &sc->sc_regs;

	/* XXX Clear old busy detect interrupts */
	bus_space_read_1(regsp->cr_iot, regsp->cr_ioh, NS8250_OCTEON_USR_REG);

	return 0;
}

static const struct ns8250_config octeon_config = {
	.type = COM_TYPE_16550_NOERS,
	.enable = ns8250_octeon_enable,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cavium,octeon-3860-uart",	.data = &octeon_config },
	{ .compat = "ns8250",			.data = &ns8250_config },
	{ .compat = "ns16450",			.data = &ns8250_config },
	{ .compat = "ns16550a",			.data = &ns8250_config },
	{ .compat = "ns16550",			.data = &ns8250_config },
	{ .compat = "ns16750",			.data = &ns16750_config },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(ns8250_uart, sizeof(struct com_softc),
	ns8250_uart_match, ns8250_uart_attach, NULL, NULL);

static int
ns8250_uart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ns8250_uart_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	char intrstr[128];
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int reg_shift;
	int error;
	void *ih;

	const struct ns8250_config *config =
	    of_compatible_lookup(phandle, compat_data)->data;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "reg-shift", &reg_shift)) {
		/* missing or bad reg-shift property, assume 0 */
		reg_shift = 0;
	}

	sc->sc_dev = self;
	if (of_getprop_uint32(phandle, "clock-frequency", &sc->sc_frequency)) {
		clk = fdtbus_clock_get_index(phandle, 0);
		if (clk != NULL)
			sc->sc_frequency = clk_get_rate(clk);
	}
	if (sc->sc_frequency == 0) {
		aprint_error(": couldn't get frequency\n");
		return;
	}

	sc->sc_type = config->type;
	sc->enable = config->enable;
	sc->disable = config->disable;

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIx64 ": %d",
		    (uint64_t)addr, error);
		return;
	}

	com_init_regs_stride(&sc->sc_regs, bst, bsh, addr, reg_shift);

	if (config->enable != NULL) {
		sc->enable(sc);
		sc->enabled = 1;
	}

	com_attach_subr(sc);

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish_xname(faa->faa_phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, comintr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

/*
 * Console support
 */

static int
ns8250_uart_console_match(int phandle)
{
	return of_compatible_match(phandle, compat_data);
}

static void
ns8250_uart_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t dummy_bsh;
	struct com_regs regs;
	bus_addr_t addr;
	tcflag_t flags;
	u_int reg_shift;
	int speed;

	const struct ns8250_config *config =
	    of_compatible_lookup(phandle, compat_data)->data;

	fdtbus_get_reg(phandle, 0, &addr, NULL);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	if (of_getprop_uint32(phandle, "reg-shift", &reg_shift)) {
		/* missing or bad reg-shift property, assume 0 */
		reg_shift = 0;
	}

	memset(&dummy_bsh, 0, sizeof(dummy_bsh));
	com_init_regs_stride(&regs, bst, dummy_bsh, addr, reg_shift);

	if (comcnattach1(&regs, speed, uart_freq, config->type, flags))
		panic("Cannot initialize ns8250 console");
}

static const struct fdt_console ns8250_uart_console = {
	.match = ns8250_uart_console_match,
	.consinit = ns8250_uart_console_consinit,
};

FDT_CONSOLE(ns8250_uart, &ns8250_uart_console);
