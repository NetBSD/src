/*	$NetBSD: imx6_com.c,v 1.3.6.2 2020/04/13 08:03:35 martin Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: imx6_com.c,v 1.3.6.2 2020/04/13 08:03:35 martin Exp $");

#include "opt_fdt.h"
#include "opt_imxuart.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>

static int imx6_com_match(device_t, struct cfdata *, void *);
static void imx6_com_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imx6_com, sizeof(struct imxuart_softc),
    imx6_com_match, imx6_com_attach, NULL, NULL);

static const char * const compatible[] = {
	"fsl,imx6q-uart",
	NULL
};

static int
imx6_com_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
imx6_com_attach(device_t parent, device_t self, void *aux)
{
	struct imxuart_softc *sc = device_private(self);
	struct imxuart_regs *regsp = &sc->sc_regs;
	struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	char intrstr[128];
	struct clk *per;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (fdtbus_clock_enable(phandle, "ipg", false) != 0) {
		aprint_error(": couldn't enable ipg clock\n");
		return;
	}

	per = fdtbus_clock_get(phandle, "per");
	if (per != NULL && clk_enable(per) != 0) {
		aprint_error(": couldn't enable per clock\n");
		return;
	}

	sc->sc_dev = self;
	regsp->ur_iot = bst;
	regsp->ur_iobase = addr;
	regsp->ur_ioh = bsh;

	if (per != NULL) {
		aprint_normal(", %u Hz", clk_get_rate(per));
		/* XXX */
		imxuart_set_frequency(clk_get_rate(per), 2);
	}

	if (imxuart_is_console(regsp->ur_iot, regsp->ur_iobase, &regsp->ur_ioh))
		aprint_normal(" (console)");

	aprint_normal("\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL,
	    0, imxuintr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
		return;
	}

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	imxuart_attach_subr(sc);
}

/*
 * Console support
 */

static int
imx6_com_console_match(int phandle)
{
	return of_match_compatible(phandle, compatible);
}

static void
imx6_com_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	bus_size_t size;
	tcflag_t flags;
	int speed;

	fdtbus_get_reg(phandle, 0, &addr, &size);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	imxuart_set_frequency(uart_freq, 2);
	if (imxuart_cnattach(bst, addr, speed, flags) != 0)
		panic("cannot attach console UART");
}

static const struct fdt_console imx6_com_console = {
	.match = imx6_com_console_match,
	.consinit = imx6_com_console_consinit,
};

FDT_CONSOLE(imx6_com, &imx6_com_console);
