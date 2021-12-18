/*	$NetBSD: mpeg.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_MPEG_H__
#define __NVKM_MPEG_H__
#include <core/engine.h>
int nv31_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int nv40_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int nv44_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int nv50_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int g84_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
#endif
