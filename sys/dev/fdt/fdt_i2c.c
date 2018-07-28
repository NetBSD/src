/* $NetBSD: fdt_i2c.c,v 1.2.20.1 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_i2c.c,v 1.2.20.1 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_i2c_controller {
	device_t i2c_dev;
	int i2c_phandle;
	const struct fdtbus_i2c_controller_func *i2c_funcs;

	LIST_ENTRY(fdtbus_i2c_controller) i2c_next;
};

static LIST_HEAD(, fdtbus_i2c_controller) fdtbus_i2c_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_i2c_controllers);

int
fdtbus_register_i2c_controller(device_t dev, int phandle,
    const struct fdtbus_i2c_controller_func *funcs)
{
	struct fdtbus_i2c_controller *i2c;

	i2c = kmem_alloc(sizeof(*i2c), KM_SLEEP);
	i2c->i2c_dev = dev;
	i2c->i2c_phandle = phandle;
	i2c->i2c_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_i2c_controllers, i2c, i2c_next);

	return 0;
}

static struct fdtbus_i2c_controller *
fdtbus_get_i2c_controller(int phandle)
{
	struct fdtbus_i2c_controller *i2c;

	LIST_FOREACH(i2c, &fdtbus_i2c_controllers, i2c_next) {
		if (i2c->i2c_phandle == phandle)
			return i2c;
	}

	return NULL;
}

i2c_tag_t
fdtbus_get_i2c_tag(int phandle)
{
	struct fdtbus_i2c_controller *i2c;

	i2c = fdtbus_get_i2c_controller(phandle);
	if (i2c == NULL)
		return NULL;

	return i2c->i2c_funcs->get_tag(i2c->i2c_dev);
}

device_t
fdtbus_attach_i2cbus(device_t dev, int phandle, i2c_tag_t tag, cfprint_t print)
{
	struct i2cbus_attach_args iba;
	prop_dictionary_t devs;
	u_int address_cells;

	devs = prop_dictionary_create();
	if (of_getprop_uint32(phandle, "#address-cells", &address_cells))
		address_cells = 1;

	of_enter_i2c_devs(devs, phandle, address_cells * 4, 0);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = tag;
	iba.iba_child_devices = prop_dictionary_get(devs, "i2c-child-devices");
	if (iba.iba_child_devices)
		prop_object_retain(iba.iba_child_devices);
	prop_object_release(devs);

	return config_found_ia(dev, "i2cbus", &iba, print);
}
