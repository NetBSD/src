/* $Id: imx23_clkctrl.c,v 1.5 2026/02/01 11:31:28 yurix Exp $ */

/*
* Copyright (c) 2013 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <dev/fdt/fdtvar.h>

#include <arm/imx/imx23_clkctrlreg.h>
#include <arm/imx/imx23var.h>

struct imx23_clkctrl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
};

static int	imx23_clkctrl_match(device_t, cfdata_t, void *);
static void	imx23_clkctrl_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imx23clkctrl, sizeof(struct imx23_clkctrl_softc),
		  imx23_clkctrl_match, imx23_clkctrl_attach, NULL, NULL);

#define CLKCTRL_RD(sc, reg)                                                 \
        bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define CLKCTRL_WR(sc, reg, val)                                            \
        bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define CLKCTRL_SOFT_RST_LOOP 455 /* At least 1 us ... */

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-clkctrl" },
	{ .compat = "fsl,clkctrl" },
	DEVICE_COMPAT_EOL
};

static int
imx23_clkctrl_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_clkctrl_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_clkctrl_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Power up 8-phase PLL outputs for USB PHY */
	CLKCTRL_WR(sc, HW_CLKCTRL_PLLCTRL0_SET,
		   HW_CLKCTRL_PLLCTRL0_EN_USB_CLKS);
	/* Enable 24MHz clock for the audio output. */
	CLKCTRL_WR(sc, HW_CLKCTRL_XTAL_CLR, HW_CLKCTRL_XTAL_FILT_CLK24M_GATE);

	aprint_normal("\n");

	return;
}
