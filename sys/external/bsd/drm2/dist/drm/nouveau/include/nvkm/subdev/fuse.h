/*	$NetBSD: fuse.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_FUSE_H__
#define __NVKM_FUSE_H__
#include <core/subdev.h>

struct nvkm_fuse {
	const struct nvkm_fuse_func *func;
	struct nvkm_subdev subdev;
	spinlock_t lock;
};

u32 nvkm_fuse_read(struct nvkm_fuse *, u32 addr);

int nv50_fuse_new(struct nvkm_device *, int, struct nvkm_fuse **);
int gf100_fuse_new(struct nvkm_device *, int, struct nvkm_fuse **);
int gm107_fuse_new(struct nvkm_device *, int, struct nvkm_fuse **);
#endif
