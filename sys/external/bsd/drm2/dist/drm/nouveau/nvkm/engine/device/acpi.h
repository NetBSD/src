/*	$NetBSD: acpi.h,v 1.2.2.2 2018/09/06 06:56:25 pgoyette Exp $	*/

#ifndef __NVKM_DEVICE_ACPI_H__
#define __NVKM_DEVICE_ACPI_H__
#include <core/os.h>
struct nvkm_device;

void nvkm_acpi_init(struct nvkm_device *);
void nvkm_acpi_fini(struct nvkm_device *);
#endif
