/*	$NetBSD: nv50.h,v 1.2 2018/08/27 04:58:32 riastradh Exp $	*/

#ifndef __NVKM_SW_NV50_H__
#define __NVKM_SW_NV50_H__
#define nv50_sw_chan(p) container_of((p), struct nv50_sw_chan, base)
#include "priv.h"
#include "chan.h"
#include "nvsw.h"
#include <core/notify.h>

struct nv50_sw_chan {
	struct nvkm_sw_chan base;
	struct {
		struct nvkm_notify notify[4];
		u32 ctxdma;
		u64 offset;
		u32 value;
	} vblank;
};

void *nv50_sw_chan_dtor(struct nvkm_sw_chan *);
#endif
