/* $NetBSD: fdt_intr.c,v 1.9 2017/06/02 00:55:26 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_intr.c,v 1.9 2017/06/02 00:55:26 jmcneill Exp $");

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
static u_int *get_specifier_by_index(int phandle, int pindex, u_int *spec);
static u_int *get_specifier_from_map(int phandle, u_int *spec, u_int spec_len, int *piphandle);

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
	struct fdtbus_interrupt_controller *ic;
	int ihandle = phandle;
	u_int *specifier;
	u_int spec_length;

	specifier = get_specifier_by_index(phandle, index, &spec_length);
	if (has_interrupt_map(phandle))
		specifier = get_specifier_from_map(phandle, specifier,
		    spec_length, &ihandle);

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
	int ihandle = phandle;
	u_int *specifier;
	u_int spec_length;

	specifier = get_specifier_by_index(phandle, index, &spec_length);
	if (has_interrupt_map(phandle))
		specifier = get_specifier_from_map(phandle, specifier,
		    spec_length, &ihandle);

	ic = fdtbus_get_interrupt_controller(ihandle);
	if (ic == NULL)
		return false;

	return ic->ic_funcs->intrstr(ic->ic_dev, specifier, buf, buflen);
}

static int
find_interrupt_map(int phandle)
{
	while (phandle > 0) {
		if (of_hasprop(phandle, "interrupt-map"))
			return phandle;
		phandle = OF_parent(phandle);
	}
	return -1;
}

static int
find_address_cells(int phandle)
{
	uint32_t cells;

	if (of_getprop_uint32(phandle, "#address-cells", &cells) != 0)
		cells = 2;

	return cells;
}

static int
find_interrupt_cells(int phandle)
{
	uint32_t cells;

	while (phandle > 0) {
		if (of_getprop_uint32(phandle, "#interrupt-cells", &cells) == 0)
			return cells;
		phandle = OF_parent(phandle);
	}
	return 0;
}

static bool
has_interrupt_map(int phandle)
{
	return find_interrupt_map(OF_parent(phandle)) != -1;
}

static u_int *
get_specifier_from_map(int phandle, u_int *specifier, u_int spec_length, int *piphandle)
{
	u_int *result = NULL;

	const int nexus_phandle = find_interrupt_map(phandle);

	int len = OF_getproplen(nexus_phandle, "interrupt-map");
	if (len <= 0) {
		printf("%s: no interrupt-map.\n", __func__);
		return NULL;
	}
	int resid = len;
	char *data = kmem_alloc(len, KM_SLEEP);
	len = OF_getprop(nexus_phandle, "interrupt-map", data, len);
	if (len <= 0) {
		printf("%s: can't get property interrupt-map.\n", __func__);
		goto done;
	}

	/* child unit address: #address-cells prop of child bus node */
	const int cua_cells = find_address_cells(nexus_phandle);
	/* child interrupt specifier: #interrupt-cells of the nexus node */
	const int cis_cells = find_interrupt_cells(nexus_phandle);

	/* Offset (in cells) from map entry to child unit address specifier */
	const u_int cua_off = 0;
	/* Offset (in cells) from map entry to child interrupt specifier */
	const u_int cis_off = cua_off + cua_cells;
	/* Offset (in cells) from map entry to interrupt parent phandle */
	const u_int ip_off = cis_off + cis_cells;
	/* Offset (in cells) from map entry to parent unit specifier */
	const u_int pus_off = ip_off + 1;

#ifdef FDT_INTR_DEBUG
	printf("cua_cells: %d, cis_cells: %d, ip_off = %d\n", cua_cells, cis_cells, ip_off);
	printf("searching for interrupt in map:");
	for (int i = 0; i < spec_length; i++)
		printf(" %08x", specifier[i]);
	printf("\n");
#endif

	u_int *p = (u_int *)data;
	while (resid > 0) {
		/* Interrupt parent phandle */
		const u_int iparent = fdtbus_get_phandle_from_native(be32toh(p[ip_off]));

		/* parent unit specifier: #address-cells of the interrupt parent */
		const u_int pus_cells = find_address_cells(iparent);
		/* parent interrupt specifier: #interrupt-cells of the interrupt parent */
		const u_int pis_cells = find_interrupt_cells(iparent);

		/* Offset (in cells) from map entry to parent interrupt specifier */
		const u_int pis_off = pus_off + pus_cells;

		if (cis_cells == spec_length && memcmp(&p[cis_off], specifier, spec_length * 4) == 0) {
			const int slen = pus_cells + pis_cells;
#ifdef FDT_INTR_DEBUG
			printf(" intr map match iparent %08x slen %d:", iparent, slen);
			for (int i = 0; i < slen; i++)
				printf(" %08x", p[pus_off + i]);
			printf("\n");
#endif
			result = kmem_alloc(slen, KM_SLEEP);
			memcpy(result, &p[pus_off], slen * 4);
			*piphandle = iparent;
			goto done;
		}
		/* Determine the length of the entry and skip that many
		 * 32 bit words
		 */
		const u_int reclen = pis_off + pis_cells;
		resid -= reclen * sizeof(u_int);
		p += reclen;
	}

done:
	kmem_free(data, len);
	return result;
}

static u_int *
get_specifier_by_index(int phandle, int pindex, u_int *spec_length)
{
	u_int *specifiers;
	u_int *specifier;
	int interrupt_parent, interrupt_cells, len;

	interrupt_parent = fdtbus_get_interrupt_parent(phandle);
	if (interrupt_parent <= 0)
		return NULL;

	interrupt_cells = find_interrupt_cells(interrupt_parent);
	if (interrupt_cells <= 0)
		return NULL;

	len = OF_getproplen(phandle, "interrupts");
	if (len <= 0)
		return NULL;

	const u_int nintr = len / interrupt_cells;

	if (pindex >= nintr)
		return NULL;

	specifiers = kmem_alloc(len, KM_SLEEP);

	if (OF_getprop(phandle, "interrupts", specifiers, len) != len) {
		kmem_free(specifiers, len);
		return NULL;
	}
	specifier = kmem_alloc(interrupt_cells * sizeof(u_int), KM_SLEEP);
	*spec_length = interrupt_cells;
	for (int i = 0; i < interrupt_cells; i++)
		specifier[i] = specifiers[pindex * interrupt_cells + i];
	kmem_free(specifiers, len);
	return specifier;
}
