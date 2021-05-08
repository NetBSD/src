/*	$NetBSD: i2cmux.c,v 1.5.2.2 2021/05/08 14:23:15 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i2cmux.c,v 1.5.2.2 2021/05/08 14:23:15 thorpej Exp $");

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
		.iba_bus = bus->busidx,
	};

	int locs[I2CBUSCF_NLOCS];
	locs[I2CBUSCF_BUS] = bus->busidx;

	config_found(sc->sc_dev, &iba, iicbus_print_multi,
	    CFARG_SUBMATCH, config_stdsubmatch,
	    CFARG_LOCATORS, locs,
	    CFARG_DEVHANDLE, devhandle,
	    CFARG_EOL);
}

#if defined(I2CMUX_USE_FDT)
static int
iicmux_fdt_count_children(struct iicmux_softc * const sc)
{
	int phandle = devhandle_to_of(sc->sc_i2c_mux_devhandle);
	char name[32];
	int child, count;

 restart:
	for (child = OF_child(phandle), count = 0; child;
	     child = OF_peer(child)) {
		if (OF_getprop(child, "name", name, sizeof(name)) <= 0) {
			continue;
		}
		if (strcmp(name, "i2c-mux") == 0) {
			phandle = child;
			goto restart;
		}
		count++;
	}

	/* phandle may have changed. */
	sc->sc_i2c_mux_devhandle = devhandle_from_of(phandle);
	return count;
}

static void
iicmux_attach_fdt(struct iicmux_softc * const sc)
{
	int phandle = devhandle_to_of(sc->sc_i2c_mux_devhandle);

	sc->sc_nbusses = iicmux_fdt_count_children(sc);
	if (sc->sc_nbusses == 0) {
		return;
	}

	/* sc_i2c_mux_devhandle may have changed. */
	phandle = devhandle_to_of(sc->sc_i2c_mux_devhandle);

	sc->sc_busses = kmem_zalloc(sizeof(*sc->sc_busses) * sc->sc_nbusses,
	    KM_SLEEP);

	int child, idx;
	for (child = OF_child(phandle), idx = 0; child;
	     child = OF_peer(child), idx++) {
		KASSERT(idx < sc->sc_nbusses);
		iicmux_attach_bus(sc, devhandle_from_of(child), idx);
	}
}
#endif /* I2CMUX_USE_FDT */

#if defined(I2CMUX_USE_ACPI)
static void
iicmux_attach_acpi(struct iicmux_softc * const sc)
{
	ACPI_HANDLE hdl = devhandle_to_acpi(sc->sc_i2c_mux_devhandle);
	struct acpi_devnode *devnode, *ad;
	int idx;

	devnode = acpi_match_node(hdl);
	KASSERT(devnode != NULL);

	/* Count child busses */
	sc->sc_nbusses = 0;
	SIMPLEQ_FOREACH(ad, &devnode->ad_child_head, ad_child_list) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE ||
		    !acpi_device_present(ad->ad_handle)) {
			continue;
		}
		sc->sc_nbusses++;
	}

	sc->sc_busses = kmem_zalloc(sizeof(*sc->sc_busses) * sc->sc_nbusses,
	    KM_SLEEP);

	/* Attach child busses */
	idx = 0;
	SIMPLEQ_FOREACH(ad, &devnode->ad_child_head, ad_child_list) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE ||
		    !acpi_device_present(ad->ad_handle)) {
			continue;
		}
		iicmux_attach_bus(sc, devhandle_from_acpi(ad->ad_handle), idx);
		idx++;
	}
}
#endif /* I2CMUX_USE_ACPI */

void
iicmux_attach(struct iicmux_softc * const sc)
{
	devhandle_t devhandle = device_handle(sc->sc_dev);

	/*
	 * We expect sc->sc_config and sc->sc_i2c_parent to be initialized
	 * by the front-end.
	 */
	KASSERT(sc->sc_config != NULL);
	KASSERT(sc->sc_i2c_parent != NULL);

	/*
	 * We start out assuming that the i2c bus nodes are children of
	 * our own node.  We'll adjust later if we encounter a subnode
	 * while enumerating our child busses that itself is defined as
	 * the mux node according to our device tree bindings.
	 */
	sc->sc_i2c_mux_devhandle = devhandle;

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
	 * Do configuration method (OF, ACPI) specific setup.
	 */
	switch (devhandle_type(devhandle)) {
#if defined(I2CMUX_USE_FDT)
	case DEVHANDLE_TYPE_OF:
		iicmux_attach_fdt(sc);
		break;
#endif /* I2CMUX_USE_FDT */

#if defined(I2CMUX_USE_ACPI)
	case DEVHANDLE_TYPE_ACPI:
		iicmux_attach_acpi(sc);
		break;
#endif /* I2CMUX_USE_ACPI */

	default:
		aprint_error_dev(sc->sc_dev, "could not configure mux: "
		    "handle type %d not supported\n",
		    devhandle_type(devhandle));
		break;
	}
}
