/*	$NetBSD: gpuobj.h,v 1.3 2018/08/27 07:36:18 riastradh Exp $	*/

#ifndef __NVKM_GPUOBJ_H__
#define __NVKM_GPUOBJ_H__
#include <core/object.h>
#include <core/memory.h>
#include <core/mm.h>
struct nvkm_vma;
struct nvkm_vm;

#define NVOBJ_FLAG_ZERO_ALLOC 0x00000001
#define NVOBJ_FLAG_HEAP       0x00000004

#ifdef __NetBSD__
#  define	__nvkm_gpuobj_iomem
#  define	__iomem			__nvkm_gpuobj_iomem
#endif

struct nvkm_gpuobj {
	struct nvkm_object object;
	const struct nvkm_gpuobj_func *func;
	struct nvkm_gpuobj *parent;
	struct nvkm_memory *memory;
	struct nvkm_mm_node *node;

	u64 addr;
	u32 size;
	struct nvkm_mm heap;

	void __iomem *map;
};

#ifdef __NetBSD__
#  undef	__iomem
#endif

struct nvkm_gpuobj_func {
	void *(*acquire)(struct nvkm_gpuobj *);
	void (*release)(struct nvkm_gpuobj *);
	u32 (*rd32)(struct nvkm_gpuobj *, u32 offset);
	void (*wr32)(struct nvkm_gpuobj *, u32 offset, u32 data);
};

int nvkm_gpuobj_new(struct nvkm_device *, u32 size, int align, bool zero,
		    struct nvkm_gpuobj *parent, struct nvkm_gpuobj **);
void nvkm_gpuobj_del(struct nvkm_gpuobj **);
int nvkm_gpuobj_wrap(struct nvkm_memory *, struct nvkm_gpuobj **);
int nvkm_gpuobj_map(struct nvkm_gpuobj *, struct nvkm_vm *, u32 access,
		    struct nvkm_vma *);
void nvkm_gpuobj_unmap(struct nvkm_vma *);
#endif
