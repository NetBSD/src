/* $NetBSD: fdt_spi.c,v 1.3 2021/08/07 16:19:10 thorpej Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
__KERNEL_RCSID(0, "$NetBSD: fdt_spi.c,v 1.3 2021/08/07 16:19:10 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <dev/spi/spivar.h>
#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_spi_controller {
	device_t spi_dev;
	int spi_phandle;
	const struct fdtbus_spi_controller_func *spi_funcs;
	LIST_ENTRY(fdtbus_spi_controller) spi_next;
};

static LIST_HEAD(, fdtbus_spi_controller) fdtbus_spi_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_spi_controllers);

int
fdtbus_register_spi_controller(device_t dev, int phandle,
    const struct fdtbus_spi_controller_func *funcs)
{
	struct fdtbus_spi_controller *spi;

	spi = kmem_alloc(sizeof(*spi), KM_SLEEP);
	spi->spi_dev = dev;
	spi->spi_phandle = phandle;
	spi->spi_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_spi_controllers, spi, spi_next);

	return 0;
}

static struct spi_controller *
fdtbus_get_spi_controller(int phandle)
{
	struct fdtbus_spi_controller *spi;

	LIST_FOREACH(spi, &fdtbus_spi_controllers, spi_next) {
		if (spi->spi_phandle == phandle) {
			return spi->spi_funcs->get_controller(spi->spi_dev);
		}
	}
	return NULL;
}

device_t
fdtbus_attach_spibus(device_t dev, int phandle, cfprint_t print)
{
	struct spi_controller *spi;
	struct spibus_attach_args sba;
	prop_dictionary_t devs;
	device_t ret;
	u_int address_cells;

	devs = prop_dictionary_create();
	if (of_getprop_uint32(phandle, "#address-cells", &address_cells))
		address_cells = 1;
	of_enter_spi_devs(devs, phandle, address_cells * 4);

	spi = fdtbus_get_spi_controller(phandle);
	KASSERT(spi != NULL);
	memset(&sba, 0, sizeof(sba));
	sba.sba_controller = spi;

	sba.sba_child_devices = prop_dictionary_get(devs, "spi-child-devices");
	if (sba.sba_child_devices)
		prop_object_retain(sba.sba_child_devices);
	prop_object_release(devs);

	ret = config_found(dev, &sba, print,
	    CFARGS(.iattr = "spibus"));
	if (sba.sba_child_devices)
		prop_object_release(sba.sba_child_devices);

	return ret;
}


