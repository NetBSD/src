/* $NetBSD: fdt_subr.c,v 1.2 2015/12/16 12:03:44 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_subr.c,v 1.2 2015/12/16 12:03:44 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/ofw/openfirm.h>
#include <dev/fdt/fdt_openfirm.h>
#include <dev/fdt/fdtvar.h>

static int
fdtbus_get_addr_cells(int phandle)
{
	int val, addr_cells, error;

	const int parent = OF_parent(phandle);
	if (parent == -1)
		return -1;

	error = OF_getprop(parent, "#address-cells", &val, sizeof(val));
	if (error <= 0) {
		addr_cells = 2;
	} else {
		addr_cells = fdt32_to_cpu(val);
	}

	return addr_cells;
}

static int
fdtbus_get_size_cells(int phandle)
{
	int val, size_cells, error;

	const int parent = OF_parent(phandle);
	if (parent == -1)
		return -1;

	error = OF_getprop(parent, "#size-cells", &val, sizeof(val));
	if (error <= 0) {
		size_cells = 0;
	} else {
		size_cells = fdt32_to_cpu(val);
	}

	return size_cells;
}

int
fdtbus_get_phandle(int phandle, const char *prop)
{
	u_int phandle_ref;
	u_int *buf;
	int len;

	len = OF_getproplen(phandle, prop);
	if (len < sizeof(phandle_ref))
		return -1;

	buf = kmem_alloc(len, KM_SLEEP);

	if (OF_getprop(phandle, prop, buf, len) != len) {
		kmem_free(buf, len);
		return -1;
	}

	phandle_ref = fdt32_to_cpu(buf[0]);
	kmem_free(buf, len);

	const void *data = fdt_openfirm_get_data();
	const int off = fdt_node_offset_by_phandle(data, phandle_ref);
	if (off < 0) {
		return -1;
	}

	return fdt_openfirm_get_phandle(off);
}

int
fdtbus_get_reg(int phandle, u_int index, bus_addr_t *paddr, bus_size_t *psize)
{
	bus_addr_t addr;
	bus_size_t size;
	uint8_t *buf;
	int len;

	const int addr_cells = fdtbus_get_addr_cells(phandle);
	const int size_cells = fdtbus_get_size_cells(phandle);
	if (addr_cells == -1 || size_cells == -1)
		return -1;

	const u_int reglen = size_cells * 4 + addr_cells * 4;
	if (reglen == 0)
		return -1;

	len = OF_getproplen(phandle, "reg");
	if (len <= 0)
		return -1;

	const u_int nregs = len / reglen;

	if (index >= nregs)
		return -1;

	buf = kmem_alloc(len, KM_SLEEP);
	if (buf == NULL)
		return -1;

	len = OF_getprop(phandle, "reg", buf, len);

	switch (addr_cells) {
	case 0:
		addr = 0;
		break;
	case 1:
		addr = be32dec(&buf[index * reglen + 0]);
		break;
	case 2:
		addr = be64dec(&buf[index * reglen + 0]);
		break;
	default:
		panic("fdtbus_get_reg: unsupported addr_cells %d", addr_cells);
	}

	switch (size_cells) {
	case 0:
		size = 0;
		break;
	case 1:
		size = be32dec(&buf[index * reglen + addr_cells * 4]);
		break;
	case 2:
		size = be64dec(&buf[index * reglen + addr_cells * 4]);
		break;
	default:
		panic("fdtbus_get_reg: unsupported size_cells %d", size_cells);
	}

	if (paddr)
		*paddr = addr;
	if (psize)
		*psize = size;

	kmem_free(buf, len);

	return 0;
}
