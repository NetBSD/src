/*	$NetBSD: nv50.h,v 1.4 2021/12/18 23:45:39 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_FB_NV50_H__
#define __NVKM_FB_NV50_H__
#define nv50_fb(p) container_of((p), struct nv50_fb, base)
#include "priv.h"

struct nv50_fb {
	const struct nv50_fb_func *func;
	struct nvkm_fb base;
#ifdef __NetBSD__
	bus_dma_segment_t r100c08_seg;
	bus_dmamap_t r100c08_page;
	void *r100c08_kva;
#else
	struct page *r100c08_page;
#endif
	dma_addr_t r100c08;
};

struct nv50_fb_func {
	int (*ram_new)(struct nvkm_fb *, struct nvkm_ram **);
	u32 (*tags)(struct nvkm_fb *);
	u32 trap;
};

int nv50_fb_new_(const struct nv50_fb_func *, struct nvkm_device *, int index,
		 struct nvkm_fb **pfb);
#endif
