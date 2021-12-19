/*	$NetBSD: pci.h,v 1.4 2021/12/19 10:51:56 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_DEVICE_PCI_H__
#define __NVKM_DEVICE_PCI_H__
#include <core/device.h>

struct nvkm_device_pci {
	struct nvkm_device device;
	struct pci_dev *pdev;
#ifdef __NetBSD__
	bus_dma_tag_t bus_dmat;
	bus_dma_tag_t dmat;
#endif
	bool suspend;
};

int nvkm_device_pci_new(struct pci_dev *, const char *cfg, const char *dbg,
			bool detect, bool mmio, u64 subdev_mask,
			struct nvkm_device **);
#endif
