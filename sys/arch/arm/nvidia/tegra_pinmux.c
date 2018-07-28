/* $NetBSD: tegra_pinmux.c,v 1.1.4.1 2018/07/28 04:37:28 pgoyette Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_tegra.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_pinmux.c,v 1.1.4.1 2018/07/28 04:37:28 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_pinmux.h>

#include <dev/fdt/fdtvar.h>

/* PINMUX fields */
#define	PINMUX_DRV_TYPE		__BITS(14,13)
#define	PINMUX_E_SCHMT		__BIT(12)
#define	PINMUX_E_OD		__BIT(11)
#define	PINMUX_E_IO_HV		__BIT(10)
#define	PINMUX_E_HSM		__BIT(9)
#define	PINMUX_LOCK		__BIT(7)
#define	PINMUX_E_INPUT		__BIT(6)
#define	PINMUX_PARK		__BIT(5)
#define	PINMUX_TRISTATE		__BIT(4)
#define	PINMUX_PUPD		__BITS(3,2)
#define	PINMUX_PM		__BITS(1,0)

struct tegra_pinmux_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh[2];
	const struct tegra_pinmux_conf *sc_conf;
};

#define PADCTRL_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh[0], (reg), (val))
#define PADCTRL_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh[0], (reg))
#define PINMUX_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh[1], (reg), (val))
#define PINMUX_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh[1], (reg))

static const struct of_compat_data compat_data[] = {
#ifdef SOC_TEGRA210
	{ "nvidia,tegra210-pinmux",	(uintptr_t)&tegra210_pinmux_conf },
#endif
	{ NULL }
};

static const struct tegra_pinmux_pins *
tegra_pinmux_lookup_byname(struct tegra_pinmux_softc *sc, const char *name)
{
	const struct tegra_pinmux_pins *pin_def;
	u_int n;

	for (n = 0; n < sc->sc_conf->npins; n++) {
		pin_def = &sc->sc_conf->pins[n];
		if (strcmp(pin_def->name, name) == 0)
			return pin_def;
	}

	return NULL;
}

static int
tegra_pinmux_lookup_func(const struct tegra_pinmux_pins *pin_def, const int phandle)
{
	const char *func;
	u_int n, valid;

	func = fdtbus_get_string(phandle, "nvidia,function");
	if (func == NULL)
		return -1;

	for (n = 0, valid = 0; n < TEGRA_PINMUX_MAXFUNC; n++) {
		if (pin_def->functions[n] == NULL)
			continue;
		++valid;
		if (strcmp(pin_def->functions[n], func) == 0)
			return n;
	}

	if (valid > 0)
		aprint_error("%s: pin %s does not support function %s\n",
		    __func__, pin_def->name, func);

	return -1;
}

static void
tegra_pinmux_pin_config(struct tegra_pinmux_softc *sc,
    const struct tegra_pinmux_pins *pin_def, const int phandle)
{
	uint32_t cfg;
	u_int val;

	cfg = PINMUX_READ(sc, pin_def->reg);
	const uint32_t ocfg = cfg;

	const int func = tegra_pinmux_lookup_func(pin_def, phandle);
	if (func != -1) {
		cfg &= ~PINMUX_PM;
		cfg |= __SHIFTIN(func, PINMUX_PM);
	}
	if (of_getprop_uint32(phandle, "nvidia,pull", &val) == 0) {
		cfg &= ~PINMUX_PUPD;
		cfg |= __SHIFTIN(val, PINMUX_PUPD);
	}
	if (of_getprop_uint32(phandle, "nvidia,tristate", &val) == 0) {
		cfg &= ~PINMUX_TRISTATE;
		cfg |= __SHIFTIN(val, PINMUX_TRISTATE);
	}
	if (of_getprop_uint32(phandle, "nvidia,open-drain", &val) == 0) {
		cfg &= ~PINMUX_E_OD;
		cfg |= __SHIFTIN(val, PINMUX_E_OD);
	}
	if (of_getprop_uint32(phandle, "nvidia,lock", &val) == 0) {
		cfg &= ~PINMUX_LOCK;
		cfg |= __SHIFTIN(val, PINMUX_LOCK);
	}
	if (of_getprop_uint32(phandle, "nvidia,io-hv", &val) == 0) {
		cfg &= ~PINMUX_E_IO_HV;
		cfg |= __SHIFTIN(val, PINMUX_E_IO_HV);
	}
	if (of_getprop_uint32(phandle, "nvidia,high-speed-mode", &val) == 0) {
		cfg &= ~PINMUX_E_HSM;
		cfg |= __SHIFTIN(val, PINMUX_E_HSM);
	}
	if (of_getprop_uint32(phandle, "nvidia,schmitt", &val) == 0) {
		cfg &= ~PINMUX_E_SCHMT;
		cfg |= __SHIFTIN(val, PINMUX_E_SCHMT);
	}
	if (of_getprop_uint32(phandle, "nvidia,drive-type", &val) == 0) {
		cfg &= ~PINMUX_DRV_TYPE;
		cfg |= __SHIFTIN(val, PINMUX_DRV_TYPE);
	}

	aprint_debug_dev(sc->sc_dev, "pin %s %08x -> %08x\n", pin_def->name, ocfg, cfg);
	if (cfg != ocfg)
		PINMUX_WRITE(sc, pin_def->reg, cfg);

	/*
	 * Not yet supported (PADCTRL):
	 *   nvidia,pull-down-strength, nvidia,pull-up-strength
	 *   nvidia,slew-rate-rising, nvidia,slew-rate-falling
	 */
}

static int
tegra_pinmux_set_config(device_t dev, const void *data, size_t len)
{
	struct tegra_pinmux_softc * const sc = device_private(dev);
	const struct tegra_pinmux_pins *pin_def;
	int child;

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		const char *pins = fdtbus_get_string(child, "nvidia,pins");
		if (pins == NULL) {
			aprint_error_dev(dev, "skipping %s (no nvidia,pins property)\n",
			    fdtbus_get_string(child, "name"));
			continue;
		}
		int pins_len = OF_getproplen(child, "nvidia,pins");

		for (; pins_len > 0;
		    pins_len -= strlen(pins) + 1, pins += strlen(pins) + 1) {
			pin_def = tegra_pinmux_lookup_byname(sc, pins);
			if (pin_def == NULL) {
				aprint_error_dev(dev, "unknown pin name '%s'\n", pins);
				continue;
			}

			tegra_pinmux_pin_config(sc, pin_def, child);
		}
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func tegra_pinmux_funcs = {
	.set_config = tegra_pinmux_set_config,
};

static int
tegra_pinmux_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
tegra_pinmux_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_pinmux_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error, res;
	int child;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	for (res = 0; res < __arraycount(sc->sc_bsh); res++) {
		error = fdtbus_get_reg(phandle, res, &addr, &size);
		if (error != 0) {
			aprint_error(": couldn't get resource %d: %d\n", res, error);
			return;
		}
		error = bus_space_map(sc->sc_bst, addr, size, res, &sc->sc_bsh[res]);
		if (error) {
			aprint_error(": couldn't map %#" PRIx64 ": %d",
			    (uint64_t)addr, error);
			return;
		}
	}
	sc->sc_conf = (void *)of_search_compatible(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": Pinmux\n");

	for (child = OF_child(phandle); child; child = OF_peer(child))
		fdtbus_register_pinctrl_config(self, child, &tegra_pinmux_funcs);

	fdtbus_pinctrl_configure();
}

CFATTACH_DECL_NEW(tegra_pinmux, sizeof(struct tegra_pinmux_softc),
	tegra_pinmux_match, tegra_pinmux_attach, NULL, NULL);
