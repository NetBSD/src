/*	$NetBSD: pci.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_DEVICE_PCI_H__
#define __NVKM_DEVICE_PCI_H__
#include <core/device.h>

struct nvkm_device_pci {
	struct nvkm_device device;
	struct pci_dev *pdev;
	bool suspend;
};

int nvkm_device_pci_new(struct pci_dev *, const char *cfg, const char *dbg,
			bool detect, bool mmio, u64 subdev_mask,
			struct nvkm_device **);
#endif
