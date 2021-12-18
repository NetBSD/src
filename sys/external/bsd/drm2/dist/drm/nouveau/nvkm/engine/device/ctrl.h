/*	$NetBSD: ctrl.h,v 1.3 2021/12/18 23:45:34 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_DEVICE_CTRL_H__
#define __NVKM_DEVICE_CTRL_H__
#define nvkm_control(p) container_of((p), struct nvkm_control, object)
#include <core/object.h>

struct nvkm_control {
	struct nvkm_object object;
	struct nvkm_device *device;
};

extern const struct nvkm_device_oclass nvkm_control_oclass;
#endif
