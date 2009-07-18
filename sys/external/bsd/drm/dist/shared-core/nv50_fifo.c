/*
 * Copyright (C) 2007 Ben Skeggs.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"

struct nv50_fifo_priv {
	struct nouveau_gpuobj_ref *thingo[2];
	int cur_thingo;
};

#define IS_G80 ((dev_priv->chipset & 0xf0) == 0x50)

static void
nv50_fifo_init_thingo(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nv50_fifo_priv *priv = dev_priv->Engine.fifo.priv;
	struct nouveau_gpuobj_ref *cur;
	int i, nr;

	DRM_DEBUG("\n");

	cur = priv->thingo[priv->cur_thingo];
	priv->cur_thingo = !priv->cur_thingo;

	/* We never schedule channel 0 or 127 */
	for (i = 1, nr = 0; i < 127; i++) {
		if (dev_priv->fifos[i]) {
			INSTANCE_WR(cur->gpuobj, nr++, i);
		}
	}
	NV_WRITE(0x32f4, cur->instance >> 12);
	NV_WRITE(0x32ec, nr);
	NV_WRITE(0x2500, 0x101);
}

static int
nv50_fifo_channel_enable(struct drm_device *dev, int channel, int nt)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_channel *chan = dev_priv->fifos[channel];
	uint32_t inst;

	DRM_DEBUG("ch%d\n", channel);

	if (!chan->ramfc)
		return -EINVAL;

	if (IS_G80) inst = chan->ramfc->instance >> 12;
	else        inst = chan->ramfc->instance >> 8;
	NV_WRITE(NV50_PFIFO_CTX_TABLE(channel),
		 inst | NV50_PFIFO_CTX_TABLE_CHANNEL_ENABLED);

	if (!nt) nv50_fifo_init_thingo(dev);
	return 0;
}

static void
nv50_fifo_channel_disable(struct drm_device *dev, int channel, int nt)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t inst;

	DRM_DEBUG("ch%d, nt=%d\n", channel, nt);

	if (IS_G80) inst = NV50_PFIFO_CTX_TABLE_INSTANCE_MASK_G80;
	else        inst = NV50_PFIFO_CTX_TABLE_INSTANCE_MASK_G84;
	NV_WRITE(NV50_PFIFO_CTX_TABLE(channel), inst);

	if (!nt) nv50_fifo_init_thingo(dev);
}

static void
nv50_fifo_init_reset(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t pmc_e = NV_PMC_ENABLE_PFIFO;

	DRM_DEBUG("\n");

	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) & ~pmc_e);
	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) |  pmc_e);
}

static void
nv50_fifo_init_intr(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	NV_WRITE(NV03_PFIFO_INTR_0, 0xFFFFFFFF);
	NV_WRITE(NV03_PFIFO_INTR_EN_0, 0xFFFFFFFF);
}

static void
nv50_fifo_init_context_table(struct drm_device *dev)
{
	int i;

	DRM_DEBUG("\n");

	for (i = 0; i < NV50_PFIFO_CTX_TABLE__SIZE; i++)
		nv50_fifo_channel_disable(dev, i, 1);
	nv50_fifo_init_thingo(dev);
}

static void
nv50_fifo_init_regs__nv(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	NV_WRITE(0x250c, 0x6f3cfc34);
}

static void
nv50_fifo_init_regs(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	NV_WRITE(0x2500, 0);
	NV_WRITE(0x3250, 0);
	NV_WRITE(0x3220, 0);
	NV_WRITE(0x3204, 0);
	NV_WRITE(0x3210, 0);
	NV_WRITE(0x3270, 0);

	/* Enable dummy channels setup by nv50_instmem.c */
	nv50_fifo_channel_enable(dev, 0, 1);
	nv50_fifo_channel_enable(dev, 127, 1);
}

int
nv50_fifo_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nv50_fifo_priv *priv;
	int ret;

	DRM_DEBUG("\n");

	priv = drm_calloc(1, sizeof(*priv), DRM_MEM_DRIVER);
	if (!priv)
		return -ENOMEM;
	dev_priv->Engine.fifo.priv = priv;

	nv50_fifo_init_reset(dev);
	nv50_fifo_init_intr(dev);

	ret = nouveau_gpuobj_new_ref(dev, NULL, NULL, 0, 128*4, 0x1000,
				     NVOBJ_FLAG_ZERO_ALLOC, &priv->thingo[0]);
	if (ret) {
		DRM_ERROR("error creating thingo0: %d\n", ret);
		return ret;
	}

	ret = nouveau_gpuobj_new_ref(dev, NULL, NULL, 0, 128*4, 0x1000,
				     NVOBJ_FLAG_ZERO_ALLOC, &priv->thingo[1]);
	if (ret) {
		DRM_ERROR("error creating thingo1: %d\n", ret);
		return ret;
	}

	nv50_fifo_init_context_table(dev);
	nv50_fifo_init_regs__nv(dev);
	nv50_fifo_init_regs(dev);

	return 0;
}

void
nv50_fifo_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nv50_fifo_priv *priv = dev_priv->Engine.fifo.priv;

	DRM_DEBUG("\n");

	if (!priv)
		return;

	nouveau_gpuobj_ref_del(dev, &priv->thingo[0]);
	nouveau_gpuobj_ref_del(dev, &priv->thingo[1]);

	dev_priv->Engine.fifo.priv = NULL;
	drm_free(priv, sizeof(*priv), DRM_MEM_DRIVER);
}

int
nv50_fifo_channel_id(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	return (NV_READ(NV03_PFIFO_CACHE1_PUSH1) &
			NV50_PFIFO_CACHE1_PUSH1_CHID_MASK);
}

int
nv50_fifo_create_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *ramfc = NULL;
	int ret;

	DRM_DEBUG("ch%d\n", chan->id);

	if (IS_G80) {
		uint32_t ramfc_offset = chan->ramin->gpuobj->im_pramin->start;
		uint32_t vram_offset = chan->ramin->gpuobj->im_backing->start;
		ret = nouveau_gpuobj_new_fake(dev, ramfc_offset, vram_offset,
					      0x100, NVOBJ_FLAG_ZERO_ALLOC |
					      NVOBJ_FLAG_ZERO_FREE, &ramfc,
					      &chan->ramfc);
		if (ret)
			return ret;
	} else {
		ret = nouveau_gpuobj_new_ref(dev, chan, NULL, 0, 0x100, 256,
					     NVOBJ_FLAG_ZERO_ALLOC |
					     NVOBJ_FLAG_ZERO_FREE,
					     &chan->ramfc);
		if (ret)
			return ret;
		ramfc = chan->ramfc->gpuobj;
	}

	INSTANCE_WR(ramfc, 0x48/4, chan->pushbuf->instance >> 4);
	INSTANCE_WR(ramfc, 0x80/4, (0xc << 24) | (chan->ramht->instance >> 4));
	INSTANCE_WR(ramfc, 0x3c/4, 0x000f0078); /* fetch? */
	INSTANCE_WR(ramfc, 0x44/4, 0x2101ffff);
	INSTANCE_WR(ramfc, 0x60/4, 0x7fffffff);
	INSTANCE_WR(ramfc, 0x10/4, 0x00000000);
	INSTANCE_WR(ramfc, 0x08/4, 0x00000000);
	INSTANCE_WR(ramfc, 0x40/4, 0x00000000);
	INSTANCE_WR(ramfc, 0x50/4, 0x2039b2e0);
	INSTANCE_WR(ramfc, 0x54/4, 0x000f0000);
	INSTANCE_WR(ramfc, 0x7c/4, 0x30000001);
	INSTANCE_WR(ramfc, 0x78/4, 0x00000000);
	INSTANCE_WR(ramfc, 0x4c/4, chan->pushbuf_mem->size - 1);

	if (!IS_G80) {
		INSTANCE_WR(chan->ramin->gpuobj, 0, chan->id);
		INSTANCE_WR(chan->ramin->gpuobj, 1, chan->ramfc->instance >> 8);

		INSTANCE_WR(ramfc, 0x88/4, 0x3d520); /* some vram addy >> 10 */
		INSTANCE_WR(ramfc, 0x98/4, chan->ramin->instance >> 12);
	}

	ret = nv50_fifo_channel_enable(dev, chan->id, 0);
	if (ret) {
		DRM_ERROR("error enabling ch%d: %d\n", chan->id, ret);
		nouveau_gpuobj_ref_del(dev, &chan->ramfc);
		return ret;
	}

	return 0;
}

void
nv50_fifo_destroy_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("ch%d\n", chan->id);

	nv50_fifo_channel_disable(dev, chan->id, 0);

	/* Dummy channel, also used on ch 127 */
	if (chan->id == 0)
		nv50_fifo_channel_disable(dev, 127, 0);

	if ((NV_READ(NV03_PFIFO_CACHE1_PUSH1) & 0xffff) == chan->id)
		NV_WRITE(NV03_PFIFO_CACHE1_PUSH1, 127);

	nouveau_gpuobj_ref_del(dev, &chan->ramfc);
}

int
nv50_fifo_load_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *ramfc = chan->ramfc->gpuobj;

	DRM_DEBUG("ch%d\n", chan->id);

	/*XXX: incomplete, only touches the regs that NV does */

	NV_WRITE(0x3244, 0);
	NV_WRITE(0x3240, 0);

	NV_WRITE(0x3224, INSTANCE_RD(ramfc, 0x3c/4));
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_INSTANCE, INSTANCE_RD(ramfc, 0x48/4));
	NV_WRITE(0x3234, INSTANCE_RD(ramfc, 0x4c/4));
	NV_WRITE(0x3254, 1);
	NV_WRITE(NV03_PFIFO_RAMHT, INSTANCE_RD(ramfc, 0x80/4));

	if (!IS_G80) {
		NV_WRITE(0x340c, INSTANCE_RD(ramfc, 0x88/4));
		NV_WRITE(0x3410, INSTANCE_RD(ramfc, 0x98/4));
	}

	NV_WRITE(NV03_PFIFO_CACHE1_PUSH1, chan->id | (1<<16));
	return 0;
}

int
nv50_fifo_save_context(struct nouveau_channel *chan)
{
	DRM_DEBUG("ch%d\n", chan->id);
	DRM_ERROR("stub!\n");
	return 0;
}
