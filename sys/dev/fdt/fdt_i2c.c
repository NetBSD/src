/* $NetBSD: fdt_i2c.c,v 1.16 2025/09/23 00:52:14 thorpej Exp $ */

/*
 * Copyright (c) 2021, 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: fdt_i2c.c,v 1.16 2025/09/23 00:52:14 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_calls.h>
#include <dev/i2c/i2c_enum.h>

struct fdtbus_i2c_controller {
	i2c_tag_t i2c_tag;
	int i2c_phandle;

	LIST_ENTRY(fdtbus_i2c_controller) i2c_next;
};

static LIST_HEAD(, fdtbus_i2c_controller) fdtbus_i2c_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_i2c_controllers);

void
fdtbus_register_i2c_controller(device_t dev, i2c_tag_t tag)
{
	int phandle = devhandle_to_of(device_handle(dev));
	struct fdtbus_i2c_controller *i2c;

	i2c = kmem_alloc(sizeof(*i2c), KM_SLEEP);
	i2c->i2c_tag = tag;
	i2c->i2c_phandle = phandle;

	LIST_INSERT_HEAD(&fdtbus_i2c_controllers, i2c, i2c_next);
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
fdtbus_i2c_get_tag(int phandle)
{
	struct fdtbus_i2c_controller *i2c;

	i2c = fdtbus_get_i2c_controller(phandle);
	if (i2c == NULL)
		return NULL;

	return i2c->i2c_tag;
}

i2c_tag_t
fdtbus_i2c_acquire(int phandle, const char *prop)
{
	int i2c_phandle;

	i2c_phandle = fdtbus_get_phandle(phandle, prop);
	if (i2c_phandle == -1)
		return NULL;

	return fdtbus_i2c_get_tag(i2c_phandle);
}

static int
fdtbus_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct i2c_enumerate_devices_args *args = v;
	int i2c_node, node;
	char name[32], compat_buf[32];
	bus_addr_t addr;
	char *clist;
	int clist_size;
	bool cbrv;

	i2c_node = devhandle_to_of(call_handle);

	/*
	 * The Device Tree bindings state that if a controller has a
	 * child node named "i2c-bus", then that is the node beneath
	 * which the child devices are populated.
	 */
	for (node = OF_child(i2c_node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0) {
			continue;
		}
		if (strcmp(name, "i2c-bus") == 0) {
			i2c_node = node;
			break;
		}
	}

	for (node = OF_child(i2c_node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0) {
			continue;
		}
		if (fdtbus_get_reg(node, 0, &addr, NULL) != 0) {
			continue;
		}

		clist_size = OF_getproplen(node, "compatible");
		if (clist_size <= 0) {
			continue;
		}
		clist = kmem_tmpbuf_alloc(clist_size,
		    compat_buf, sizeof(compat_buf), KM_SLEEP);
		if (OF_getprop(node, "compatible", clist, clist_size)
			       < clist_size) {
			kmem_tmpbuf_free(clist, clist_size, compat_buf);
			continue;
		}

		cbrv = i2c_enumerate_device(dev, args, name, clist,
		    clist_size, (i2c_addr_t)addr,
		    devhandle_from_of(call_handle, node));

		kmem_tmpbuf_free(clist, clist_size, compat_buf);

		if (!cbrv) {
			break;
		}
	}

	return 0;
}
OF_DEVICE_CALL_REGISTER(I2C_ENUMERATE_DEVICES_STR,
			fdtbus_i2c_enumerate_devices);
