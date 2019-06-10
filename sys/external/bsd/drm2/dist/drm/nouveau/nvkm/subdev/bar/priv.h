/*	$NetBSD: priv.h,v 1.2.6.2 2019/06/10 22:08:20 christos Exp $	*/

#ifndef __NVKM_BAR_PRIV_H__
#define __NVKM_BAR_PRIV_H__
#define nvkm_bar(p) container_of((p), struct nvkm_bar, subdev)
#include <subdev/bar.h>

void nvkm_bar_ctor(const struct nvkm_bar_func *, struct nvkm_device *,
		   int, struct nvkm_bar *);

struct nvkm_bar_func {
	void *(*dtor)(struct nvkm_bar *);
	int (*oneinit)(struct nvkm_bar *);
	int (*init)(struct nvkm_bar *);
	struct nvkm_vm *(*kmap)(struct nvkm_bar *);
	int  (*umap)(struct nvkm_bar *, u64 size, int type, struct nvkm_vma *);
	void (*flush)(struct nvkm_bar *);
};

void g84_bar_flush(struct nvkm_bar *);
#endif
