/*	$NetBSD: mc.h,v 1.2 2018/08/27 04:58:30 riastradh Exp $	*/

#ifndef __NVKM_MC_H__
#define __NVKM_MC_H__
#include <core/subdev.h>

struct nvkm_mc {
	const struct nvkm_mc_func *func;
	struct nvkm_subdev subdev;
};

void nvkm_mc_intr(struct nvkm_mc *, bool *handled);
void nvkm_mc_intr_unarm(struct nvkm_mc *);
void nvkm_mc_intr_rearm(struct nvkm_mc *);
void nvkm_mc_unk260(struct nvkm_mc *, u32 data);

int nv04_mc_new(struct nvkm_device *, int, struct nvkm_mc **);
int nv44_mc_new(struct nvkm_device *, int, struct nvkm_mc **);
int nv50_mc_new(struct nvkm_device *, int, struct nvkm_mc **);
int g98_mc_new(struct nvkm_device *, int, struct nvkm_mc **);
int gf100_mc_new(struct nvkm_device *, int, struct nvkm_mc **);
int gk20a_mc_new(struct nvkm_device *, int, struct nvkm_mc **);
#endif
