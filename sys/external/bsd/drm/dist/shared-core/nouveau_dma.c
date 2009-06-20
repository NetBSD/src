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
#include "nouveau_dma.h"

int
nouveau_dma_channel_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_drm_channel *dchan = &dev_priv->channel;
	struct nouveau_gpuobj *gpuobj = NULL;
	struct mem_block *pushbuf;
	int grclass, ret, i;

	DRM_DEBUG("\n");

	pushbuf = nouveau_mem_alloc(dev, 0, 0x8000,
				    NOUVEAU_MEM_FB | NOUVEAU_MEM_MAPPED,
				    (struct drm_file *)-2);
	if (!pushbuf) {
		DRM_ERROR("Failed to allocate DMA push buffer\n");
		return -ENOMEM;
	}

	/* Allocate channel */
	ret = nouveau_fifo_alloc(dev, &dchan->chan, (struct drm_file *)-2,
				 pushbuf, NvDmaFB, NvDmaTT);
	if (ret) {
		DRM_ERROR("Error allocating GPU channel: %d\n", ret);
		return ret;
	}
	DRM_DEBUG("Using FIFO channel %d\n", dchan->chan->id);

	/* Map push buffer */
	drm_core_ioremap(dchan->chan->pushbuf_mem->map, dev);
	if (!dchan->chan->pushbuf_mem->map->handle) {
		DRM_ERROR("Failed to ioremap push buffer\n");
		return -EINVAL;
	}
	dchan->pushbuf = (void*)dchan->chan->pushbuf_mem->map->handle;

	/* Initialise DMA vars */
	dchan->max  = (dchan->chan->pushbuf_mem->size >> 2) - 2;
	dchan->put  = dchan->chan->pushbuf_base >> 2;
	dchan->cur  = dchan->put;
	dchan->free = dchan->max - dchan->cur;

	/* Insert NOPS for NOUVEAU_DMA_SKIPS */
	dchan->free -= NOUVEAU_DMA_SKIPS;
	dchan->push_free = NOUVEAU_DMA_SKIPS;
	for (i=0; i < NOUVEAU_DMA_SKIPS; i++)
		OUT_RING(0);

	/* NV_MEMORY_TO_MEMORY_FORMAT requires a notifier */
	if ((ret = nouveau_notifier_alloc(dchan->chan, NvNotify0, 1,
					  &dchan->notify0_offset))) {
		DRM_ERROR("Error allocating NvNotify0: %d\n", ret);
		return ret;
	}

	/* We use NV_MEMORY_TO_MEMORY_FORMAT for buffer moves */
	if (dev_priv->card_type < NV_50) grclass = NV_MEMORY_TO_MEMORY_FORMAT;
	else                             grclass = NV50_MEMORY_TO_MEMORY_FORMAT;
	if ((ret = nouveau_gpuobj_gr_new(dchan->chan, grclass, &gpuobj))) {
		DRM_ERROR("Error creating NvM2MF: %d\n", ret);
		return ret;
	}

	if ((ret = nouveau_gpuobj_ref_add(dev, dchan->chan, NvM2MF,
					  gpuobj, NULL))) {
		DRM_ERROR("Error referencing NvM2MF: %d\n", ret);
		return ret;
	}
	dchan->m2mf_dma_source = NvDmaFB;
	dchan->m2mf_dma_destin = NvDmaFB;

	BEGIN_RING(NvSubM2MF, NV_MEMORY_TO_MEMORY_FORMAT_NAME, 1);
	OUT_RING  (NvM2MF);
	BEGIN_RING(NvSubM2MF, NV_MEMORY_TO_MEMORY_FORMAT_SET_DMA_NOTIFY, 1);
	OUT_RING  (NvNotify0);
	BEGIN_RING(NvSubM2MF, NV_MEMORY_TO_MEMORY_FORMAT_SET_DMA_SOURCE, 2);
	OUT_RING  (dchan->m2mf_dma_source);
	OUT_RING  (dchan->m2mf_dma_destin);
	FIRE_RING();

	return 0;
}

void
nouveau_dma_channel_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_drm_channel *dchan = &dev_priv->channel;

	DRM_DEBUG("\n");

	if (dchan->chan) {
		drm_core_ioremapfree(dchan->chan->pushbuf_mem->map, dev);
		nouveau_fifo_free(dchan->chan);
		dchan->chan = NULL;
	}
}

#define READ_GET() ((NV_READ(dchan->chan->get) -                               \
		    dchan->chan->pushbuf_base) >> 2)
#define WRITE_PUT(val) do {                                                    \
	NV_WRITE(dchan->chan->put,                                             \
		 ((val) << 2) + dchan->chan->pushbuf_base);                    \
} while(0)

int
nouveau_dma_wait(struct drm_device *dev, int size)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_drm_channel *dchan = &dev_priv->channel;
	uint32_t get;

	while (dchan->free < size) {
		get = READ_GET();

		if (dchan->put >= get) {
			dchan->free = dchan->max - dchan->cur;

			if (dchan->free < size) {
				dchan->push_free = 1;
				OUT_RING(0x20000000|dchan->chan->pushbuf_base);
				if (get <= NOUVEAU_DMA_SKIPS) {
					/*corner case - will be idle*/
					if (dchan->put <= NOUVEAU_DMA_SKIPS)
						WRITE_PUT(NOUVEAU_DMA_SKIPS + 1);

					do {
						get = READ_GET();
					} while (get <= NOUVEAU_DMA_SKIPS);
				}

				WRITE_PUT(NOUVEAU_DMA_SKIPS);
				dchan->cur  = dchan->put = NOUVEAU_DMA_SKIPS;
				dchan->free = get - (NOUVEAU_DMA_SKIPS + 1);
			}
		} else {
			dchan->free = get - dchan->cur - 1;
		}
	}

	return 0;
}
