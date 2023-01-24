/* $NetBSD: plcom_fdt.c,v 1.6 2023/01/24 06:56:40 mlelstv Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: plcom_fdt.c,v 1.6 2023/01/24 06:56:40 mlelstv Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

static int	plcom_fdt_match(device_t, cfdata_t, void *);
static void	plcom_fdt_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arm,pl011", .value = PLCOM_TYPE_PL011 },
	{ .compat = "arm,sbsa-uart", .value = PLCOM_TYPE_GENERIC_UART },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(plcom_fdt, sizeof(struct plcom_softc),
	plcom_fdt_match, plcom_fdt_attach, NULL, NULL);

static int
plcom_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
plcom_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct plcom_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;
	const u_int *data;
	int len;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": missing 'reg' property\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_dev = self;

	/* Enable clocks */
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": failed to enable clock #%d\n", i);
			return;
		}
		/* First clock is UARTCLK */
		if (i == 0)
			sc->sc_frequency = clk_get_rate(clk);
	}

	sc->sc_hwflags = 0;
	sc->sc_swflags = 0;

	if ((data = fdtbus_get_prop(phandle, "arm,primecell-periphid", &len)) != NULL)
                sc->sc_pi.pi_periphid = be32toh(data[0]);

	sc->sc_pi.pi_type = of_compatible_lookup(faa->faa_phandle, compat_data)->value;
	sc->sc_pi.pi_flags = PLC_FLAG_32BIT_ACCESS;
	sc->sc_pi.pi_iot = faa->faa_bst;
	sc->sc_pi.pi_iobase = addr;
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_pi.pi_ioh)) {
		aprint_error(": couldn't map device\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": ARM PL011 UART\n");

	plcom_attach_subr(sc);

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SERIAL, FDT_INTR_MPSAFE,
	    plcomintr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		return;
	}
}

static int
plcom_fdt_console_match(int phandle)
{
	return of_compatible_match(phandle, compat_data);
}

static void
plcom_fdt_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	static struct plcom_instance pi;
	bus_addr_t addr;
	bus_size_t size;
	tcflag_t flags;
	int speed;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0)
		return;

	pi.pi_type = PLCOM_TYPE_PL011;
	pi.pi_flags = PLC_FLAG_32BIT_ACCESS;
	pi.pi_iot = faa->faa_bst;
	pi.pi_iobase = addr;
	pi.pi_size = size;

	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;
	flags = fdtbus_get_stdout_flags();

	plcomcnattach(&pi, speed, uart_freq, flags, -1);
}

static const struct fdt_console plcom_fdt_console = {
	.match = plcom_fdt_console_match,
	.consinit = plcom_fdt_console_consinit,
};

FDT_CONSOLE(plcom_fdt, &plcom_fdt_console);
