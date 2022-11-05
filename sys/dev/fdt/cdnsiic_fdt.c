/* $NetBSD: cdnsiic_fdt.c,v 1.1 2022/11/05 17:31:37 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: cdnsiic_fdt.c,v 1.1 2022/11/05 17:31:37 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/ic/cdnsiicvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cdns,i2c-r1p10" },
	{ .compat = "cdns,i2c-r1p14" },
	DEVICE_COMPAT_EOL
};

static int
cdnsiic_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
cdnsiic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct cdnsiic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_pclk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_pclk == NULL || clk_enable(sc->sc_pclk) != 0) {
		aprint_error(": couldn't enable pclk\n");
		return;
	}

	if (of_getprop_uint32(phandle, "clock-frequency", &sc->sc_bus_freq))
		sc->sc_bus_freq = 100000;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	error = cdnsiic_attach(sc);
	if (error != 0) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	fdtbus_register_i2c_controller(&sc->sc_ic, phandle);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);
}

CFATTACH_DECL_NEW(cdnsiic_fdt, sizeof(struct cdnsiic_softc),
    cdnsiic_fdt_match, cdnsiic_fdt_attach, NULL, NULL);
