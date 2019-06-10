/*	$NetBSD: ce.h,v 1.2.6.2 2019/06/10 22:08:14 christos Exp $	*/

#ifndef __NVKM_CE_H__
#define __NVKM_CE_H__
#include <engine/falcon.h>

int gt215_ce_new(struct nvkm_device *, int, struct nvkm_engine **);
int gf100_ce_new(struct nvkm_device *, int, struct nvkm_engine **);
int gk104_ce_new(struct nvkm_device *, int, struct nvkm_engine **);
int gm204_ce_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
