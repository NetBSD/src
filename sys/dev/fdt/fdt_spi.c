/* $NetBSD: fdt_spi.c,v 1.12 2025/09/14 00:28:43 thorpej Exp $ */

/*
 * Copyright (c) 2021, 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: fdt_spi.c,v 1.12 2025/09/14 00:28:43 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ofw/openfirm.h>

#include <dev/spi/spi_calls.h>

struct fdtbus_spi_controller {
	device_t spi_dev;
	const struct spi_controller *spi_controller;
	LIST_ENTRY(fdtbus_spi_controller) spi_next;
};

static LIST_HEAD(, fdtbus_spi_controller) fdtbus_spi_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_spi_controllers);

/* XXX Maybe garbage-collect this. */
int
fdtbus_register_spi_controller(device_t dev,
    const struct spi_controller *controller)
{
	struct fdtbus_spi_controller *spi;

	spi = kmem_alloc(sizeof(*spi), KM_SLEEP);
	spi->spi_dev = dev;
	spi->spi_controller = controller;

	LIST_INSERT_HEAD(&fdtbus_spi_controllers, spi, spi_next);

	return 0;
}

#if 0
static const struct spi_controller *
fdtbus_get_spi_controller(int phandle)
{
	struct fdtbus_spi_controller *spi;

	LIST_FOREACH(spi, &fdtbus_spi_controllers, spi_next) {
		if (phandle ==
		    devhandle_to_of(device_handle(spi->spi_dev))) {
			return spi->spi_controller;
		}
	}
	return NULL;
}
#endif

static int
fdtbus_spi_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct spi_enumerate_devices_args *args = v;
	int spi_node, node;
	char name[32], compat_buf[32];
	bus_addr_t chip_select;
	char *clist;
	int clist_size;
	bool cbrv;

	spi_node = devhandle_to_of(call_handle);

	for (node = OF_child(spi_node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0) {
			continue;
		}

		if (fdtbus_get_reg(node, 0, &chip_select, NULL) != 0) {
			continue;
		}

		/* Device Tree bindings specify a max chip select of 256. */
		if (chip_select > 256) {
			continue;
		}

		clist_size = OF_getproplen(node, "compatible");
		if (clist_size <= 0) {
			continue;
		}

		clist = kmem_tmpbuf_alloc(clist_size,
		    compat_buf, sizeof(compat_buf), KM_SLEEP);
		if (OF_getprop(node, "compatible", clist, clist_size) <
		    clist_size) {
			kmem_tmpbuf_free(clist, clist_size, compat_buf);
			continue;
		}

		args->chip_select = (int)chip_select;
		args->sa->sa_name = name;
		args->sa->sa_clist = clist;
		args->sa->sa_clist_size = clist_size;
		args->sa->sa_devhandle = devhandle_from_of(call_handle, node);

		cbrv = args->callback(dev, args);

		kmem_tmpbuf_free(clist, clist_size, compat_buf);

		if (!cbrv) {
			break;
		}
	}

	return 0;
}
OF_DEVICE_CALL_REGISTER(SPI_ENUMERATE_DEVICES_STR,
			fdtbus_spi_enumerate_devices);
