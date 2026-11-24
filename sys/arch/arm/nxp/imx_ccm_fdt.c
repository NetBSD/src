/* $NetBSD: imx_ccm_fdt.c,v 1.1 2026/11/24 11:05:52 hkenken Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_fdt.c,v 1.1 2026/11/24 11:05:52 hkenken Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

static struct clk *
imx_ccm_clock_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct imx_ccm_softc * const sc = device_private(dev);
	struct imx_ccm_clk *clk;

	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);

	for (int i = 0; i < sc->sc_nclks; i++) {
		clk = &sc->sc_clks[i];
		if (clk->id == clock_id)
			return &clk->base;
	}

	return NULL;
}

static const struct fdtbus_clock_controller_func imx_ccm_fdtclock_funcs = {
	.decode = imx_ccm_clock_decode,
};

static struct clk *
imx_ccm_clock_byname(struct imx_ccm_softc *sc, const char *parent)
{
	return fdtbus_clock_byname(parent);
}

int
imx_ccm_attach(struct imx_ccm_softc *sc)
{
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return ENXIO;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh[0]) != 0) {
		aprint_error(": couldn't map registers\n");
		return ENXIO;
	}

	sc->sc_clock_find_parent = imx_ccm_clock_byname;

	if (imx_ccm_clock_attach(sc) != 0) {
		aprint_error(": couldn't attach clocks\n");
		return ENXIO;
	}

	fdtbus_register_clock_controller(sc->sc_dev, sc->sc_phandle,
	    &imx_ccm_fdtclock_funcs);

	return 0;
}
