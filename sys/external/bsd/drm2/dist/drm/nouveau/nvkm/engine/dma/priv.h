/*	$NetBSD: priv.h,v 1.2.6.2 2019/06/10 22:08:17 christos Exp $	*/

#ifndef __NVKM_DMA_PRIV_H__
#define __NVKM_DMA_PRIV_H__
#define nvkm_dma(p) container_of((p), struct nvkm_dma, engine)
#include <engine/dma.h>

struct nvkm_dmaobj_func {
	int (*bind)(struct nvkm_dmaobj *, struct nvkm_gpuobj *, int align,
		    struct nvkm_gpuobj **);
};

int nvkm_dma_new_(const struct nvkm_dma_func *, struct nvkm_device *,
		  int index, struct nvkm_dma **);

struct nvkm_dma_func {
	int (*class_new)(struct nvkm_dma *, const struct nvkm_oclass *,
			 void *data, u32 size, struct nvkm_dmaobj **);
};
#endif
