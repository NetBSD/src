/*	$NetBSD: i2cmux_fdt.c,v 1.2 2020/12/23 16:04:42 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i2cmux_fdt.c,v 1.2 2020/12/23 16:04:42 thorpej Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/i2c/i2cvar.h>

/*
 * i2c mux
 *
 * This works by interposing a virtual controller in front of the
 * real i2c controller.  We provide our own acquire release functions
 * that perform the following tasks:
 *
 *	acquire -> acquire parent controller, program mux
 *
 *	release -> idle mux, release parent controller
 *
 * All of the actual I/O operations are transparently passed through.
 *
 * N.B. the locking order; the generic I2C layer has already acquired
 * our virtual controller's mutex before calling our acquire function,
 * and we will then acquire the real controller's mutex when we acquire
 * the bus, so the order is:
 *
 *	mux virtual controller -> parent controller
 */

struct iicmux_softc;
struct iicmux_bus;

struct iicmux_config {
	const char *desc;
	int	(*get_mux_info)(struct iicmux_softc *);
	int	(*get_bus_info)(struct iicmux_bus *);
	int	(*acquire_bus)(struct iicmux_bus *, int);
	void	(*release_bus)(struct iicmux_bus *, int);
};

struct iicmux_bus {
	struct i2c_controller controller;
	struct iicmux_softc *mux;
	int phandle;
	int busidx;

	union {
		struct {
			bus_addr_t value;
		} gpio;

		struct {
			bus_addr_t idx;
		} pinctrl;
	};
};

struct iicmux_softc {
	device_t			sc_dev;
	int				sc_phandle;
	const struct iicmux_config *	sc_config;
	i2c_tag_t			sc_i2c_parent;
	struct iicmux_bus *		sc_busses;
	int				sc_nbusses;

	union {
		struct {
			struct fdtbus_gpio_pin **pins;
			int npins;
			uint32_t idle_value;
			bool has_idle_value;
		} sc_gpio;

		struct {
			u_int idle_idx;
			bool has_idle_idx;
		} sc_pinctrl;
	};
};

/*****************************************************************************/

static int
iicmux_acquire_bus(void * const v, int const flags)
{
	struct iicmux_bus * const bus = v;
	struct iicmux_softc * const sc = bus->mux;
	int error;

	error = iic_acquire_bus(sc->sc_i2c_parent, flags);
	if (error) {
		return error;
	}

	error = sc->sc_config->acquire_bus(bus, flags);
	if (error) {
		iic_release_bus(sc->sc_i2c_parent, flags);
	}

	return error;
}

static void
iicmux_release_bus(void * const v, int const flags)
{
	struct iicmux_bus * const bus = v;
	struct iicmux_softc * const sc = bus->mux;

	sc->sc_config->release_bus(bus, flags);
	iic_release_bus(sc->sc_i2c_parent, flags);
}

static int
iicmux_exec(void * const v, i2c_op_t const op, i2c_addr_t const addr,
    const void * const cmdbuf, size_t const cmdlen, void * const databuf,
    size_t const datalen, int const flags)
{
	struct iicmux_bus * const bus = v;
	struct iicmux_softc * const sc = bus->mux;

	return iic_exec(sc->sc_i2c_parent, op, addr, cmdbuf, cmdlen,
			databuf, datalen, flags);
}

/*****************************************************************************/

static int
iicmux_gpio_get_mux_info(struct iicmux_softc * const sc)
{
	int i;

	sc->sc_gpio.npins = fdtbus_gpio_count(sc->sc_phandle, "mux-gpios");
	if (sc->sc_gpio.npins == 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get mux-gpios property\n");
		return ENXIO;
	}

	sc->sc_gpio.pins =
	    kmem_zalloc(sizeof(*sc->sc_gpio.pins) * sc->sc_gpio.npins,
	    		       KM_SLEEP);
	for (i = 0; i < sc->sc_gpio.npins; i++) {
		sc->sc_gpio.pins[i] = fdtbus_gpio_acquire_index(sc->sc_phandle,
		    "mux-gpios", i, GPIO_PIN_OUTPUT);
		if (sc->sc_gpio.pins[i] == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to acquire gpio #%d\n", i);
			return ENXIO;
		}
	}

	sc->sc_gpio.has_idle_value =
	    of_getprop_uint32(sc->sc_phandle, "idle-state",
			      &sc->sc_gpio.idle_value) == 0;

	return 0;
}

static int
iicmux_gpio_get_bus_info(struct iicmux_bus * const bus)
{
	struct iicmux_softc * const sc = bus->mux;
	int error;

	error = fdtbus_get_reg(bus->phandle, 0, &bus->gpio.value, NULL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get reg property for bus %d\n", bus->busidx);
	}

	return error;
}

static void
iicmux_gpio_set_value(struct iicmux_softc * const sc, bus_addr_t value)
{
	int i;

	for (i = 0; i < sc->sc_gpio.npins; i++, value >>= 1) {
		fdtbus_gpio_write(sc->sc_gpio.pins[i], value & 1);
	}
}

static int
iicmux_gpio_acquire_bus(struct iicmux_bus * const bus, int const flags __unused)
{
	iicmux_gpio_set_value(bus->mux, bus->gpio.value);

	return 0;
}

static void
iicmux_gpio_release_bus(struct iicmux_bus * const bus, int const flags __unused)
{
	struct iicmux_softc *sc = bus->mux;

	if (sc->sc_gpio.has_idle_value) {
		iicmux_gpio_set_value(sc, sc->sc_gpio.idle_value);
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

static int
iicmux_pinctrl_get_mux_info(struct iicmux_softc * const sc)
{

	sc->sc_pinctrl.has_idle_idx =
	    fdtbus_get_index(sc->sc_phandle, "pinctrl-names", "idle",
			     &sc->sc_pinctrl.idle_idx) == 0;

	return 0;
}

static int
iicmux_pinctrl_get_bus_info(struct iicmux_bus * const bus)
{
	struct iicmux_softc * const sc = bus->mux;
	int error;

	error = fdtbus_get_reg(bus->phandle, 0, &bus->pinctrl.idx,
			       NULL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get reg property for bus %d\n", bus->busidx);
	}

	return error;
}

static int
iicmux_pinctrl_acquire_bus(struct iicmux_bus * const bus,
    int const flags __unused)
{
	struct iicmux_softc * const sc = bus->mux;

	return fdtbus_pinctrl_set_config_index(sc->sc_phandle,
	    bus->pinctrl.idx);
}

static void
iicmux_pinctrl_release_bus(struct iicmux_bus * const bus,
    int const flags __unused)
{
	struct iicmux_softc * const sc = bus->mux;

	if (sc->sc_pinctrl.has_idle_idx) {
		(void) fdtbus_pinctrl_set_config_index(sc->sc_phandle,
		    sc->sc_pinctrl.idle_idx);
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

static const struct of_compat_data compat_data[] = {
	{ .compat = "i2c-mux-gpio",
	  .data = (uintptr_t)&iicmux_gpio_config },

	{ .compat = "i2c-mux-pinctrl",
	  .data = (uintptr_t)&iicmux_pinctrl_config },

	{ NULL }
};

static int
iicmux_count_children(struct iicmux_softc * const sc)
{
	int child, count;

	for (child = OF_child(sc->sc_phandle), count = 0; child;
	     child = OF_peer(child)) {
		count++;
	}

	return count;
}

/* XXX iicbus_print() should be able to do this. */
static int
iicmux_print(void * const aux, const char * const pnp)
{
	i2c_tag_t const tag = aux;
	struct iicmux_bus * const bus = tag->ic_cookie;
	int rv;

	rv = iicbus_print(aux, pnp);
	aprint_normal(" bus %d", bus->busidx);

	return rv;
}

static void
iicmux_attach_bus(struct iicmux_softc * const sc,
    int const phandle, int const busidx)
{
	struct iicmux_bus * const bus = &sc->sc_busses[busidx];
	int error;

	bus->mux = sc;
	bus->busidx = busidx;
	bus->phandle = phandle;

	error = sc->sc_config->get_bus_info(bus);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get info for bus %d, error %d\n",
		    busidx, error);
		return;
	}

	iic_tag_init(&bus->controller);
	bus->controller.ic_cookie = bus;
	bus->controller.ic_acquire_bus = iicmux_acquire_bus;
	bus->controller.ic_release_bus = iicmux_release_bus;
	bus->controller.ic_exec = iicmux_exec;

	fdtbus_register_i2c_controller(&bus->controller, bus->phandle);

	fdtbus_attach_i2cbus(sc->sc_dev, bus->phandle, &bus->controller,
	    iicmux_print);
}

static int
iicmux_fdt_match(device_t const parent, cfdata_t const match, void * const aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
iicmux_fdt_attach(device_t const parent, device_t const self, void * const aux)
{
	struct iicmux_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_config = (const struct iicmux_config *)
	    of_search_compatible(sc->sc_phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": %s I2C mux\n", sc->sc_config->desc);

	sc->sc_i2c_parent = fdtbus_i2c_acquire(sc->sc_phandle, "i2c-parent");
	if (sc->sc_i2c_parent == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to acquire i2c-parent\n");
		return;
	}

	const int nchildren = iicmux_count_children(sc);
	if (nchildren == 0) {
		return;
	}

	sc->sc_busses = kmem_zalloc(sizeof(*sc->sc_busses) * nchildren,
	    KM_SLEEP);

	int child, idx;
	for (child = OF_child(sc->sc_phandle), idx = 0; child;
	     child = OF_peer(child), idx++) {
		KASSERT(idx < nchildren);
		iicmux_attach_bus(sc, child, idx);
	}
}

CFATTACH_DECL_NEW(iicmux_fdt, sizeof(struct iicmux_softc),
    iicmux_fdt_match, iicmux_fdt_attach, NULL, NULL); 
