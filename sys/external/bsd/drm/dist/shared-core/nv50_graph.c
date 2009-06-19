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
#include "nv50_grctx.h"

#define IS_G80 ((dev_priv->chipset & 0xf0) == 0x50)

static void
nv50_graph_init_reset(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t pmc_e = NV_PMC_ENABLE_PGRAPH | (1 << 21);

	DRM_DEBUG("\n");

	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) & ~pmc_e);
	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) |  pmc_e);
}

static void
nv50_graph_init_intr(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");
	NV_WRITE(NV03_PGRAPH_INTR, 0xffffffff);
	NV_WRITE(0x400138, 0xffffffff);
	NV_WRITE(NV40_PGRAPH_INTR_EN, 0xffffffff);
}

static void
nv50_graph_init_regs__nv(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	NV_WRITE(0x400804, 0xc0000000);
	NV_WRITE(0x406800, 0xc0000000);
	NV_WRITE(0x400c04, 0xc0000000);
	NV_WRITE(0x401804, 0xc0000000);
	NV_WRITE(0x405018, 0xc0000000);
	NV_WRITE(0x402000, 0xc0000000);

	NV_WRITE(0x400108, 0xffffffff);

	NV_WRITE(0x400824, 0x00004000);
	NV_WRITE(0x400500, 0x00010001);
}

static void
nv50_graph_init_regs(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	NV_WRITE(NV04_PGRAPH_DEBUG_3, (1<<2) /* HW_CONTEXT_SWITCH_ENABLED */);
}

static int
nv50_graph_init_ctxctl(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t *voodoo = NULL;

	DRM_DEBUG("\n");

	switch (dev_priv->chipset) {
	case 0x50:
		voodoo = nv50_ctxprog;
		break;
	case 0x84:
		voodoo = nv84_ctxprog;
		break;
	case 0x86:
		voodoo = nv86_ctxprog;
		break;
	case 0x92:
		voodoo = nv92_ctxprog;
		break;
	case 0x94:
	case 0x96:
		voodoo = nv94_ctxprog;
		break;
	case 0x98:
		voodoo = nv98_ctxprog;
		break;
	case 0xa0:
		voodoo = nva0_ctxprog;
		break;
	case 0xaa:
		voodoo = nvaa_ctxprog;
		break;
	default:
		DRM_ERROR("no ctxprog for chipset NV%02x\n", dev_priv->chipset);
		return -EINVAL;
	}

	NV_WRITE(NV40_PGRAPH_CTXCTL_UCODE_INDEX, 0);
	while (*voodoo != ~0) {
		NV_WRITE(NV40_PGRAPH_CTXCTL_UCODE_DATA, *voodoo);
		voodoo++;
	}

	NV_WRITE(0x400320, 4);
	NV_WRITE(NV40_PGRAPH_CTXCTL_CUR, 0);
	NV_WRITE(NV20_PGRAPH_CHANNEL_CTX_POINTER, 0);

	return 0;
}

int
nv50_graph_init(struct drm_device *dev)
{
	int ret;

	DRM_DEBUG("\n");

	nv50_graph_init_reset(dev);
	nv50_graph_init_intr(dev);
	nv50_graph_init_regs__nv(dev);
	nv50_graph_init_regs(dev);

	ret = nv50_graph_init_ctxctl(dev);
	if (ret)
		return ret;

	return 0;
}

void
nv50_graph_takedown(struct drm_device *dev)
{
	DRM_DEBUG("\n");
}

int
nv50_graph_create_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *ramin = chan->ramin->gpuobj;
	struct nouveau_gpuobj *ctx;
	struct nouveau_engine *engine = &dev_priv->Engine;
	uint32_t *ctxvals = NULL;
	int grctx_size = 0x70000, hdr;
	int ret;

	DRM_DEBUG("ch%d\n", chan->id);

	ret = nouveau_gpuobj_new_ref(dev, chan, NULL, 0, grctx_size, 0x1000,
				     NVOBJ_FLAG_ZERO_ALLOC |
				     NVOBJ_FLAG_ZERO_FREE, &chan->ramin_grctx);
	if (ret)
		return ret;
	ctx = chan->ramin_grctx->gpuobj;

	hdr = IS_G80 ? 0x200 : 0x20;
	INSTANCE_WR(ramin, (hdr + 0x00)/4, 0x00190002);
	INSTANCE_WR(ramin, (hdr + 0x04)/4, chan->ramin_grctx->instance +
					   grctx_size - 1);
	INSTANCE_WR(ramin, (hdr + 0x08)/4, chan->ramin_grctx->instance);
	INSTANCE_WR(ramin, (hdr + 0x0c)/4, 0);
	INSTANCE_WR(ramin, (hdr + 0x10)/4, 0);
	INSTANCE_WR(ramin, (hdr + 0x14)/4, 0x00010000);

	switch (dev_priv->chipset) {
	case 0x50:
		ctxvals = nv50_ctxvals;
		break;
	case 0x84:
		ctxvals = nv84_ctxvals;
		break;
	case 0x86:
		ctxvals = nv86_ctxvals;
		break;
	case 0x92:
		ctxvals = nv92_ctxvals;
		break;
	case 0x94:
		ctxvals = nv94_ctxvals;
		break;
	case 0x96:
		ctxvals = nv96_ctxvals;
		break;
	case 0x98:
		ctxvals = nv98_ctxvals;
		break;
	case 0xa0:
		ctxvals = nva0_ctxvals;
		break;
	case 0xaa:
		ctxvals = nvaa_ctxvals;
		break;
	default:
		break;
	}

	if (ctxvals) {
		int pos = 0;

		while (*ctxvals) {
			int cnt = *ctxvals++;

			while (cnt--)
				INSTANCE_WR(ctx, pos++, *ctxvals);
			ctxvals++;
		}
	} else {
		/* This is complete crack, it accidently used to make at
		 * least some G8x cards work partially somehow, though there's
		 * no good reason why - and it stopped working as the rest
		 * of the code got off the drugs..
		 */
		ret = engine->graph.load_context(chan);
		if (ret) {
			DRM_ERROR("Error hacking up context: %d\n", ret);
			return ret;
		}
	}

	INSTANCE_WR(ctx, 0x00000/4, chan->ramin->instance >> 12);
	if ((dev_priv->chipset & 0xf0) == 0xa0)
		INSTANCE_WR(ctx, 0x00004/4, 0x00000002);
	else
		INSTANCE_WR(ctx, 0x0011c/4, 0x00000002);

	return 0;
}

void
nv50_graph_destroy_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int i, hdr;

	DRM_DEBUG("ch%d\n", chan->id);

	hdr = IS_G80 ? 0x200 : 0x20;
	for (i=hdr; i<hdr+24; i+=4)
		INSTANCE_WR(chan->ramin->gpuobj, i/4, 0);

	nouveau_gpuobj_ref_del(dev, &chan->ramin_grctx);
}

static int
nv50_graph_transfer_context(struct drm_device *dev, uint32_t inst, int save)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t old_cp, tv = 20000;
	int i;

	DRM_DEBUG("inst=0x%08x, save=%d\n", inst, save);

	old_cp = NV_READ(NV20_PGRAPH_CHANNEL_CTX_POINTER);
	NV_WRITE(NV20_PGRAPH_CHANNEL_CTX_POINTER, inst);
	NV_WRITE(0x400824, NV_READ(0x400824) |
		 (save ? NV40_PGRAPH_CTXCTL_0310_XFER_SAVE :
			 NV40_PGRAPH_CTXCTL_0310_XFER_LOAD));
	NV_WRITE(NV40_PGRAPH_CTXCTL_0304, NV40_PGRAPH_CTXCTL_0304_XFER_CTX);

	for (i = 0; i < tv; i++) {
		if (NV_READ(NV40_PGRAPH_CTXCTL_030C) == 0)
			break;
	}
	NV_WRITE(NV20_PGRAPH_CHANNEL_CTX_POINTER, old_cp);

	if (i == tv) {
		DRM_ERROR("failed: inst=0x%08x save=%d\n", inst, save);
		DRM_ERROR("0x40030C = 0x%08x\n",
			  NV_READ(NV40_PGRAPH_CTXCTL_030C));
		return -EBUSY;
	}

	return 0;
}

int
nv50_graph_load_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t inst = chan->ramin->instance >> 12;
	int ret; (void)ret;

	DRM_DEBUG("ch%d\n", chan->id);

#if 0
	if ((ret = nv50_graph_transfer_context(dev, inst, 0)))
		return ret;
#endif

	NV_WRITE(NV20_PGRAPH_CHANNEL_CTX_POINTER, inst);
	NV_WRITE(0x400320, 4);
	NV_WRITE(NV40_PGRAPH_CTXCTL_CUR, inst | (1<<31));

	return 0;
}

int
nv50_graph_save_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	uint32_t inst = chan->ramin->instance >> 12;

	DRM_DEBUG("ch%d\n", chan->id);

	return nv50_graph_transfer_context(dev, inst, 1);
}
