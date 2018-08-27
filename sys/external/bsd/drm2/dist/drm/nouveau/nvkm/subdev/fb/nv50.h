/*	$NetBSD: nv50.h,v 1.2 2018/08/27 04:58:33 riastradh Exp $	*/

#ifndef __NVKM_FB_NV50_H__
#define __NVKM_FB_NV50_H__
#define nv50_fb(p) container_of((p), struct nv50_fb, base)
#include "priv.h"

struct nv50_fb {
	const struct nv50_fb_func *func;
	struct nvkm_fb base;
#ifdef __NetBSD__
	bus_dma_segment_t r100c08_seg;
	bus_dmamap_t r100c08_map;
	void *r100c08_kva;
#else
	struct page *r100c08_page;
	dma_addr_t r100c08;
#endif
};

struct nv50_fb_func {
	int (*ram_new)(struct nvkm_fb *, struct nvkm_ram **);
	u32 trap;
};

int nv50_fb_new_(const struct nv50_fb_func *, struct nvkm_device *, int index,
		 struct nvkm_fb **pfb);
extern int nv50_fb_memtype[0x80];
#endif
