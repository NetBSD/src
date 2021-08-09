/*	$NetBSD: i2cmux.c,v 1.6.2.2 2021/08/09 00:57:56 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i2cmux.c,v 1.6.2.2 2021/08/09 00:57:56 thorpej Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2cmuxvar.h>

#include "locators.h"

/*
 * i2c mux
 *
 * This works by interposing a set of virtual controllers behind the real
 * i2c controller.  We provide our own acquire and release functions that
 * perform the following tasks:
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
 *
 * These are common routines used by various i2c mux controller
 * implementations (gpio, pin mux, i2c device, etc.).
 */

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

static void
iicmux_attach_bus(struct iicmux_softc * const sc, devhandle_t devhandle,
    int const busidx)
{
	struct iicmux_bus * const bus = &sc->sc_busses[busidx];

	bus->mux = sc;
	bus->busidx = busidx;
	bus->devhandle = devhandle;

	bus->bus_data = sc->sc_config->get_bus_info(bus);
	if (bus->bus_data == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get info for bus %d\n", busidx);
		return;
	}

	iic_tag_init(&bus->controller);
	bus->controller.ic_cookie = bus;
	bus->controller.ic_channel = bus->busidx;
	bus->controller.ic_acquire_bus = iicmux_acquire_bus;
	bus->controller.ic_release_bus = iicmux_release_bus;
	bus->controller.ic_exec = iicmux_exec;

#if defined(I2CMUX_USE_FDT)
	if (devhandle_type(devhandle) == DEVHANDLE_TYPE_OF) {
		fdtbus_register_i2c_controller(&bus->controller,
		    devhandle_to_of(devhandle));
	}
#endif /* I2CMUX_USE_FDT */

	struct i2cbus_attach_args iba = {
		.iba_tag = &bus->controller,
	};

	int locs[I2CBUSCF_NLOCS];
	locs[I2CBUSCF_BUS] = bus->busidx;

	config_found(sc->sc_dev, &iba, iicbus_print_multi,
	    CFARGS(.submatch = config_stdsubmatch,
		   .locators = locs,
		   .devhandle = devhandle));
}

static bool
iicmux_count_busses_callback(device_t self, devhandle_t devhandle,
    void *v __unused)
{
	struct iicmux_softc *sc = device_private(self);

#if defined(I2CMUX_USE_FDT)
	if (devhandle_type(devhandle) == DEVHANDLE_TYPE_OF) {
		char name[32];

		if (OF_getprop(devhandle_to_of(devhandle), "name", name,
			       sizeof(name)) <= 0) {
			/* Skip this DT node (shouldn't happen). */
			return true;	/* keep enumerating */
		}
		if (strcmp(name, "i2c-mux") == 0) {
			/*
			 * This DT node is the actual mux node; reset the
			 * our devhandle and restart enumeration.
			 */
			device_set_handle(self, devhandle);
			sc->sc_nbusses = -1;
			return false;	/* stop enumerating */
		}
	}
#endif /* I2CMUX_USE_FDT */

	sc->sc_nbusses++;
	return true;			/* keep enumerating */
}

static bool
iicmux_attach_busses_callback(device_t self, devhandle_t devhandle, void *v)
{
	struct iicmux_softc *sc = device_private(self);
	int * const idxp = v;

	KASSERT(*idxp < sc->sc_nbusses);
	iicmux_attach_bus(sc, devhandle, (*idxp)++);

	return true;			/* keep enumerating */
}

void
iicmux_attach(struct iicmux_softc * const sc)
{
	int error, idx;

	/*
	 * We expect sc->sc_config and sc->sc_i2c_parent to be initialized
	 * by the front-end.
	 */
	KASSERT(sc->sc_config != NULL);
	KASSERT(sc->sc_i2c_parent != NULL);

	/*
	 * Gather up all of the various bits of information needed
	 * for this particular type of i2c mux.
	 */
	sc->sc_mux_data = sc->sc_config->get_mux_info(sc);
	if (sc->sc_mux_data == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to get info for mux\n");
		return;
	}

	/*
	 * Count the number of busses behind this mux.
	 */
 count_again:
	error = device_enumerate_children(sc->sc_dev,
	    iicmux_count_busses_callback, NULL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to enumerate busses to count, error = %d\n", error);
		return;
	}
	if (sc->sc_nbusses == -1) {
		/*
		 * We had to reset our devhandle to a different device
		 * tree node.  We need to try counting again.
		 */
		sc->sc_nbusses = 0;
		goto count_again;
	}
	if (sc->sc_nbusses == 0) {
		/* No busses; no more work to do. */
		return;
	}

	sc->sc_busses = kmem_zalloc(sizeof(*sc->sc_busses) * sc->sc_nbusses,
	    KM_SLEEP);

	/* Now attach them. */
	idx = 0;
	error = device_enumerate_children(sc->sc_dev,
	    iicmux_attach_busses_callback, &idx);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to enumerate busses to attach, error = %d\n",
		    error);
	}
}
