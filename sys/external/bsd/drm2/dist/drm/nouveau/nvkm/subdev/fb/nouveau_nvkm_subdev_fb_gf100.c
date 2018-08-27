/*	$NetBSD: nouveau_nvkm_subdev_fb_gf100.c,v 1.2 2018/08/27 04:58:33 riastradh Exp $	*/

/*
 * Copyright 2012 Red Hat Inc.
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
 *
 * Authors: Ben Skeggs
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvkm_subdev_fb_gf100.c,v 1.2 2018/08/27 04:58:33 riastradh Exp $");

#include "gf100.h"
#include "ram.h"

extern const u8 gf100_pte_storage_type_map[256];

bool
gf100_fb_memtype_valid(struct nvkm_fb *fb, u32 tile_flags)
{
	u8 memtype = (tile_flags & 0x0000ff00) >> 8;
	return likely((gf100_pte_storage_type_map[memtype] != 0xff));
}

void
gf100_fb_intr(struct nvkm_fb *base)
{
	struct gf100_fb *fb = gf100_fb(base);
	struct nvkm_subdev *subdev = &fb->base.subdev;
	struct nvkm_device *device = subdev->device;
	u32 intr = nvkm_rd32(device, 0x000100);
	if (intr & 0x08000000)
		nvkm_debug(subdev, "PFFB intr\n");
	if (intr & 0x00002000)
		nvkm_debug(subdev, "PBFB intr\n");
}

void
gf100_fb_init(struct nvkm_fb *base)
{
	struct gf100_fb *fb = gf100_fb(base);
	struct nvkm_device *device = fb->base.subdev.device;

#ifdef __NetBSD__
	if (fb->r100c10_map)
		nvkm_wr32(device, 0x100c10, fb->r100c10 >> 8);
#else
	if (fb->r100c10_page)
		nvkm_wr32(device, 0x100c10, fb->r100c10 >> 8);
#endif

	nvkm_mask(device, 0x100c80, 0x00000001, 0x00000000); /* 128KiB lpg */
}

void *
gf100_fb_dtor(struct nvkm_fb *base)
{
	struct gf100_fb *fb = gf100_fb(base);
	struct nvkm_device *device = fb->base.subdev.device;

#ifdef __NetBSD__
	if (fb->r100c10_map) {
		const bus_dma_tag_t dmat = device->func->dma_tag(device);

		bus_dmamap_unload(dmat, fb->r100c10_map);
		bus_dmamem_unmap(dmat, fb->r100c10_kva, PAGE_SIZE);
		bus_dmamap_destroy(dmat, fb->r100c10_map);
		bus_dmamem_free(dmat, &fb->r100c10_seg, 1);
		fb->r100c10_map = NULL;
	}
#else
	if (fb->r100c10_page) {
		dma_unmap_page(device->dev, fb->r100c10, PAGE_SIZE,
			       DMA_BIDIRECTIONAL);
		__free_page(fb->r100c10_page);
	}
#endif

	return fb;
}

int
gf100_fb_new_(const struct nvkm_fb_func *func, struct nvkm_device *device,
	      int index, struct nvkm_fb **pfb)
{
	struct gf100_fb *fb;

	if (!(fb = kzalloc(sizeof(*fb), GFP_KERNEL)))
		return -ENOMEM;
	nvkm_fb_ctor(func, device, index, &fb->base);
	*pfb = &fb->base;

#ifdef __NetBSD__
    {
	const bus_dma_tag_t dmat = device->func->dma_tag(device);
	int nsegs;

	fb->r100c10_map = NULL; /* paranoia */
	fb->r100c10_kva = NULL;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &fb->r100c10_seg, 1, &nsegs, BUS_DMA_WAITOK);
	if (ret)
fail0:		return ret;
	KASSERT(nsegs == 1);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_create(dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &fb->r100c10_map);
	if (ret) {
fail1:		bus_dmamem_free(dmat, &fb->r100c10_seg, 1);
		goto fail0;
	}

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_map(dmat, &fb->r100c10_seg, 1, PAGE_SIZE,
	    &fb->r100c10_kva, BUS_DMA_WAITOK);
	if (ret) {
fail2:		bus_dmamap_destroy(dmat, fb->r100c10_map);
		goto fail1;
	}
	(void)memset(fb->r100c10_kva, 0, PAGE_SIZE);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load(dmat, fb->r100c10_map, fb->r100c10_kva,
	    PAGE_SIZE, NULL, BUS_DMA_WAITOK);
	if (ret) {
fail3: __unused	bus_dmamem_unmap(dmat, fb->r100c10_kva, PAGE_SIZE);
		goto fail2;
	}

	fb->r100c10 = fb->r100c10_map->dm_segs[0].ds_addr;
    }
#else
	fb->r100c10_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (fb->r100c10_page) {
		fb->r100c10 = dma_map_page(device->dev, fb->r100c10_page, 0,
					   PAGE_SIZE, DMA_BIDIRECTIONAL);
		if (dma_mapping_error(device->dev, fb->r100c10))
			return -EFAULT;
	}
#endif

	return 0;
}

static const struct nvkm_fb_func
gf100_fb = {
	.dtor = gf100_fb_dtor,
	.init = gf100_fb_init,
	.intr = gf100_fb_intr,
	.ram_new = gf100_ram_new,
	.memtype_valid = gf100_fb_memtype_valid,
};

int
gf100_fb_new(struct nvkm_device *device, int index, struct nvkm_fb **pfb)
{
	return gf100_fb_new_(&gf100_fb, device, index, pfb);
}
