/* $NetBSD: fdt_intr.c,v 1.3 2015/12/16 19:33:55 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_intr.c,v 1.3 2015/12/16 19:33:55 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_interrupt_controller {
	device_t ic_dev;
	int ic_phandle;
	const struct fdtbus_interrupt_controller_func *ic_funcs;

	struct fdtbus_interrupt_controller *ic_next;
};

static struct fdtbus_interrupt_controller *fdtbus_ic = NULL;

static int
fdtbus_get_interrupt_parent(int phandle)
{
	u_int interrupt_parent;

	while (phandle >= 0) {
		if (of_getprop_uint32(phandle, "interrupt-parent",
		    &interrupt_parent) == 0) {
			break;
		}
		if (phandle == 0) {
			return -1;
		}
		phandle = OF_parent(phandle);
	}
	if (phandle < 0) {
		return -1;
	}

	const void *data = fdtbus_get_data();
	const int off = fdt_node_offset_by_phandle(data, interrupt_parent);
	if (off < 0) {
		return -1;
	}

	return fdtbus_offset2phandle(off);
}

static struct fdtbus_interrupt_controller *
fdtbus_get_interrupt_controller(int phandle)
{
	struct fdtbus_interrupt_controller *ic;

	const int ic_phandle = fdtbus_get_interrupt_parent(phandle);
	if (ic_phandle < 0) {
		return NULL;
	}

	for (ic = fdtbus_ic; ic; ic = ic->ic_next) {
		if (ic->ic_phandle == ic_phandle) {
			return ic;
		}
	}

	return NULL;
}

int
fdtbus_register_interrupt_controller(device_t dev, int phandle,
    const struct fdtbus_interrupt_controller_func *funcs)
{
	struct fdtbus_interrupt_controller *ic;

	ic = kmem_alloc(sizeof(*ic), KM_SLEEP);
	ic->ic_dev = dev;
	ic->ic_phandle = phandle;
	ic->ic_funcs = funcs;

	ic->ic_next = fdtbus_ic;
	fdtbus_ic = ic;

	return 0;
}

void *
fdtbus_intr_establish(int phandle, u_int index, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	struct fdtbus_interrupt_controller *ic;

	ic = fdtbus_get_interrupt_controller(phandle);
	if (ic == NULL)
		return NULL;

	return ic->ic_funcs->establish(ic->ic_dev, phandle, index, ipl,
	    flags, func, arg);
}

void
fdtbus_intr_disestablish(int phandle, void *ih)
{
	struct fdtbus_interrupt_controller *ic;

	ic = fdtbus_get_interrupt_controller(phandle);
	KASSERT(ic != NULL);

	return ic->ic_funcs->disestablish(ic->ic_dev, ih);
}

bool
fdtbus_intr_str(int phandle, u_int index, char *buf, size_t buflen)
{
	struct fdtbus_interrupt_controller *ic;

	ic = fdtbus_get_interrupt_controller(phandle);
	if (ic == NULL)
		return false;

	return ic->ic_funcs->intrstr(ic->ic_dev, phandle, index, buf, buflen);
}
