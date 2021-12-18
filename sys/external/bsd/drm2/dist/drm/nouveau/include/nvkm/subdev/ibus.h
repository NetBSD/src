/*	$NetBSD: ibus.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_IBUS_H__
#define __NVKM_IBUS_H__
#include <core/subdev.h>

int gf100_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gf117_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gk104_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gk20a_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gm200_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gp10b_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
#endif
