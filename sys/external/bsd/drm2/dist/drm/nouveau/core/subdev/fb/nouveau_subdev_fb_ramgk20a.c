/*
 * Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "priv.h"

#include <subdev/fb.h>
#include <linux/mm.h>

struct gk20a_mem {
	struct nouveau_mem base;
	void *cpuaddr;
	dma_addr_t handle;
#if defined(__NetBSD__)
	bus_dma_segment_t dmaseg;
	bus_size_t dmasize;
#endif
};
#define to_gk20a_mem(m) container_of(m, struct gk20a_mem, base)

static void
gk20a_ram_put(struct nouveau_fb *pfb, struct nouveau_mem **pmem)
{
	struct gk20a_mem *mem = to_gk20a_mem(*pmem);

	*pmem = NULL;
	if (unlikely(mem == NULL))
		return;

#if defined(__NetBSD__)
	if (likely(mem->base.pages)) {
		const bus_dma_tag_t dmat = nv_device(pfb)->platformdev->dmat;
		bus_dmamap_unload(dmat, mem->base.pages);
		bus_dmamem_unmap(dmat, mem->cpuaddr, mem->dmasize);
		bus_dmamap_destroy(dmat, mem->base.pages);
		bus_dmamem_free(dmat, &mem->dmaseg, 1);
	}
#else
	struct device *dev = nv_device_base(nv_device(pfb));
	if (likely(mem->cpuaddr))
		dma_free_coherent(dev, mem->base.size << PAGE_SHIFT,
				  mem->cpuaddr, mem->handle);

	kfree(mem->base.pages);
#endif
	kfree(mem);
}

static int
gk20a_ram_get(struct nouveau_fb *pfb, u64 size, u32 align, u32 ncmin,
	     u32 memtype, struct nouveau_mem **pmem)
{
#if !defined(__NetBSD__)
	struct device *dev = nv_device_base(nv_device(pfb));
	int i;
#endif
	struct gk20a_mem *mem;
	u32 type = memtype & 0xff;
	u32 npages, order;

	nv_debug(pfb, "%s: size: %llx align: %x, ncmin: %x\n", __func__,
		 (unsigned long long)size, align, ncmin);

	npages = size >> PAGE_SHIFT;
	if (npages == 0)
		npages = 1;

	if (align == 0)
		align = PAGE_SIZE;
	align >>= PAGE_SHIFT;

	/* round alignment to the next power of 2, if needed */
#if defined(__NetBSD__)
	order = fls32(align);
#else
	order = fls(align);
#endif
	if ((align & (align - 1)) == 0)
		order--;
	align = BIT(order);

	/* ensure returned address is correctly aligned */
	npages = max(align, npages);

	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	mem->base.size = npages;
	mem->base.memtype = type;

#if defined(__NetBSD__)
	int ret, nsegs;

	if (align == 0)
		align = PAGE_SIZE;

	const bus_dma_tag_t dmat = nv_device(pfb)->platformdev->dmat;
	const bus_size_t dmasize = npages << PAGE_SHIFT;

	ret = -bus_dmamem_alloc(dmat, dmasize, align, 0,
	    &mem->dmaseg, 1, &nsegs, BUS_DMA_WAITOK);
	if (ret) {
fail0:		kfree(mem);
		return ret;
	}
	KASSERT(nsegs == 1);

	ret = -bus_dmamap_create(dmat, dmasize, nsegs, dmasize, 0,
	    BUS_DMA_WAITOK, &mem->base.pages);
	if (ret) {
fail1:		bus_dmamem_free(dmat, &mem->dmaseg, nsegs);
		goto fail0;
	}

	ret = -bus_dmamem_map(dmat, &mem->dmaseg, nsegs, dmasize,
	    &mem->cpuaddr, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (ret) {
fail2:		bus_dmamap_destroy(dmat, mem->base.pages);
		goto fail1;
	}
	memset(mem->cpuaddr, 0, dmasize);

	ret = -bus_dmamap_load(dmat, mem->base.pages, mem->cpuaddr,
	    dmasize, NULL, BUS_DMA_WAITOK);
	if (ret) {
fail3: __unused	bus_dmamem_unmap(dmat, mem->cpuaddr, dmasize);
		goto fail2;
	}

	nv_debug(pfb, "alloc size: 0x%x, align: 0x%x, paddr: %"PRIxPADDR
	   ", vaddr: %p\n", npages << PAGE_SHIFT, align,
	   mem->base.pages->dm_segs[0].ds_addr, mem->cpuaddr);

	mem->dmasize = dmasize;
	mem->base.offset = (u64)mem->base.pages->dm_segs[0].ds_addr;
	*pmem = &mem->base;
#else
	mem->base.pages = kzalloc(sizeof(dma_addr_t) * npages, GFP_KERNEL);
	if (!mem->base.pages) {
		kfree(mem);
		return -ENOMEM;
	}

	*pmem = &mem->base;

	mem->cpuaddr = dma_alloc_coherent(dev, npages << PAGE_SHIFT,
					  &mem->handle, GFP_KERNEL);
	if (!mem->cpuaddr) {
		nv_error(pfb, "%s: cannot allocate memory!\n", __func__);
		gk20a_ram_put(pfb, pmem);
		return -ENOMEM;
	}

	align <<= PAGE_SHIFT;

	/* alignment check */
	if (unlikely(mem->handle & (align - 1)))
		nv_warn(pfb, "memory not aligned as requested: %pad (0x%x)\n",
			&mem->handle, align);

	nv_debug(pfb, "alloc size: 0x%x, align: 0x%x, paddr: %pad, vaddr: %p\n",
		 npages << PAGE_SHIFT, align, &mem->handle, mem->cpuaddr);

	for (i = 0; i < npages; i++)
		mem->base.pages[i] = mem->handle + (PAGE_SIZE * i);

	mem->base.offset = (u64)mem->base.pages[0];
#endif

	return 0;
}

static int
gk20a_ram_ctor(struct nouveau_object *parent, struct nouveau_object *engine,
	      struct nouveau_oclass *oclass, void *data, u32 datasize,
	      struct nouveau_object **pobject)
{
	struct nouveau_ram *ram;
	int ret;

	ret = nouveau_ram_create(parent, engine, oclass, &ram);
	*pobject = nv_object(ram);
	if (ret)
		return ret;
	ram->type = NV_MEM_TYPE_STOLEN;
	ram->size = get_num_physpages() << PAGE_SHIFT;

	ram->get = gk20a_ram_get;
	ram->put = gk20a_ram_put;

	return 0;
}

struct nouveau_oclass
gk20a_ram_oclass = {
	.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = gk20a_ram_ctor,
		.dtor = _nouveau_ram_dtor,
		.init = _nouveau_ram_init,
		.fini = _nouveau_ram_fini,
	},
};
