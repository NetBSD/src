/* $NetBSD: pinctrl_single.c,v 1.6 2021/11/07 17:12:25 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: pinctrl_single.c,v 1.6 2021/11/07 17:12:25 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>

#define	PINCTRL_FLAG_PINCONF	__BIT(0)	/* supports generic pinconf */

struct pinctrl_single_config {
	uint32_t	flags;
};

static const struct pinctrl_single_config pinctrl_config = {
};

static const struct pinctrl_single_config pinconf_config = {
	.flags = PINCTRL_FLAG_PINCONF
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "pinctrl-single",	.data = &pinctrl_config },
	{ .compat = "pinconf-single",	.data = &pinconf_config },
	DEVICE_COMPAT_EOL
};

struct pinctrl_single_softc {
	device_t			sc_dev;
	int				sc_phandle;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	uint32_t			sc_flags;
	u_int				sc_regwidth;
	u_int				sc_funcmask;
};

static void
pinctrl_single_pins_write(struct pinctrl_single_softc *sc, u_int off, u_int val)
{
	union {
		uint32_t reg32;
		uint16_t reg16;
		uint8_t reg8;
	} u;

	aprint_debug_dev(sc->sc_dev, "writing %#x with %#x\n", off, val);

	switch (sc->sc_regwidth) {
	case 8:
		u.reg8 = bus_space_read_1(sc->sc_bst, sc->sc_bsh, off);
		u.reg8 &= ~sc->sc_funcmask;
		u.reg8 |= val;
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, off, u.reg8);
		break;
	case 16:
		u.reg16 = bus_space_read_2(sc->sc_bst, sc->sc_bsh, off);
		u.reg16 &= ~sc->sc_funcmask;
		u.reg16 |= val;
		bus_space_write_2(sc->sc_bst, sc->sc_bsh, off, u.reg16);
		break;
	case 32:
		u.reg32 = bus_space_read_4(sc->sc_bst, sc->sc_bsh, off);
		u.reg32 &= ~sc->sc_funcmask;
		u.reg32 |= val;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, off, u.reg32);
		break;
	default:
		device_printf(sc->sc_dev, "%s: unsupported reg width %d\n",
		    __func__, sc->sc_regwidth);
		break;
	}
}

static int
pinctrl_single_pins_set_config(device_t dev, const void *data, size_t len)
{
	struct pinctrl_single_softc * const sc = device_private(dev);
	const u_int *pins;
	int pinslen;

	if (len != 4 && len != 8)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	pins = fdtbus_get_prop(phandle, "pinctrl-single,pins", &pinslen);
	if (pins == NULL)
		return -1;

	while (pinslen >= 4 + len) {
		const int off = be32toh(pins[0]);
		const int val = be32toh(pins[1]);
		const int mux = len == 4 ? 0 : be32toh(pins[2]);

		pinctrl_single_pins_write(sc, off, val | mux);
		pins += 1 + (len / 4);
		pinslen -= (4 + len);
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func pinctrl_single_pins_funcs = {
	.set_config = pinctrl_single_pins_set_config,
};

static int
pinctrl_single_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
pinctrl_single_attach(device_t parent, device_t self, void *aux)
{
	struct pinctrl_single_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct pinctrl_single_config *conf;
	bus_addr_t addr;
	bus_size_t size;
	int child;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	conf = of_compatible_lookup(phandle, compat_data)->data;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_flags = conf->flags;

	if (of_getprop_uint32(phandle, "pinctrl-single,register-width", &sc->sc_regwidth) != 0) {
		aprint_error(": missing 'pinctrl-single,register-width' property\n");
		return;
	}
	if (of_getprop_uint32(phandle, "pinctrl-single,function-mask", &sc->sc_funcmask) != 0) {
		aprint_error(": missing 'pinctrl-single,function-mask' property\n");
		return;
	}

	switch (sc->sc_regwidth) {
	case 8:
	case 16:
	case 32:
		break;
	default:
		aprint_error(": register width %d not supported\n", sc->sc_regwidth);
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (of_hasprop(child, "pinctrl-single,pins"))
			fdtbus_register_pinctrl_config(self, child, &pinctrl_single_pins_funcs);
	}

	fdtbus_pinctrl_set_config(phandle, "default");
}

CFATTACH_DECL_NEW(pinctrl_single, sizeof(struct pinctrl_single_softc),
	pinctrl_single_match, pinctrl_single_attach, NULL, NULL);
