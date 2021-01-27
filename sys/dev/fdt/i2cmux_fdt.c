/*	$NetBSD: i2cmux_fdt.c,v 1.10 2021/01/27 03:10:21 thorpej Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: i2cmux_fdt.c,v 1.10 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/i2c/i2cmuxvar.h>

/*****************************************************************************/

struct mux_info_gpio {
	struct fdtbus_gpio_pin **pins;
	int npins;
	uint32_t idle_value;
	bool has_idle_value;
};

struct bus_info_gpio {
	bus_addr_t value;
};

static void *
iicmux_gpio_get_mux_info(struct iicmux_softc * const sc)
{
	struct mux_info_gpio *mux_data;
	int i;

	mux_data = kmem_zalloc(sizeof(*mux_data), KM_SLEEP);

	mux_data->npins = fdtbus_gpio_count(sc->sc_handle, "mux-gpios");
	if (mux_data->npins == 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get mux-gpios property\n");
		goto bad;
	}

	mux_data->pins =
	    kmem_zalloc(sizeof(*mux_data->pins) * mux_data->npins, KM_SLEEP);
	for (i = 0; i < mux_data->npins; i++) {
		mux_data->pins[i] = fdtbus_gpio_acquire_index(sc->sc_handle,
		    "mux-gpios", i, GPIO_PIN_OUTPUT);
		if (mux_data->pins[i] == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to acquire gpio #%d\n", i);
			goto bad;
		}
	}

	mux_data->has_idle_value =
	    of_getprop_uint32(sc->sc_handle, "idle-state",
			      &mux_data->idle_value) == 0;

	return mux_data;

 bad:
	for (i = 0; i < mux_data->npins; i++) {
		if (mux_data->pins[i] != NULL) {
			fdtbus_gpio_release(mux_data->pins[i]);
		}
	}
	kmem_free(mux_data, sizeof(*mux_data));
	return NULL;
}

static void *
iicmux_gpio_get_bus_info(struct iicmux_bus * const bus)
{
	struct iicmux_softc * const sc = bus->mux;
	struct bus_info_gpio *bus_info;
	int error;

	bus_info = kmem_zalloc(sizeof(*bus_info), KM_SLEEP);

	error = fdtbus_get_reg(bus->handle, 0, &bus_info->value, NULL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get reg property for bus %d\n", bus->busidx);
		kmem_free(bus_info, sizeof(*bus_info));
		return NULL;
	}

	return bus_info;
}

static void
iicmux_gpio_set_value(struct iicmux_softc * const sc, bus_addr_t value)
{
	struct mux_info_gpio * const mux_info = sc->sc_mux_data;
	int i;

	for (i = 0; i < mux_info->npins; i++, value >>= 1) {
		fdtbus_gpio_write(mux_info->pins[i], value & 1);
	}
}

static int
iicmux_gpio_acquire_bus(struct iicmux_bus * const bus, int const flags __unused)
{
	struct bus_info_gpio * const bus_info = bus->bus_data;

	iicmux_gpio_set_value(bus->mux, bus_info->value);

	return 0;
}

static void
iicmux_gpio_release_bus(struct iicmux_bus * const bus, int const flags __unused)
{
	struct iicmux_softc * const sc = bus->mux;
	struct mux_info_gpio * const mux_info = sc->sc_mux_data;

	if (mux_info->has_idle_value) {
		iicmux_gpio_set_value(sc, mux_info->idle_value);
	}
}

static const struct iicmux_config iicmux_gpio_config = {
	.desc = "GPIO",
	.get_mux_info = iicmux_gpio_get_mux_info,
	.get_bus_info = iicmux_gpio_get_bus_info,
	.acquire_bus = iicmux_gpio_acquire_bus,
	.release_bus = iicmux_gpio_release_bus,
};

/*****************************************************************************/

struct mux_info_pinctrl {
	u_int idle_idx;
	bool has_idle_idx;
} sc_pinctrl;

struct bus_info_pinctrl {
	bus_addr_t idx;
};

static void *
iicmux_pinctrl_get_mux_info(struct iicmux_softc * const sc)
{
	struct mux_info_pinctrl *mux_info;

	mux_info = kmem_alloc(sizeof(*mux_info), KM_SLEEP);

	mux_info->has_idle_idx =
	    fdtbus_get_index(sc->sc_handle, "pinctrl-names", "idle",
			     &mux_info->idle_idx) == 0;

	return mux_info;
}

static void *
iicmux_pinctrl_get_bus_info(struct iicmux_bus * const bus)
{
	struct iicmux_softc * const sc = bus->mux;
	struct bus_info_pinctrl *bus_info;
	int error;

	bus_info = kmem_alloc(sizeof(*bus_info), KM_SLEEP);

	error = fdtbus_get_reg(bus->handle, 0, &bus_info->idx, NULL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get reg property for bus %d\n", bus->busidx);
		kmem_free(bus_info, sizeof(*bus_info));
		return NULL;
	}

	return bus_info;
}

static int
iicmux_pinctrl_acquire_bus(struct iicmux_bus * const bus,
    int const flags __unused)
{
	struct iicmux_softc * const sc = bus->mux;
	struct bus_info_pinctrl * const bus_info = bus->bus_data;

	return fdtbus_pinctrl_set_config_index(sc->sc_handle, bus_info->idx);
}

static void
iicmux_pinctrl_release_bus(struct iicmux_bus * const bus,
    int const flags __unused)
{
	struct iicmux_softc * const sc = bus->mux;
	struct mux_info_pinctrl * const mux_info = sc->sc_mux_data;

	if (mux_info->has_idle_idx) {
		(void) fdtbus_pinctrl_set_config_index(sc->sc_handle,
		    mux_info->idle_idx);
	}
}

static const struct iicmux_config iicmux_pinctrl_config = {
	.desc = "PinMux",
	.get_mux_info = iicmux_pinctrl_get_mux_info,
	.get_bus_info = iicmux_pinctrl_get_bus_info,
	.acquire_bus = iicmux_pinctrl_acquire_bus,
	.release_bus = iicmux_pinctrl_release_bus,
};

/*****************************************************************************/

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "i2c-mux-gpio",
	  .data = &iicmux_gpio_config },

	{ .compat = "i2c-mux-pinctrl",
	  .data = &iicmux_pinctrl_config },

	DEVICE_COMPAT_EOL
};

static int
iicmux_fdt_match(device_t const parent, cfdata_t const match, void * const aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
iicmux_fdt_attach(device_t const parent, device_t const self, void * const aux)
{
	struct iicmux_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_handle = faa->faa_phandle;
	sc->sc_config = of_compatible_lookup(sc->sc_handle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": %s I2C mux\n", sc->sc_config->desc);

	sc->sc_i2c_parent = fdtbus_i2c_acquire(sc->sc_handle, "i2c-parent");
	if (sc->sc_i2c_parent == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to acquire i2c-parent\n");
		return;
	}

	iicmux_attach(sc);
}

CFATTACH_DECL_NEW(iicmux_fdt, sizeof(struct iicmux_softc),
    iicmux_fdt_match, iicmux_fdt_attach, NULL, NULL);
