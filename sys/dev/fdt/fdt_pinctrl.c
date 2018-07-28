/* $NetBSD: fdt_pinctrl.c,v 1.4.6.1 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
 * Copyright (c) 2015 Martin Fouts
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
__KERNEL_RCSID(0, "$NetBSD: fdt_pinctrl.c,v 1.4.6.1 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_pinctrl_controller {
	device_t pc_dev;
	int pc_phandle;
	const struct fdtbus_pinctrl_controller_func *pc_funcs;

	LIST_ENTRY(fdtbus_pinctrl_controller) pc_next;
};

static LIST_HEAD(, fdtbus_pinctrl_controller) fdtbus_pinctrl_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_pinctrl_controllers);

int
fdtbus_register_pinctrl_config(device_t dev, int phandle,
    const struct fdtbus_pinctrl_controller_func *funcs)
{
	struct fdtbus_pinctrl_controller *pc;

	pc = kmem_alloc(sizeof(*pc), KM_SLEEP);
	pc->pc_dev = dev;
	pc->pc_phandle = phandle;
	pc->pc_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_pinctrl_controllers, pc, pc_next);

	return 0;
}

static struct fdtbus_pinctrl_controller *
fdtbus_pinctrl_lookup(int phandle)
{
	struct fdtbus_pinctrl_controller *pc;

	LIST_FOREACH(pc, &fdtbus_pinctrl_controllers, pc_next) {
		if (pc->pc_phandle == phandle)
			return pc;
	}

	return NULL;
}

int
fdtbus_pinctrl_set_config_index(int phandle, u_int index)
{
	struct fdtbus_pinctrl_controller *pc;
	const u_int *pinctrl_data;
	char buf[16];
	u_int xref, pinctrl_cells;
	int len, error;

	snprintf(buf, sizeof(buf), "pinctrl-%u", index);

	pinctrl_data = fdtbus_get_prop(phandle, buf, &len);
	if (pinctrl_data == NULL)
		return ENOENT;

	while (len > 0) {
		xref = fdtbus_get_phandle_from_native(be32toh(pinctrl_data[0]));
		pc = fdtbus_pinctrl_lookup(xref);
		if (pc == NULL)
			return ENXIO;

		if (of_getprop_uint32(OF_parent(xref), "#pinctrl-cells", &pinctrl_cells) != 0)
			pinctrl_cells = 1;

		error = pc->pc_funcs->set_config(pc->pc_dev, pinctrl_data, pinctrl_cells * 4);
		if (error != 0)
			return error;

		pinctrl_data += pinctrl_cells;
		len -= (pinctrl_cells * 4);
	}

	return 0;
}

int
fdtbus_pinctrl_set_config(int phandle, const char *cfgname)
{
	const char *pinctrl_names, *name;
	int len, index;

	if ((len = OF_getproplen(phandle, "pinctrl-names")) < 0)
		return -1;

	pinctrl_names = fdtbus_get_string(phandle, "pinctrl-names");

	for (name = pinctrl_names, index = 0; len > 0;
	     name += strlen(name) + 1, index++) {
		if (strcmp(name, cfgname) == 0)
			return fdtbus_pinctrl_set_config_index(phandle, index);
	}

	/* Not found */
	return -1;
}

static void
fdtbus_pinctrl_configure_node(int phandle)
{
	char buf[256];
	int child, error;

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;

		/* Configure child nodes */
		fdtbus_pinctrl_configure_node(child);

		/*
		 * Set configuration 0 for this node. This may fail if the
		 * pinctrl provider is missing; that's OK, we will re-configure
		 * when that provider attaches.
		 */
		fdtbus_get_path(child, buf, sizeof(buf));
		error = fdtbus_pinctrl_set_config_index(child, 0);
		if (error == 0)
			aprint_debug("pinctrl: set config pinctrl-0 for %s\n", buf);
		else if (error != ENOENT)
			aprint_debug("pinctrl: failed to set config pinctrl-0 for %s: %d\n", buf, error);
	}
}

void
fdtbus_pinctrl_configure(void)
{
	fdtbus_pinctrl_configure_node(OF_finddevice("/"));
}
