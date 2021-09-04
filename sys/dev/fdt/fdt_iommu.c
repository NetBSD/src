/* $NetBSD: fdt_iommu.c,v 1.1 2021/09/04 12:34:39 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_iommu.c,v 1.1 2021/09/04 12:34:39 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_iommu {
	device_t			iommu_dev;
	int				iommu_phandle;
	const struct fdtbus_iommu_func	*iommu_funcs;
	u_int				iommu_cells;
	LIST_ENTRY(fdtbus_iommu)	iommu_next;
};

static LIST_HEAD(, fdtbus_iommu) fdtbus_iommus =
    LIST_HEAD_INITIALIZER(fdtbus_iommus);

/*
 * fdtbus_get_iommu --
 *
 *	Return the iommu registered with the specified node, or NULL if
 *	not found.
 */
static struct fdtbus_iommu *
fdtbus_get_iommu(int phandle)
{
	struct fdtbus_iommu *iommu;

	LIST_FOREACH(iommu, &fdtbus_iommus, iommu_next) {
		if (iommu->iommu_phandle == phandle) {
			return iommu;
		}
	}

	return NULL;
}

/*
 * fdtbus_register_iommu --
 *
 *	Register an IOMMU on the specified node.
 */
int
fdtbus_register_iommu(device_t dev, int phandle,
    const struct fdtbus_iommu_func *funcs)
{
	struct fdtbus_iommu *iommu;
	u_int cells;

	if (funcs == NULL || funcs->map == NULL) {
		return EINVAL;
	}

	if (of_getprop_uint32(phandle, "#iommu-cells", &cells) != 0) {
		return ENXIO;
	}

	if (fdtbus_get_iommu(phandle) != NULL) {
		return EEXIST;
	}

	iommu = kmem_alloc(sizeof(*iommu), KM_SLEEP);
	iommu->iommu_dev = dev;
	iommu->iommu_phandle = phandle;
	iommu->iommu_funcs = funcs;
	iommu->iommu_cells = cells;

	LIST_INSERT_HEAD(&fdtbus_iommus, iommu, iommu_next);

	return 0;
}

/*
 * fdtbus_iommu_map --
 *
 *	Get a bus dma tag that is translated by any iommus specified by
 *	the device tree. The `index` property refers to the N-th item
 *	in the list of IOMMUs specified in the "iommus" property. If an
 *	IOMMU is not found, the original bus dma tag is returned.
 */
bus_dma_tag_t
fdtbus_iommu_map(int phandle, u_int index, bus_dma_tag_t dmat)
{
	struct fdtbus_iommu *iommu;
	const u_int *p;
	u_int n, cells;
	int len, resid;

	p = fdtbus_get_prop(phandle, "iommus", &len);
	if (p == NULL) {
		return dmat;
	}

	for (n = 0, resid = len; resid > 0; n++) {
		const int iommu_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(iommu_phandle, "#iommu-cells", &cells)) {
			break;
		}
		if (n == index) {
			iommu = fdtbus_get_iommu(iommu_phandle);
			if (iommu == NULL) {
				break;
			}
			return iommu->iommu_funcs->map(iommu->iommu_dev,
			    cells > 0 ? &p[1] : NULL, dmat);
		}
		resid -= (cells + 1) * 4;
		p += cells + 1;
	}

	return dmat;
}

/*
 * fdtbus_iommu_map_pci --
 *
 *	Get a bus dma tag that is translated by any iommus specified by
 *	the device tree for PCI devices. The `rid` param is the requester
 *	ID. Returns a (maybe translated) dma tag.
 */
bus_dma_tag_t
fdtbus_iommu_map_pci(int phandle, uint32_t rid, bus_dma_tag_t dmat)
{
	struct fdtbus_iommu *iommu;
	uint32_t map_mask;
	const u_int *p;
	u_int cells;
	int len;

	len = 0;
	p = fdtbus_get_prop(phandle, "iommu-map", &len);
	KASSERT(p != NULL || len == 0);

	if (of_getprop_uint32(phandle, "iommu-map-mask", &map_mask) == 0) {
		rid &= map_mask;
	}

	while (len >= 4) {
		const u_int rid_base = be32toh(*p++);
		const int iommu_phandle =
		    fdtbus_get_phandle_from_native(be32toh(*p++));
		const u_int iommu_base = be32toh(*p++);
		const u_int length = be32toh(*p++);
		len -= 4;

		if (iommu_phandle <= 0) {
			continue;
		}
		if (of_getprop_uint32(iommu_phandle, "#iommu-cells", &cells)) {
			continue;
		}
		if (cells != 1) {
			/*
			 * The pci-iommu bindings expect iommu references with
			 * exactly one specifier cell.
			 */
			continue;
		}
		iommu = fdtbus_get_iommu(iommu_phandle);
		if (iommu == NULL) {
			continue;
		}

		if (rid >= rid_base && rid < rid_base + length) {
			const uint32_t sid = rid - rid_base + iommu_base;
			return iommu->iommu_funcs->map(iommu->iommu_dev,
			    &sid, dmat);
		}
	}

	return dmat;
}
