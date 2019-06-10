/*	$NetBSD: acpi.h,v 1.2.6.2 2019/06/10 22:08:16 christos Exp $	*/

#ifndef __NVKM_DEVICE_ACPI_H__
#define __NVKM_DEVICE_ACPI_H__
#include <core/os.h>
struct nvkm_device;

void nvkm_acpi_init(struct nvkm_device *);
void nvkm_acpi_fini(struct nvkm_device *);
#endif
