/*	$NetBSD: nouveau_subdev_instmem_nv40.c,v 1.2.2.1 2016/03/19 11:30:30 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_subdev_instmem_nv40.c,v 1.2.2.1 2016/03/19 11:30:30 skrll Exp $");

#include <engine/graph/nv40.h>

#include "nv04.h"

/******************************************************************************
 * instmem subdev implementation
 *****************************************************************************/

static u32
nv40_instmem_rd32(struct nouveau_object *object, u64 addr)
{
	struct nv04_instmem_priv *priv = (void *)object;
#ifdef __NetBSD__
	return bus_space_read_4(priv->iomemt, priv->iomemh, addr);
#else
	return ioread32_native(priv->iomem + addr);
#endif
}

static void
nv40_instmem_wr32(struct nouveau_object *object, u64 addr, u32 data)
{
	struct nv04_instmem_priv *priv = (void *)object;
#ifdef __NetBSD__
	bus_space_write_4(priv->iomemt, priv->iomemh, addr, data);
#else
	iowrite32_native(data, priv->iomem + addr);
#endif
}

static int
nv40_instmem_ctor(struct nouveau_object *parent, struct nouveau_object *engine,
		  struct nouveau_oclass *oclass, void *data, u32 size,
		  struct nouveau_object **pobject)
{
	struct nouveau_device *device = nv_device(parent);
	struct nv04_instmem_priv *priv;
	int ret, bar, vs;

	ret = nouveau_instmem_create(parent, engine, oclass, &priv);
	*pobject = nv_object(priv);
	if (ret)
		return ret;

	/* map bar */
	if (nv_device_resource_len(device, 2))
		bar = 2;
	else
		bar = 3;

#ifdef __NetBSD__
	priv->iomemt = nv_device_resource_tag(device, bar);
	priv->iomemsz = nv_device_resource_len(device, bar);
	ret = bus_space_map(priv->iomemt,
	    nv_device_resource_start(device, bar),
	    priv->iomemsz, 0, &priv->iomemh);
	if (ret) {
		priv->iomemsz = 0;
		nv_error(priv, "unable to map PRAMIN BAR: %d\n", ret);
		return -EFAULT;
	}
#else
	priv->iomem = ioremap(nv_device_resource_start(device, bar),
			      nv_device_resource_len(device, bar));
	if (!priv->iomem) {
		nv_error(priv, "unable to map PRAMIN BAR\n");
		return -EFAULT;
	}
#endif

	/* PRAMIN aperture maps over the end of vram, reserve enough space
	 * to fit graphics contexts for every channel, the magics come
	 * from engine/graph/nv40.c
	 */
	vs = hweight8((nv_rd32(priv, 0x001540) & 0x0000ff00) >> 8);
	if      (device->chipset == 0x40) priv->base.reserved = 0x6aa0 * vs;
	else if (device->chipset  < 0x43) priv->base.reserved = 0x4f00 * vs;
	else if (nv44_graph_class(priv))  priv->base.reserved = 0x4980 * vs;
	else				  priv->base.reserved = 0x4a40 * vs;
	priv->base.reserved += 16 * 1024;
	priv->base.reserved *= 32;		/* per-channel */
	priv->base.reserved += 512 * 1024;	/* pci(e)gart table */
	priv->base.reserved += 512 * 1024;	/* object storage */

	priv->base.reserved = round_up(priv->base.reserved, 4096);

	ret = nouveau_mm_init(&priv->heap, 0, priv->base.reserved, 1);
	if (ret)
		return ret;

	/* 0x00000-0x10000: reserve for probable vbios image */
	ret = nouveau_gpuobj_new(nv_object(priv), NULL, 0x10000, 0, 0,
				&priv->vbios);
	if (ret)
		return ret;

	/* 0x10000-0x18000: reserve for RAMHT */
	ret = nouveau_ramht_new(nv_object(priv), NULL, 0x08000, 0,
			       &priv->ramht);
	if (ret)
		return ret;

	/* 0x18000-0x18200: reserve for RAMRO
	 * 0x18200-0x20000: padding
	 */
	ret = nouveau_gpuobj_new(nv_object(priv), NULL, 0x08000, 0, 0,
				&priv->ramro);
	if (ret)
		return ret;

	/* 0x20000-0x21000: reserve for RAMFC
	 * 0x21000-0x40000: padding and some unknown crap
	 */
	ret = nouveau_gpuobj_new(nv_object(priv), NULL, 0x20000, 0,
				 NVOBJ_FLAG_ZERO_ALLOC, &priv->ramfc);
	if (ret)
		return ret;

	return 0;
}

struct nouveau_oclass *
nv40_instmem_oclass = &(struct nouveau_instmem_impl) {
	.base.handle = NV_SUBDEV(INSTMEM, 0x40),
	.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = nv40_instmem_ctor,
		.dtor = nv04_instmem_dtor,
		.init = _nouveau_instmem_init,
		.fini = _nouveau_instmem_fini,
		.rd32 = nv40_instmem_rd32,
		.wr32 = nv40_instmem_wr32,
	},
	.instobj = &nv04_instobj_oclass.base,
}.base;
