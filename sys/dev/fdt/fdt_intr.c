/* $NetBSD: fdt_intr.c,v 1.5 2016/01/05 21:57:18 marty Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_intr.c,v 1.5 2016/01/05 21:57:18 marty Exp $");

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

static bool has_interrupt_map(int phandle);
static u_int *get_entry_from_map(int phandle, int pindex);
static u_int *get_specifier_by_index(int phandle, int pindex);

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
fdtbus_get_interrupt_controller_ic(int phandle)
{
	struct fdtbus_interrupt_controller * ic;
	for (ic = fdtbus_ic; ic; ic = ic->ic_next) {
		if (ic->ic_phandle == phandle) {
			return ic;
		}
	}
	return NULL;
}

static struct fdtbus_interrupt_controller *
fdtbus_get_interrupt_controller(int phandle)
{
	const int ic_phandle = fdtbus_get_interrupt_parent(phandle);
	if (ic_phandle < 0) {
		return NULL;
	}

	return fdtbus_get_interrupt_controller_ic(ic_phandle);
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
	u_int *specifier;
	int ihandle;
	struct fdtbus_interrupt_controller *ic;

	if (has_interrupt_map(phandle)) {
		specifier = get_entry_from_map(phandle, index);
		ihandle = be32toh(specifier[1]);
		ihandle = fdtbus_get_phandle_from_native(ihandle);
		specifier += 2;
	} else {
		specifier = get_specifier_by_index(phandle, index);
		ihandle = phandle;
	}
	ic = fdtbus_get_interrupt_controller(ihandle);
	if (ic == NULL)
		return NULL;
	return ic->ic_funcs->establish(ic->ic_dev, specifier,
					       ipl, flags, func, arg);
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
	int ihandle;
	u_int *specifier;

	if (has_interrupt_map(phandle)) {
		specifier = get_entry_from_map(phandle, index);
		ihandle = be32toh(specifier[1]);
		ihandle = fdtbus_get_phandle_from_native(ihandle);
	} else {
		ihandle = phandle;
		specifier = get_specifier_by_index(phandle, index);
	}
	ic = fdtbus_get_interrupt_controller(ihandle);
	if (ic == NULL)
		return false;
	return ic->ic_funcs->intrstr(ic->ic_dev, specifier, buf, buflen);
}

/*
 * Devices that have multiple interrupts, connected to two or more
 * interrupt sources use an interrupt map rather than a simple
 * interrupt parent to indicate which interrupt controller goes with
 * which map.  The interrupt map is contained in the node describing
 * the first level parent and contains one entry per interrupt:
 *   index -- the index of the entry in the map
 *   &parent -- pointer to the node containing the actual interrupt parent
 *              for the specific interrupt
 *  [specifier 0 - specifier N-1] The N (usually 2 or 3) 32 bit words
 *              that make up the specifier.
 *
 * returns true if the device phandle has an interrupt-parent that
 * contains an interrupt-map.
 */
static bool
has_interrupt_map(int phandle)
{
	int ic_phandle;
	of_getprop_uint32(phandle, "interrupt-parent", &ic_phandle);
	if (ic_phandle <= 0)
		return false;
	ic_phandle = fdtbus_get_phandle_from_native(ic_phandle);
	if (ic_phandle <= 0)
		return false;
	int len = OF_getproplen(ic_phandle, "interrupt-map");
	if (len > 0)
		return true;
	return false;
}

/*
 * Walk the specifier map and return a pointer to the map entry
 * associated with pindex.  Return null if there is no entry.
 *
 * Because the length of the specifier depends on the interrupt
 * controller, we need to repeatedly obtain interrupt-celss for
 * the controller for the current index.
 *
 * this version leaks memory and needs a revisit
 */
static u_int *
get_entry_from_map(int phandle, int pindex)
{
	int intr_cells;
	int intr_parent;

	of_getprop_uint32(phandle, "#interrupt-cells", &intr_cells);
	of_getprop_uint32(phandle, "interrupt-parent", &intr_parent);

	intr_parent = fdtbus_get_phandle_from_native(intr_parent);
	int len = OF_getproplen(intr_parent, "interrupt-map");
	if (len <= 0) {
		printf(" no interrupt-map.\n");
		return NULL;
	}
	uint resid = len;
	char *data = kmem_alloc(len, KM_SLEEP);
	len = OF_getprop(intr_parent, "interrupt-map", data, len);
	if (len <= 0) {
		printf(" can't get property interrupt-map.\n");
		return NULL;
	}
	u_int *p = (u_int *)data;

	while (resid > 0) {
		u_int index = be32toh(p[0]);
		if (index == pindex) {
			return &p[0];
									
		}
		/* Determine the length of the entry and skip that many
		 * 32 bit words
		 */
		const u_int parent = fdtbus_get_phandle_from_native(be32toh(p[intr_cells]));
		u_int pintr_cells;
		of_getprop_uint32(parent, "#interrupt-cells", &pintr_cells);
		const u_int reclen = (intr_cells + pintr_cells + 1);
		resid -= reclen * sizeof(u_int);
		p += reclen;
	}
	return NULL;
}


/*
 * Devices that don't connect to more than one interrupt source use
 * an array of specifiers.  Find the specifier that matches pindex
 * and return a pointer to it.
 *
 * This version leaks memory and needs a revisit.
 */
static u_int *get_specifier_by_index(int phandle, int pindex)
{
	u_int *specifiers;
	int interrupt_parent, interrupt_cells, len;

	len = OF_getprop(phandle, "interrupt-parent", &interrupt_parent,
	    sizeof(interrupt_parent));
	interrupt_parent = be32toh(interrupt_parent);
	interrupt_parent = fdtbus_get_phandle_from_native(interrupt_parent);
	if (len != sizeof(interrupt_parent) || interrupt_parent <= 0) {
		printf("%s: interrupt_parent sanity check failed\n", __func__);
		return NULL;
	}

	len = OF_getprop(interrupt_parent, "#interrupt-cells",
			 &interrupt_cells,  sizeof(interrupt_cells));
	interrupt_cells = be32toh(interrupt_cells);
	if (len != sizeof(interrupt_cells) || interrupt_cells <= 0) {
		printf("%s: interrupt_celyls sanity check failed\n", __func__);
		return NULL;
	}

	len = OF_getproplen(phandle, "interrupts");
	if (len <= 0) {
		printf("%s: Couldn't get property interrupts\n", __func__);
		return NULL;
	}

	const u_int clen = interrupt_cells * sizeof(u_int);
	const u_int nintr = len / interrupt_cells;

	if (pindex >= nintr)
		return NULL;

	specifiers = kmem_alloc(len, KM_SLEEP);

	if (OF_getprop(phandle, "interrupts", specifiers, len) != len) {
		kmem_free(specifiers, len);
		return NULL;
	}
	return &specifiers[pindex * clen];
}
