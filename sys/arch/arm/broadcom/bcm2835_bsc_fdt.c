/*	$NetBSD: bcm2835_bsc_fdt.c,v 1.6 2021/01/29 14:11:14 skrll Exp $	*/

/*
 * Copyright (c) 2019 Jason R. Thorpe
 * Copyright (c) 2012 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_bsc_fdt.c,v 1.6 2021/01/29 14:11:14 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernhist.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_bscreg.h>
#include <arm/broadcom/bcm2835_bscvar.h>

#include <dev/fdt/fdtvar.h>

static int bsciic_fdt_match(device_t, cfdata_t, void *);
static void bsciic_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bsciic_fdt, sizeof(struct bsciic_softc),
    bsciic_fdt_match, bsciic_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2835-i2c" },
	DEVICE_COMPAT_EOL
};

static int
bsciic_fdt_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
bsciic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct bsciic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	int error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": unable to get device registers\n");
		return;
	}

	/* Enable clock */
	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't acquire clock\n");
		return;
	}

	if (clk_enable(sc->sc_clk) != 0) {
		aprint_error(": failed to enable clock\n");
		return;
	}

	sc->sc_frequency = clk_get_rate(sc->sc_clk);

	if (of_getprop_uint32(phandle, "clock-frequency",
	    &sc->sc_clkrate) != 0) {
		sc->sc_clkrate = 100000;
	}

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error(": unable to map device\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Broadcom Serial Controller\n");

	bsciic_attach(sc);

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt\n");
		return;
	}
	sc->sc_inth = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, bsciic_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_inth == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "failed to establish interrupt %s\n", intrstr);
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = bsciic_acquire_bus;
	sc->sc_i2c.ic_release_bus = bsciic_release_bus;
	sc->sc_i2c.ic_exec = bsciic_exec;

	fdtbus_register_i2c_controller(&sc->sc_i2c, phandle);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_i2c, iicbus_print);
}
