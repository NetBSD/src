/*	$NetBSD: imx6_iomux.c,v 1.3 2023/05/04 13:29:33 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx6_iomux.c,v 1.3 2023/05/04 13:29:33 bouyer Exp $");

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/nxp/imx6_iomuxreg.h>

#include <dev/fdt/fdtvar.h>

struct imxiomux_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	int sc_phandle;
};

#define CONFIG_NO_PAD_CTL	__BIT(31)
#define CONFIG_SION		__BIT(30)

static int
imx6_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct imxiomux_softc * const sc = device_private(dev);
	int pins_len;
	uint32_t reg;

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));
	const u_int *pins = fdtbus_get_prop(phandle, "fsl,pins", &pins_len);

	aprint_debug_dev(sc->sc_dev, "name %s\n", fdtbus_get_string(phandle, "name"));
	while (pins_len >= 24) {
		u_int mux_reg   = be32toh(pins[0]);
		u_int conf_reg  = be32toh(pins[1]);
		u_int input_reg = be32toh(pins[2]);
		u_int mux_mode  = be32toh(pins[3]);
		u_int input_val = be32toh(pins[4]);
		u_int config    = be32toh(pins[5]);

		if (config & CONFIG_SION)
			mux_mode |= IOMUX_CONFIG_SION;
		config &= ~CONFIG_SION;

		reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, mux_reg);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, mux_reg, mux_mode);
		aprint_debug_dev(sc->sc_dev,
		    "mux    offset 0x%08x, val 0x%08x -> 0x%08x\n",
		    mux_reg, reg, mux_mode);

		if (!(config & CONFIG_NO_PAD_CTL)) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, conf_reg);
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, conf_reg, config);
			aprint_debug_dev(sc->sc_dev,
			    "config offset 0x%08x, val 0x%08x -> 0x%08x\n",
			    conf_reg, reg, config);
		}

		if (__SHIFTOUT(input_val, __BITS(31, 24)) == 0xff) {
			uint8_t sel   = __SHIFTOUT(input_val, __BITS(7, 0));
			uint8_t width = __SHIFTOUT(input_val, __BITS(15, 8));
			uint8_t shift = __SHIFTOUT(input_val, __BITS(23, 16));
			uint32_t mask = __BITS(shift + (width - 1), shift);

			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, input_reg);
			reg &= ~mask;
			reg |= __SHIFTIN(sel, mask);
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, input_reg, reg);
			aprint_debug_dev(sc->sc_dev,
			    "+input offset 0x%08x, val 0x%08x\n",
			    input_reg, reg);
		} else if (input_reg != 0) {
			reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, input_reg);
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, input_reg, input_val);
			aprint_debug_dev(sc->sc_dev,
			    "input  offset 0x%08x, val 0x%08x -> 0x%08x\n",
			    input_reg, reg, input_val);
		}

		pins_len -= 24;
		pins += 6;
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func imx6_pinctrl_funcs = {
	.set_config = imx6_pinctrl_set_config,
};

static int imxiomux_match(device_t, struct cfdata *, void *);
static void imxiomux_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxiomux, sizeof(struct imxiomux_softc),
    imxiomux_match, imxiomux_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-iomuxc" },
	{ .compat = "fsl,imx6sx-iomuxc" },
	{ .compat = "fsl,imx7d-iomuxc" },
	{ .compat = "fsl,imx8mq-iomuxc" },
	DEVICE_COMPAT_EOL
};

static int
imxiomux_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imxiomux_attach(device_t parent, device_t self, void *aux)
{
	struct imxiomux_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get iomux registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	error = bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error(": couldn't map iomux registers: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": IOMUX Controller\n");

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		if (of_hasprop(child, "fsl,pins")) {
			fdtbus_register_pinctrl_config(self, child, &imx6_pinctrl_funcs);
		} else {
			for (int sub = OF_child(child); sub; sub = OF_peer(sub)) {
				if (!of_hasprop(sub, "fsl,pins"))
					continue;
				fdtbus_register_pinctrl_config(self, sub, &imx6_pinctrl_funcs);
			}
		}
	}
}

