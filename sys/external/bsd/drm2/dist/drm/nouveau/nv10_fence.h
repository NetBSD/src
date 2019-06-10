/*	$NetBSD: nv10_fence.h,v 1.1.1.1.32.1 2019/06/10 22:08:07 christos Exp $	*/

#ifndef __NV10_FENCE_H_
#define __NV10_FENCE_H_

#include "nouveau_fence.h"
#include "nouveau_bo.h"

struct nv10_fence_chan {
	struct nouveau_fence_chan base;
	struct nvif_object sema;
	struct nvif_object head[4];
};

struct nv10_fence_priv {
	struct nouveau_fence_priv base;
	struct nouveau_bo *bo;
	spinlock_t lock;
	u32 sequence;
};

#endif
