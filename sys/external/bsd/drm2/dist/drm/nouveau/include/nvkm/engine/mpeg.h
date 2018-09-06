/*	$NetBSD: mpeg.h,v 1.2.2.2 2018/09/06 06:56:24 pgoyette Exp $	*/

#ifndef __NVKM_MPEG_H__
#define __NVKM_MPEG_H__
#include <core/engine.h>
int nv31_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int nv40_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int nv44_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int nv50_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
int g84_mpeg_new(struct nvkm_device *, int index, struct nvkm_engine **);
#endif
