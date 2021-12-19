/*	$NetBSD: umem.h,v 1.3 2021/12/19 10:51:58 riastradh Exp $	*/

#ifndef __NVKM_UMEM_H__
#define __NVKM_UMEM_H__
#define nvkm_umem(p) container_of((p), struct nvkm_umem, object)
#include <core/object.h>
#include "mem.h"

struct nvkm_umem {
	struct nvkm_object object;
	struct nvkm_mmu *mmu;
	u8 type:8;
	bool priv:1;
	bool mappable:1;
	bool io:1;

	struct nvkm_memory *memory;
	struct list_head head;

	union {
		struct nvkm_vma *bar;
		void *map;
	};
#ifdef __NetBSD__
	bus_dma_tag_t dmat;
	bus_size_t size;
#endif
};

int nvkm_umem_new(const struct nvkm_oclass *, void *argv, u32 argc,
		  struct nvkm_object **);
#endif
