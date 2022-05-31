/*	$NetBSD: nouveau_nvkm_subdev_mmu_mem.c,v 1.8 2022/05/31 20:53:35 mrg Exp $	*/

/*
 * Copyright 2017 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvkm_subdev_mmu_mem.c,v 1.8 2022/05/31 20:53:35 mrg Exp $");

#define nvkm_mem(p) container_of((p), struct nvkm_mem, memory)
#include "mem.h"

#include <core/memory.h>

#include <nvif/if000a.h>
#include <nvif/unpack.h>

#include <linux/nbsd-namespace.h>

struct nvkm_mem {
	struct nvkm_memory memory;
	enum nvkm_memory_target target;
	struct nvkm_mmu *mmu;
	u64 pages;
#ifdef __NetBSD__
	bus_dma_segment_t *mem;
	int nseg;
	bus_dmamap_t dmamap;
	bus_addr_t *dma;
#else
	struct page **mem;
	union {
		struct scatterlist *sgl;
		dma_addr_t *dma;
	};
#endif
};

static enum nvkm_memory_target
nvkm_mem_target(struct nvkm_memory *memory)
{
	return nvkm_mem(memory)->target;
}

static u8
nvkm_mem_page(struct nvkm_memory *memory)
{
	return PAGE_SHIFT;
}

static u64
nvkm_mem_addr(struct nvkm_memory *memory)
{
	struct nvkm_mem *mem = nvkm_mem(memory);
	if (mem->pages == 1 && mem->mem)
		return mem->dma[0];
	return ~0ULL;
}

static u64
nvkm_mem_size(struct nvkm_memory *memory)
{
	return nvkm_mem(memory)->pages << PAGE_SHIFT;
}

static int
nvkm_mem_map_dma(struct nvkm_memory *memory, u64 offset, struct nvkm_vmm *vmm,
		 struct nvkm_vma *vma, void *argv, u32 argc)
{
	struct nvkm_mem *mem = nvkm_mem(memory);
	struct nvkm_vmm_map map = {
		.memory = &mem->memory,
		.offset = offset,
		.dma = mem->dma,
	};
	return nvkm_vmm_map(vmm, vma, argv, argc, &map);
}

static void *
nvkm_mem_dtor(struct nvkm_memory *memory)
{
	struct nvkm_mem *mem = nvkm_mem(memory);
#ifdef __NetBSD__
	if (mem->dma) {
		kmem_free(mem->dma, mem->nseg * sizeof(mem->dma[0]));
	}
	if (mem->mem) {
		struct nvkm_device *device = mem->mmu->subdev.device;
		bus_dma_tag_t dmat = device->func->dma_tag(device);

		bus_dmamap_unload(dmat, mem->dmamap);
		bus_dmamem_free(dmat, mem->mem, mem->nseg);
		bus_dmamap_destroy(dmat, mem->dmamap);
		kmem_free(mem->mem, mem->pages * sizeof(mem->mem[0]));
	}
#else
	if (mem->mem) {
		while (mem->pages--) {
			dma_unmap_page(mem->mmu->subdev.device->dev,
				       mem->dma[mem->pages], PAGE_SIZE,
				       DMA_BIDIRECTIONAL);
			__free_page(mem->mem[mem->pages]);
		}
		kvfree(mem->dma);
		kvfree(mem->mem);
	}
#endif
	return mem;
}

static const struct nvkm_memory_func
nvkm_mem_dma = {
	.dtor = nvkm_mem_dtor,
	.target = nvkm_mem_target,
	.page = nvkm_mem_page,
	.addr = nvkm_mem_addr,
	.size = nvkm_mem_size,
	.map = nvkm_mem_map_dma,
};

#ifndef __NetBSD__
static int
nvkm_mem_map_sgl(struct nvkm_memory *memory, u64 offset, struct nvkm_vmm *vmm,
		 struct nvkm_vma *vma, void *argv, u32 argc)
{
	struct nvkm_mem *mem = nvkm_mem(memory);
	struct nvkm_vmm_map map = {
		.memory = &mem->memory,
		.offset = offset,
		.sgl = mem->sgl,
	};
	return nvkm_vmm_map(vmm, vma, argv, argc, &map);
}

static const struct nvkm_memory_func
nvkm_mem_sgl = {
	.dtor = nvkm_mem_dtor,
	.target = nvkm_mem_target,
	.page = nvkm_mem_page,
	.addr = nvkm_mem_addr,
	.size = nvkm_mem_size,
	.map = nvkm_mem_map_sgl,
};
#endif

int
#ifdef __NetBSD__
nvkm_mem_map_host(struct nvkm_memory *memory, bus_dma_tag_t *tagp, void **pmap,
    bus_size_t *sizep)
#else
nvkm_mem_map_host(struct nvkm_memory *memory, void **pmap)
#endif
{
	struct nvkm_mem *mem = nvkm_mem(memory);
	if (mem->mem) {
#ifdef __NetBSD__
		struct nvkm_device *device = mem->mmu->subdev.device;
		bus_dma_tag_t dmat = device->func->dma_tag(device);
		/* XXX errno NetBSD->Linux */
		int ret = -bus_dmamem_map(dmat, mem->mem, mem->nseg,
		    mem->pages << PAGE_SHIFT, pmap, BUS_DMA_WAITOK);
		if (ret) {
			*pmap = NULL;
			return ret;
		}
		*tagp = dmat;
		*sizep = mem->pages << PAGE_SHIFT;
		return 0;
#else
		*pmap = vmap(mem->mem, mem->pages, VM_MAP, PAGE_KERNEL);
#endif
		return *pmap ? 0 : -EFAULT;
	}
	return -EINVAL;
}

static int
nvkm_mem_new_host(struct nvkm_mmu *mmu, int type, u8 page, u64 size,
		  void *argv, u32 argc, struct nvkm_memory **pmemory)
{
	struct device *dev = mmu->subdev.device->dev;
	union {
		struct nvif_mem_ram_vn vn;
		struct nvif_mem_ram_v0 v0;
	} *args = argv;
	int ret = -ENOSYS;
	enum nvkm_memory_target target;
	struct nvkm_mem *mem;
	gfp_t gfp = GFP_USER | __GFP_ZERO;

	if ( (mmu->type[type].type & NVKM_MEM_COHERENT) &&
	    !(mmu->type[type].type & NVKM_MEM_UNCACHED))
		target = NVKM_MEM_TARGET_HOST;
	else
		target = NVKM_MEM_TARGET_NCOH;

	if (page != PAGE_SHIFT)
		return -EINVAL;

	if (!(mem = kzalloc(sizeof(*mem), GFP_KERNEL)))
		return -ENOMEM;
	mem->target = target;
	mem->mmu = mmu;
	*pmemory = &mem->memory;

	if (!(ret = nvif_unpack(ret, &argv, &argc, args->v0, 0, 0, false))) {
		if (args->v0.dma) {
			nvkm_memory_ctor(&nvkm_mem_dma, &mem->memory);
#ifndef __NetBSD__
			mem->dma = args->v0.dma;
#else
			mem->dmamap = args->v0.dma;
			mem->nseg = mem->dmamap->dm_nsegs;
			mem->dma = kmem_zalloc(mem->dmamap->dm_nsegs *
			    sizeof(mem->dma[0]), KM_SLEEP);
			for (unsigned i = 0; i < mem->dmamap->dm_nsegs; i++) {
				KASSERT(mem->dmamap->dm_segs[i].ds_len <=
				    PAGE_SIZE);
				mem->dma[i] = mem->dmamap->dm_segs[i].ds_addr;
			}
#endif
		} else {
#ifdef __NetBSD__
			return -ENODEV;
#else
			nvkm_memory_ctor(&nvkm_mem_sgl, &mem->memory);
			mem->sgl = args->v0.sgl;
#endif
		}

		if (!IS_ALIGNED(size, PAGE_SIZE))
			return -EINVAL;
		mem->pages = size >> PAGE_SHIFT;
#ifdef __NetBSD__
		KASSERT(mem->pages == mem->nseg);
#endif
		return 0;
	} else
	if ( (ret = nvif_unvers(ret, &argv, &argc, args->vn))) {
		kfree(mem);
		return ret;
	}

	nvkm_memory_ctor(&nvkm_mem_dma, &mem->memory);
	size = ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT;

#ifdef __NetBSD__
	__USE(gfp);
	__USE(dev);
	struct nvkm_device *device = mem->mmu->subdev.device;
	bus_dma_tag_t dmat = device->func->dma_tag(device);
	mem->mem = kmem_zalloc(size * sizeof(mem->mem[0]), KM_SLEEP);
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dmat, size << PAGE_SHIFT, PAGE_SIZE, PAGE_SIZE,
	    mem->mem, size, &mem->nseg, BUS_DMA_WAITOK);
	if (ret) {
fail0:		kmem_free(mem->mem, size * sizeof(mem->mem[0]));
		return ret;
	}
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_create(dmat, size << PAGE_SHIFT, mem->nseg,
	    PAGE_SIZE, PAGE_SIZE, BUS_DMA_WAITOK, &mem->dmamap);
	if (ret) {
fail1:		bus_dmamem_free(dmat, mem->mem, mem->nseg);
		goto fail0;
	}
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load_raw(dmat, mem->dmamap, mem->mem, mem->nseg,
	    size << PAGE_SHIFT, BUS_DMA_WAITOK);
	if (ret) {
fail2: __unused
		bus_dmamap_destroy(dmat, mem->dmamap);
		goto fail1;
	}
	mem->dma = kmem_zalloc(mem->dmamap->dm_nsegs * sizeof(mem->dma[0]),
	    KM_SLEEP);
	for (unsigned i = 0; i < mem->dmamap->dm_nsegs; i++) {
		KASSERT(mem->dmamap->dm_segs[i].ds_len <= PAGE_SIZE);
		mem->dma[i] = mem->dmamap->dm_segs[i].ds_addr;
	}
	mem->pages = size;
	KASSERT(mem->pages == mem->nseg);
#else
	if (!(mem->mem = kvmalloc_array(size, sizeof(*mem->mem), GFP_KERNEL)))
		return -ENOMEM;
	if (!(mem->dma = kvmalloc_array(size, sizeof(*mem->dma), GFP_KERNEL)))
		return -ENOMEM;

	if (mmu->dma_bits > 32)
		gfp |= GFP_HIGHUSER;
	else
		gfp |= GFP_DMA32;

	for (mem->pages = 0; size; size--, mem->pages++) {
		struct page *p = alloc_page(gfp);
		if (!p)
			return -ENOMEM;

		mem->dma[mem->pages] = dma_map_page(mmu->subdev.device->dev,
						    p, 0, PAGE_SIZE,
						    DMA_BIDIRECTIONAL);
		if (dma_mapping_error(dev, mem->dma[mem->pages])) {
			__free_page(p);
			return -ENOMEM;
		}

		mem->mem[mem->pages] = p;
	}
#endif

	return 0;
}

int
nvkm_mem_new_type(struct nvkm_mmu *mmu, int type, u8 page, u64 size,
		  void *argv, u32 argc, struct nvkm_memory **pmemory)
{
	struct nvkm_memory *memory = NULL;
	int ret;

	if (mmu->type[type].type & NVKM_MEM_VRAM) {
		ret = mmu->func->mem.vram(mmu, type, page, size,
					  argv, argc, &memory);
	} else {
		ret = nvkm_mem_new_host(mmu, type, page, size,
					argv, argc, &memory);
	}

	if (ret)
		nvkm_memory_unref(&memory);
	*pmemory = memory;
	return ret;
}
