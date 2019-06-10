/*	$NetBSD: ibus.h,v 1.2.6.2 2019/06/10 22:08:14 christos Exp $	*/

#ifndef __NVKM_IBUS_H__
#define __NVKM_IBUS_H__
#include <core/subdev.h>

int gf100_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gf117_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gk104_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
int gk20a_ibus_new(struct nvkm_device *, int, struct nvkm_subdev **);
#endif
