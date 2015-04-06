/*	$NetBSD: nouveau_subdev_fb_nvc0.c,v 1.1.1.1.8.1 2015/04/06 15:18:16 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_subdev_fb_nvc0.c,v 1.1.1.1.8.1 2015/04/06 15:18:16 skrll Exp $");

#include "nvc0.h"

extern const u8 nvc0_pte_storage_type_map[256];

bool
nvc0_fb_memtype_valid(struct nouveau_fb *pfb, u32 tile_flags)
{
	u8 memtype = (tile_flags & 0x0000ff00) >> 8;
	return likely((nvc0_pte_storage_type_map[memtype] != 0xff));
}

static void
nvc0_fb_intr(struct nouveau_subdev *subdev)
{
	struct nvc0_fb_priv *priv = (void *)subdev;
	u32 intr = nv_rd32(priv, 0x000100);
	if (intr & 0x08000000) {
		nv_debug(priv, "PFFB intr\n");
		intr &= ~0x08000000;
	}
	if (intr & 0x00002000) {
		nv_debug(priv, "PBFB intr\n");
		intr &= ~0x00002000;
	}
}

int
nvc0_fb_init(struct nouveau_object *object)
{
	struct nvc0_fb_priv *priv = (void *)object;
	int ret;

	ret = nouveau_fb_init(&priv->base);
	if (ret)
		return ret;

#ifdef __NetBSD__
	if (priv->r100c10_map)
		nv_wr32(priv, 0x100c10, priv->r100c10 >> 8);
#else
	if (priv->r100c10_page)
		nv_wr32(priv, 0x100c10, priv->r100c10 >> 8);
#endif
	return 0;
}

void
nvc0_fb_dtor(struct nouveau_object *object)
{
	struct nouveau_device *device = nv_device(object);
	struct nvc0_fb_priv *priv = (void *)object;

#ifdef __NetBSD__
	if (priv->r100c10_map) {
		const bus_dma_tag_t dmat = device->pdev->pd_pa.pa_dmat64;

		bus_dmamap_unload(dmat, priv->r100c10_map);
		bus_dmamem_unmap(dmat, priv->r100c10_kva, PAGE_SIZE);
		bus_dmamap_destroy(dmat, priv->r100c10_map);
		bus_dmamem_free(dmat, &priv->r100c10_seg, 1);
	}
#else
	if (priv->r100c10_page) {
		nv_device_unmap_page(device, priv->r100c10);
		__free_page(priv->r100c10_page);
	}
#endif

	nouveau_fb_destroy(&priv->base);
}

int
nvc0_fb_ctor(struct nouveau_object *parent, struct nouveau_object *engine,
	     struct nouveau_oclass *oclass, void *data, u32 size,
	     struct nouveau_object **pobject)
{
	struct nouveau_device *device = nv_device(parent);
	struct nvc0_fb_priv *priv;
	int ret;

	ret = nouveau_fb_create(parent, engine, oclass, &priv);
	*pobject = nv_object(priv);
	if (ret)
		return ret;

#ifdef __NetBSD__
    {
	/* XXX pa_dmat or pa_dmat64?  */
	const bus_dma_tag_t dmat = device->pdev->pd_pa.pa_dmat64;
	int nsegs;

	priv->r100c10_map = NULL; /* paranoia */
	priv->r100c10_kva = NULL;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &priv->r100c10_seg, 1, &nsegs, BUS_DMA_WAITOK);
	if (ret) {
fail0:		nouveau_fb_destroy(&priv->base);
		return ret;
	}
	KASSERT(nsegs == 1);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_create(dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &priv->r100c10_map);
	if (ret) {
fail1:		bus_dmamem_free(dmat, &priv->r100c10_seg, 1);
		goto fail0;
	}

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_map(dmat, &priv->r100c10_seg, 1, PAGE_SIZE,
	    &priv->r100c10_kva, BUS_DMA_WAITOK);
	if (ret) {
fail2:		bus_dmamap_destroy(dmat, priv->r100c10_map);
		goto fail1;
	}
	(void)memset(priv->r100c10_kva, 0, PAGE_SIZE);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load(dmat, priv->r100c10_map, priv->r100c10_kva,
	    PAGE_SIZE, NULL, BUS_DMA_WAITOK);
	if (ret) {
fail3: __unused	bus_dmamem_unmap(dmat, priv->r100c10_kva, PAGE_SIZE);
		goto fail2;
	}

	priv->r100c10 = priv->r100c10_map->dm_segs[0].ds_addr;
    }
#else
	priv->r100c10_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (priv->r100c10_page) {
		priv->r100c10 = nv_device_map_page(device, priv->r100c10_page);
		if (!priv->r100c10)
			return -EFAULT;
	}
#endif

	nv_subdev(priv)->intr = nvc0_fb_intr;
	return 0;
}

struct nouveau_oclass *
nvc0_fb_oclass = &(struct nouveau_fb_impl) {
	.base.handle = NV_SUBDEV(FB, 0xc0),
	.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = nvc0_fb_ctor,
		.dtor = nvc0_fb_dtor,
		.init = nvc0_fb_init,
		.fini = _nouveau_fb_fini,
	},
	.memtype = nvc0_fb_memtype_valid,
	.ram = &nvc0_ram_oclass,
}.base;
