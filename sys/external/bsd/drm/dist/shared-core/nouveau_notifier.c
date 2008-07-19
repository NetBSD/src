/*
 * Copyright (C) 2007 Ben Skeggs.
 *
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

int
nouveau_notifier_init_channel(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	int flags, ret;

	flags = (NOUVEAU_MEM_PCI | NOUVEAU_MEM_MAPPED |
	         NOUVEAU_MEM_FB_ACCEPTABLE);

	chan->notifier_block = nouveau_mem_alloc(dev, 0, PAGE_SIZE, flags,
						 (struct drm_file *)-2);
	if (!chan->notifier_block)
		return -ENOMEM;
	DRM_DEBUG("Allocated notifier block in 0x%08x\n",
		  chan->notifier_block->flags);

	ret = nouveau_mem_init_heap(&chan->notifier_heap,
				    0, chan->notifier_block->size);
	if (ret)
		return ret;

	return 0;
}

void
nouveau_notifier_takedown_channel(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;

	if (chan->notifier_block) {
		nouveau_mem_free(dev, chan->notifier_block);
		chan->notifier_block = NULL;
	}

	nouveau_mem_takedown(&chan->notifier_heap);
}

static void
nouveau_notifier_gpuobj_dtor(struct drm_device *dev,
			     struct nouveau_gpuobj *gpuobj)
{
	DRM_DEBUG("\n");

	if (gpuobj->priv)
		nouveau_mem_free_block(gpuobj->priv);
}

int
nouveau_notifier_alloc(struct nouveau_channel *chan, uint32_t handle,
		       int count, uint32_t *b_offset)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *nobj = NULL;
	struct mem_block *mem;
	uint32_t offset;
	int target, ret;

	if (!chan->notifier_heap) {
		DRM_ERROR("Channel %d doesn't have a notifier heap!\n",
			  chan->id);
		return -EINVAL;
	}

	mem = nouveau_mem_alloc_block(chan->notifier_heap, count*32, 0,
				      (struct drm_file *)-2, 0);
	if (!mem) {
		DRM_ERROR("Channel %d notifier block full\n", chan->id);
		return -ENOMEM;
	}
	mem->flags = NOUVEAU_MEM_NOTIFIER;

	offset = chan->notifier_block->start;
	if (chan->notifier_block->flags & NOUVEAU_MEM_FB) {
		target = NV_DMA_TARGET_VIDMEM;
	} else
	if (chan->notifier_block->flags & NOUVEAU_MEM_AGP) {
		if (dev_priv->gart_info.type == NOUVEAU_GART_SGDMA &&
		    dev_priv->card_type < NV_50) {
			ret = nouveau_sgdma_get_page(dev, offset, &offset);
			if (ret)
				return ret;
			target = NV_DMA_TARGET_PCI;
		} else {
			target = NV_DMA_TARGET_AGP;
		}
	} else
	if (chan->notifier_block->flags & NOUVEAU_MEM_PCI) {
		target = NV_DMA_TARGET_PCI_NONLINEAR;
	} else {
		DRM_ERROR("Bad DMA target, flags 0x%08x!\n",
			  chan->notifier_block->flags);
		return -EINVAL;
	}
	offset += mem->start;

	if ((ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					  offset, mem->size,
					  NV_DMA_ACCESS_RW, target, &nobj))) {
		nouveau_mem_free_block(mem);
		DRM_ERROR("Error creating notifier ctxdma: %d\n", ret);
		return ret;
	}
	nobj->dtor   = nouveau_notifier_gpuobj_dtor;
	nobj->priv   = mem;

	if ((ret = nouveau_gpuobj_ref_add(dev, chan, handle, nobj, NULL))) {
		nouveau_gpuobj_del(dev, &nobj);
		nouveau_mem_free_block(mem);
		DRM_ERROR("Error referencing notifier ctxdma: %d\n", ret);
		return ret;
	}

	*b_offset = mem->start;
	return 0;
}

int
nouveau_ioctl_notifier_alloc(struct drm_device *dev, void *data,
			     struct drm_file *file_priv)
{
	struct drm_nouveau_notifierobj_alloc *na = data;
	struct nouveau_channel *chan;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;
	NOUVEAU_GET_USER_CHANNEL_WITH_RETURN(na->channel, file_priv, chan);

	ret = nouveau_notifier_alloc(chan, na->handle, na->count, &na->offset);
	if (ret)
		return ret;

	return 0;
}
