/* $NetBSD: usbnopphy.c,v 1.2 2021/01/27 03:10:21 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: usbnopphy.c,v 1.2 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

static int usbnopphy_match(device_t, cfdata_t, void *);
static void usbnopphy_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "usb-nop-xceiv" },
	DEVICE_COMPAT_EOL
};

struct usbnopphy_softc {
	device_t		sc_dev;

	struct clk		*sc_clk;
	struct fdtbus_regulator *sc_reg;
	struct fdtbus_gpio_pin	*sc_pin_reset;
	struct fdtbus_gpio_pin	*sc_pin_vbus_det;
};

CFATTACH_DECL_NEW(usbnopphy, sizeof(struct usbnopphy_softc),
	usbnopphy_match, usbnopphy_attach, NULL, NULL);

static void *
usbnopphy_acquire(device_t dev, const void *data, size_t len)
{
	struct usbnopphy_softc * const sc = device_private(dev);

	return sc;
}

static void
usbnopphy_release(device_t dev, void *priv)
{
}

static int
usbnopphy_enable(device_t dev, void *priv, bool enable)
{
	struct usbnopphy_softc * const sc = device_private(dev);
	int error;

	if (enable) {
		if (sc->sc_reg != NULL) {
			error = fdtbus_regulator_enable(sc->sc_reg);
			if (error != 0)
				return error;
		}
		if (sc->sc_clk != NULL) {
			error = clk_enable(sc->sc_clk);
			if (error != 0)
				return error;
		}
		if (sc->sc_pin_reset != NULL) {
			fdtbus_gpio_write(sc->sc_pin_reset, 1);
			delay(20000);
			fdtbus_gpio_write(sc->sc_pin_reset, 0);
		}
	} else {
		if (sc->sc_pin_reset != NULL)
			fdtbus_gpio_write(sc->sc_pin_reset, 1);
		if (sc->sc_reg != NULL)
			fdtbus_regulator_disable(sc->sc_reg);
		if (sc->sc_clk != NULL)
			clk_disable(sc->sc_clk);
	}

	return 0;
}

const struct fdtbus_phy_controller_func usbnopphy_funcs = {
	.acquire = usbnopphy_acquire,
	.release = usbnopphy_release,
	.enable = usbnopphy_enable,
};

static int
usbnopphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
usbnopphy_attach(device_t parent, device_t self, void *aux)
{
	struct usbnopphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;

	sc->sc_reg = fdtbus_regulator_acquire(phandle, "vbus-regulator");
	sc->sc_clk = fdtbus_clock_get(phandle, "main_clk");
	sc->sc_pin_reset = fdtbus_gpio_acquire(phandle, "reset-gpios", GPIO_PIN_OUTPUT);

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	fdtbus_register_phy_controller(self, phandle, &usbnopphy_funcs);
}
