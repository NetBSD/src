/*
 * Copyright 2005-2006 Stephane Marchesin
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"
#include "nouveau_drm.h"


/* returns the size of fifo context */
int nouveau_fifo_ctx_size(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv=dev->dev_private;

	if (dev_priv->card_type >= NV_40)
		return 128;
	else if (dev_priv->card_type >= NV_17)
		return 64;
	else
		return 32;
}

/***********************************
 * functions doing the actual work
 ***********************************/

static int nouveau_fifo_instmem_configure(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	NV_WRITE(NV03_PFIFO_RAMHT,
			(0x03 << 24) /* search 128 */ |
			((dev_priv->ramht_bits - 9) << 16) |
			(dev_priv->ramht_offset >> 8)
			);

	NV_WRITE(NV03_PFIFO_RAMRO, dev_priv->ramro_offset>>8);

	switch(dev_priv->card_type)
	{
		case NV_40:
			switch (dev_priv->chipset) {
			case 0x47:
			case 0x49:
			case 0x4b:
				NV_WRITE(0x2230, 1);
				break;
			default:
				break;
			}
			NV_WRITE(NV40_PFIFO_RAMFC, 0x30002);
			break;
		case NV_44:
			NV_WRITE(NV40_PFIFO_RAMFC, ((nouveau_mem_fb_amount(dev)-512*1024+dev_priv->ramfc_offset)>>16) |
					(2 << 16));
			break;
		case NV_30:
		case NV_20:
		case NV_17:
			NV_WRITE(NV03_PFIFO_RAMFC, (dev_priv->ramfc_offset>>8) |
					(1 << 16) /* 64 Bytes entry*/);
			/* XXX nvidia blob set bit 18, 21,23 for nv20 & nv30 */
			break;
		case NV_11:
		case NV_10:
		case NV_04:
			NV_WRITE(NV03_PFIFO_RAMFC, dev_priv->ramfc_offset>>8);
			break;
	}

	return 0;
}

int nouveau_fifo_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;

	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) &
			~NV_PMC_ENABLE_PFIFO);
	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) |
			 NV_PMC_ENABLE_PFIFO);

	/* Enable PFIFO error reporting */
	NV_WRITE(NV03_PFIFO_INTR_0, 0xFFFFFFFF);
	NV_WRITE(NV03_PFIFO_INTR_EN_0, 0xFFFFFFFF);

	NV_WRITE(NV03_PFIFO_CACHES, 0x00000000);

	ret = nouveau_fifo_instmem_configure(dev);
	if (ret) {
		DRM_ERROR("Failed to configure instance memory\n");
		return ret;
	}

	/* FIXME remove all the stuff that's done in nouveau_fifo_alloc */

	DRM_DEBUG("Setting defaults for remaining PFIFO regs\n");

	/* All channels into PIO mode */
	NV_WRITE(NV04_PFIFO_MODE, 0x00000000);

	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000000);
	/* Channel 0 active, PIO mode */
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH1, 0x00000000);
	/* PUT and GET to 0 */
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUT, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_GET, 0x00000000);
	/* No cmdbuf object */
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_INSTANCE, 0x00000000);
	NV_WRITE(NV03_PFIFO_CACHE0_PUSH0, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE0_PULL0, 0x00000000);
	NV_WRITE(NV04_PFIFO_SIZE, 0x0000FFFF);
	NV_WRITE(NV04_PFIFO_CACHE1_HASH, 0x0000FFFF);
	NV_WRITE(NV04_PFIFO_CACHE0_PULL1, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_CTL, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_STATE, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_ENGINE, 0x00000000);

	NV_WRITE(NV04_PFIFO_CACHE1_DMA_FETCH, NV_PFIFO_CACHE1_DMA_FETCH_TRIG_112_BYTES |
				      NV_PFIFO_CACHE1_DMA_FETCH_SIZE_128_BYTES |
				      NV_PFIFO_CACHE1_DMA_FETCH_MAX_REQS_4 |
#ifdef __BIG_ENDIAN
				      NV_PFIFO_CACHE1_BIG_ENDIAN |
#endif
				      0x00000000);

	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH, 0x00000001);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL1, 0x00000001);

	/* FIXME on NV04 */
	if (dev_priv->card_type >= NV_10) {
		NV_WRITE(NV10_PGRAPH_CTX_USER, 0x0);
		NV_WRITE(NV04_PFIFO_DELAY_0, 0xff /* retrycount*/ );
		if (dev_priv->card_type >= NV_40)
			NV_WRITE(NV10_PGRAPH_CTX_CONTROL, 0x00002001);
		else
			NV_WRITE(NV10_PGRAPH_CTX_CONTROL, 0x10110000);
	} else {
		NV_WRITE(NV04_PGRAPH_CTX_USER, 0x0);
		NV_WRITE(NV04_PFIFO_DELAY_0, 0xff /* retrycount*/ );
		NV_WRITE(NV04_PGRAPH_CTX_CONTROL, 0x10110000);
	}

	NV_WRITE(NV04_PFIFO_DMA_TIMESLICE, 0x001fffff);
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000001);
	return 0;
}

static int
nouveau_fifo_pushbuf_ctxdma_init(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct mem_block *pb = chan->pushbuf_mem;
	struct nouveau_gpuobj *pushbuf = NULL;
	int ret;

	if (pb->flags & NOUVEAU_MEM_AGP) {
		ret = nouveau_gpuobj_gart_dma_new(chan, pb->start, pb->size,
						  NV_DMA_ACCESS_RO,
						  &pushbuf,
						  &chan->pushbuf_base);
	} else
	if (pb->flags & NOUVEAU_MEM_PCI) {
		ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					     pb->start, pb->size,
					     NV_DMA_ACCESS_RO,
					     NV_DMA_TARGET_PCI_NONLINEAR,
					     &pushbuf);
		chan->pushbuf_base = 0;
	} else if (dev_priv->card_type != NV_04) {
		ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					     pb->start, pb->size,
					     NV_DMA_ACCESS_RO,
					     NV_DMA_TARGET_VIDMEM, &pushbuf);
		chan->pushbuf_base = 0;
	} else {
		/* NV04 cmdbuf hack, from original ddx.. not sure of it's
		 * exact reason for existing :)  PCI access to cmdbuf in
		 * VRAM.
		 */
		ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					     pb->start +
					       drm_get_resource_start(dev, 1),
					     pb->size, NV_DMA_ACCESS_RO,
					     NV_DMA_TARGET_PCI, &pushbuf);
		chan->pushbuf_base = 0;
	}

	if ((ret = nouveau_gpuobj_ref_add(dev, chan, 0, pushbuf,
					  &chan->pushbuf))) {
		DRM_ERROR("Error referencing push buffer ctxdma: %d\n", ret);
		if (pushbuf != dev_priv->gart_info.sg_ctxdma)
			nouveau_gpuobj_del(dev, &pushbuf);
		return ret;
	}

	return 0;
}

static struct mem_block *
nouveau_fifo_user_pushbuf_alloc(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_config *config = &dev_priv->config;
	struct mem_block *pb;
	int pb_min_size = max(NV03_FIFO_SIZE,PAGE_SIZE);

	/* Defaults for unconfigured values */
	if (!config->cmdbuf.location)
		config->cmdbuf.location = NOUVEAU_MEM_FB;
	if (!config->cmdbuf.size || config->cmdbuf.size < pb_min_size)
		config->cmdbuf.size = pb_min_size;

	pb = nouveau_mem_alloc(dev, 0, config->cmdbuf.size,
			       config->cmdbuf.location | NOUVEAU_MEM_MAPPED,
			       (struct drm_file *)-2);
	if (!pb)
		DRM_ERROR("Couldn't allocate DMA push buffer.\n");

	return pb;
}

/* allocates and initializes a fifo for user space consumption */
int
nouveau_fifo_alloc(struct drm_device *dev, struct nouveau_channel **chan_ret,
		   struct drm_file *file_priv, struct mem_block *pushbuf,
		   uint32_t vram_handle, uint32_t tt_handle)
{
	int ret;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	struct nouveau_channel *chan;
	int channel;

	/*
	 * Alright, here is the full story
	 * Nvidia cards have multiple hw fifo contexts (praise them for that,
	 * no complicated crash-prone context switches)
	 * We allocate a new context for each app and let it write to it directly
	 * (woo, full userspace command submission !)
	 * When there are no more contexts, you lost
	 */
	for (channel = 0; channel < engine->fifo.channels; channel++) {
		if (dev_priv->fifos[channel] == NULL)
			break;
	}

	/* no more fifos. you lost. */
	if (channel == engine->fifo.channels)
		return -EINVAL;

	dev_priv->fifos[channel] = drm_calloc(1, sizeof(struct nouveau_channel),
					      DRM_MEM_DRIVER);
	if (!dev_priv->fifos[channel])
		return -ENOMEM;
	dev_priv->fifo_alloc_count++;
	chan = dev_priv->fifos[channel];
	chan->dev = dev;
	chan->id = channel;
	chan->file_priv = file_priv;
	chan->pushbuf_mem = pushbuf;

	DRM_INFO("Allocating FIFO number %d\n", channel);

	/* Locate channel's user control regs */
	if (dev_priv->card_type < NV_40) {
		chan->user = NV03_USER(channel);
		chan->user_size = NV03_USER_SIZE;
		chan->put = NV03_USER_DMA_PUT(channel);
		chan->get = NV03_USER_DMA_GET(channel);
		chan->ref_cnt = NV03_USER_REF_CNT(channel);
	} else
	if (dev_priv->card_type < NV_50) {
		chan->user = NV40_USER(channel);
		chan->user_size = NV40_USER_SIZE;
		chan->put = NV40_USER_DMA_PUT(channel);
		chan->get = NV40_USER_DMA_GET(channel);
		chan->ref_cnt = NV40_USER_REF_CNT(channel);
	} else {
		chan->user = NV50_USER(channel);
		chan->user_size = NV50_USER_SIZE;
		chan->put = NV50_USER_DMA_PUT(channel);
		chan->get = NV50_USER_DMA_GET(channel);
		chan->ref_cnt = NV50_USER_REF_CNT(channel);
	}

	/* Allocate space for per-channel fixed notifier memory */
	ret = nouveau_notifier_init_channel(chan);
	if (ret) {
		nouveau_fifo_free(chan);
		return ret;
	}

	/* Setup channel's default objects */
	ret = nouveau_gpuobj_channel_init(chan, vram_handle, tt_handle);
	if (ret) {
		nouveau_fifo_free(chan);
		return ret;
	}

	/* Create a dma object for the push buffer */
	ret = nouveau_fifo_pushbuf_ctxdma_init(chan);
	if (ret) {
		nouveau_fifo_free(chan);
		return ret;
	}

	nouveau_wait_for_idle(dev);

	/* disable the fifo caches */
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH, NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH)&(~0x1));
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000000);

	/* Create a graphics context for new channel */
	ret = engine->graph.create_context(chan);
	if (ret) {
		nouveau_fifo_free(chan);
		return ret;
	}

	/* Construct inital RAMFC for new channel */
	ret = engine->fifo.create_context(chan);
	if (ret) {
		nouveau_fifo_free(chan);
		return ret;
	}

	/* setup channel's default get/put values
	 * XXX: quite possibly extremely pointless..
	 */
	NV_WRITE(chan->get, chan->pushbuf_base);
	NV_WRITE(chan->put, chan->pushbuf_base);

	/* If this is the first channel, setup PFIFO ourselves.  For any
	 * other case, the GPU will handle this when it switches contexts.
	 */
	if (dev_priv->card_type < NV_50 &&
	    dev_priv->fifo_alloc_count == 1) {
		ret = engine->fifo.load_context(chan);
		if (ret) {
			nouveau_fifo_free(chan);
			return ret;
		}

		ret = engine->graph.load_context(chan);
		if (ret) {
			nouveau_fifo_free(chan);
			return ret;
		}
	}

	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH,
		 NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH) | 1);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL1, 0x00000001);

	/* reenable the fifo caches */
	NV_WRITE(NV03_PFIFO_CACHES, 1);

	DRM_INFO("%s: initialised FIFO %d\n", __func__, channel);
	*chan_ret = chan;
	return 0;
}

int
nouveau_channel_idle(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	uint32_t caches;
	int idle;

	caches = NV_READ(NV03_PFIFO_CACHES);
	NV_WRITE(NV03_PFIFO_CACHES, caches & ~1);

	if (engine->fifo.channel_id(dev) != chan->id) {
		struct nouveau_gpuobj *ramfc = chan->ramfc->gpuobj;

		if (INSTANCE_RD(ramfc, 0) != INSTANCE_RD(ramfc, 1))
			idle = 0;
		else
			idle = 1;
	} else {
		idle = (NV_READ(NV04_PFIFO_CACHE1_DMA_GET) ==
			NV_READ(NV04_PFIFO_CACHE1_DMA_PUT));
	}

	NV_WRITE(NV03_PFIFO_CACHES, caches);
	return idle;
}

/* stops a fifo */
void nouveau_fifo_free(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	uint64_t t_start;

	DRM_INFO("%s: freeing fifo %d\n", __func__, chan->id);

	/* Give the channel a chance to idle, wait 2s (hopefully) */
	t_start = engine->timer.read(dev);
	while (!nouveau_channel_idle(chan)) {
		if (engine->timer.read(dev) - t_start > 2000000000ULL) {
			DRM_ERROR("Failed to idle channel %d before destroy."
				  "Prepare for strangeness..\n", chan->id);
			break;
		}
	}

	/*XXX: Maybe should wait for PGRAPH to finish with the stuff it fetched
	 *     from CACHE1 too?
	 */

	/* disable the fifo caches */
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH, NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH)&(~0x1));
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000000);

	// FIXME XXX needs more code

	engine->fifo.destroy_context(chan);

	/* Cleanup PGRAPH state */
	engine->graph.destroy_context(chan);

	/* reenable the fifo caches */
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH,
		 NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH) | 1);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000001);
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000001);

	/* Deallocate push buffer */
	nouveau_gpuobj_ref_del(dev, &chan->pushbuf);
	if (chan->pushbuf_mem) {
		nouveau_mem_free(dev, chan->pushbuf_mem);
		chan->pushbuf_mem = NULL;
	}

	/* Destroy objects belonging to the channel */
	nouveau_gpuobj_channel_takedown(chan);

	nouveau_notifier_takedown_channel(chan);

	dev_priv->fifos[chan->id] = NULL;
	dev_priv->fifo_alloc_count--;
	drm_free(chan, sizeof(*chan), DRM_MEM_DRIVER);
}

/* cleanups all the fifos from file_priv */
void nouveau_fifo_cleanup(struct drm_device *dev, struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	int i;

	DRM_DEBUG("clearing FIFO enables from file_priv\n");
	for(i = 0; i < engine->fifo.channels; i++) {
		struct nouveau_channel *chan = dev_priv->fifos[i];

		if (chan && chan->file_priv == file_priv)
			nouveau_fifo_free(chan);
	}
}

int
nouveau_fifo_owner(struct drm_device *dev, struct drm_file *file_priv,
		   int channel)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;

	if (channel >= engine->fifo.channels)
		return 0;
	if (dev_priv->fifos[channel] == NULL)
		return 0;
	return (dev_priv->fifos[channel]->file_priv == file_priv);
}

/***********************************
 * ioctls wrapping the functions
 ***********************************/

static int nouveau_ioctl_fifo_alloc(struct drm_device *dev, void *data,
				    struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_channel_alloc *init = data;
	struct drm_map_list *entry;
	struct nouveau_channel *chan;
	struct mem_block *pushbuf;
	int res;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (init->fb_ctxdma_handle == ~0 || init->tt_ctxdma_handle == ~0)
		return -EINVAL;

	pushbuf = nouveau_fifo_user_pushbuf_alloc(dev);
	if (!pushbuf)
		return -ENOMEM;

	res = nouveau_fifo_alloc(dev, &chan, file_priv, pushbuf,
				 init->fb_ctxdma_handle,
				 init->tt_ctxdma_handle);
	if (res)
		return res;
	init->channel  = chan->id;
	init->put_base = chan->pushbuf_base;

	/* make the fifo available to user space */
	/* first, the fifo control regs */
	init->ctrl = dev_priv->mmio->offset + chan->user;
	init->ctrl_size = chan->user_size;
	res = drm_addmap(dev, init->ctrl, init->ctrl_size, _DRM_REGISTERS,
			 0, &chan->regs);
	if (res != 0)
		return res;

	entry = drm_find_matching_map(dev, chan->regs);
	if (!entry)
		return -EINVAL;
	init->ctrl = entry->user_token;

	/* pass back FIFO map info to the caller */
	init->cmdbuf      = chan->pushbuf_mem->map_handle;
	init->cmdbuf_size = chan->pushbuf_mem->size;

	/* and the notifier block */
	init->notifier      = chan->notifier_block->map_handle;
	init->notifier_size = chan->notifier_block->size;

	return 0;
}

static int nouveau_ioctl_fifo_free(struct drm_device *dev, void *data,
				   struct drm_file *file_priv)
{
	struct drm_nouveau_channel_free *cfree = data;
	struct nouveau_channel *chan;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;
	NOUVEAU_GET_USER_CHANNEL_WITH_RETURN(cfree->channel, file_priv, chan);

	nouveau_fifo_free(chan);
	return 0;
}

/***********************************
 * finally, the ioctl table
 ***********************************/

struct drm_ioctl_desc nouveau_ioctls[] = {
	DRM_IOCTL_DEF(DRM_NOUVEAU_CARD_INIT, nouveau_ioctl_card_init, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_GETPARAM, nouveau_ioctl_getparam, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_SETPARAM, nouveau_ioctl_setparam, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_NOUVEAU_CHANNEL_ALLOC, nouveau_ioctl_fifo_alloc, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_CHANNEL_FREE, nouveau_ioctl_fifo_free, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_GROBJ_ALLOC, nouveau_ioctl_grobj_alloc, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_NOTIFIEROBJ_ALLOC, nouveau_ioctl_notifier_alloc, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_GPUOBJ_FREE, nouveau_ioctl_gpuobj_free, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_MEM_ALLOC, nouveau_ioctl_mem_alloc, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_MEM_FREE, nouveau_ioctl_mem_free, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_MEM_TILE, nouveau_ioctl_mem_tile, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_SUSPEND, nouveau_ioctl_suspend, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_NOUVEAU_RESUME, nouveau_ioctl_resume, DRM_AUTH),
};

int nouveau_max_ioctl = DRM_ARRAY_SIZE(nouveau_ioctls);
