/*	$NetBSD: i2cmux.c,v 1.7 2021/11/10 15:39:03 msaitoh Exp $	*/

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

#if defined(__i386__) || defined(__amd64__) || defined(__aarch64__)
#include "acpica.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2cmux.c,v 1.7 2021/11/10 15:39:03 msaitoh Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2cmuxvar.h>

#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_i2c.h>
#endif

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

static int
iicmux_count_children(struct iicmux_softc * const sc)
{
	char name[32];
	int child, count;

 restart:
	for (child = OF_child(sc->sc_i2c_mux_phandle), count = 0; child;
	     child = OF_peer(child)) {
		if (OF_getprop(child, "name", name, sizeof(name)) <= 0) {
			continue;
		}
		if (strcmp(name, "i2c-mux") == 0) {
			/*
			 * The node we encountered is the actual parent
			 * of the i2c bus children.  Stash its phandle
			 * and restart the enumeration.
			 */
			sc->sc_i2c_mux_phandle = child;
			goto restart;
		}
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
    uintptr_t const handle, enum i2c_cookie_type handletype, int const busidx)
{
	struct iicmux_bus * const bus = &sc->sc_busses[busidx];

	bus->mux = sc;
	bus->busidx = busidx;
	bus->handle = handle;
	bus->handletype = handletype;

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

	switch (handletype) {
	case I2C_COOKIE_OF:
		fdtbus_register_i2c_controller(&bus->controller,
		    (int)bus->handle);

		fdtbus_attach_i2cbus(sc->sc_dev, (int)bus->handle,
		    &bus->controller, iicmux_print);
		break;
#if NACPICA > 0
	case I2C_COOKIE_ACPI: {
		struct acpi_devnode *ad = acpi_match_node((ACPI_HANDLE)handle);
		KASSERT(ad != NULL);
		struct i2cbus_attach_args iba = {
			.iba_tag = &bus->controller,
			.iba_child_devices = acpi_enter_i2c_devs(NULL, ad)
		};
		config_found(sc->sc_dev, &iba, iicbus_print, CFARGS_NONE);
	}	break;
#endif
	default:
		aprint_error_dev(sc->sc_dev, "unknown handle type\n");
		break;
	}
}

static void
iicmux_attach_fdt(struct iicmux_softc * const sc)
{
	/*
	 * We start out assuming that the i2c bus nodes are children of
	 * our own node.  We'll adjust later if we encounter an "i2c-mux"
	 * node when counting our children.  If we encounter such a node,
	 * then it's that node that is the parent of the i2c bus children.
	 */
	sc->sc_i2c_mux_phandle = (int)sc->sc_handle;

	sc->sc_nbusses = iicmux_count_children(sc);
	if (sc->sc_nbusses == 0) {
		return;
	}

	sc->sc_busses = kmem_zalloc(sizeof(*sc->sc_busses) * sc->sc_nbusses,
	    KM_SLEEP);

	int child, idx;
	for (child = OF_child(sc->sc_i2c_mux_phandle), idx = 0; child;
	     child = OF_peer(child), idx++) {
		KASSERT(idx < sc->sc_nbusses);
		iicmux_attach_bus(sc, child, I2C_COOKIE_OF, idx);
	}
}

#if NACPICA > 0
static void
iicmux_attach_acpi(struct iicmux_softc * const sc)
{
	ACPI_HANDLE hdl = (ACPI_HANDLE)sc->sc_handle;
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
		iicmux_attach_bus(sc, (uintptr_t)ad->ad_handle,
		    I2C_COOKIE_ACPI, idx);
		idx++;
	}
}
#endif

void
iicmux_attach(struct iicmux_softc * const sc)
{
	/*
	 * We expect sc->sc_handle, sc->sc_config, and sc->sc_i2c_parent
	 * to be initialized by the front-end.
	 */
	KASSERT(sc->sc_handle > 0);
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
	 * Do configuration method (OF, ACPI) specific setup.
	 */
	switch (sc->sc_handletype) {
	case I2C_COOKIE_OF:
		iicmux_attach_fdt(sc);
		break;
#if NACPICA > 0
	case I2C_COOKIE_ACPI:
		iicmux_attach_acpi(sc);
		break;
#endif
	default:
		aprint_error_dev(sc->sc_dev, "could not configure mux: "
		    "handle type %u not supported\n", sc->sc_handletype);
		break;
	}
}
