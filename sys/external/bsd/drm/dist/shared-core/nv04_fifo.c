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

#define RAMFC_WR(offset,val) INSTANCE_WR(chan->ramfc->gpuobj, \
					 NV04_RAMFC_##offset/4, (val))
#define RAMFC_RD(offset)     INSTANCE_RD(chan->ramfc->gpuobj, \
					 NV04_RAMFC_##offset/4)
#define NV04_RAMFC(c) (dev_priv->ramfc_offset + ((c) * NV04_RAMFC__SIZE))
#define NV04_RAMFC__SIZE 32

int
nv04_fifo_channel_id(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	return (NV_READ(NV03_PFIFO_CACHE1_PUSH1) &
			NV03_PFIFO_CACHE1_PUSH1_CHID_MASK);
}

int
nv04_fifo_create_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;

	if ((ret = nouveau_gpuobj_new_fake(dev, NV04_RAMFC(chan->id), ~0,
						NV04_RAMFC__SIZE,
						NVOBJ_FLAG_ZERO_ALLOC |
						NVOBJ_FLAG_ZERO_FREE,
						NULL, &chan->ramfc)))
		return ret;

	/* Setup initial state */
	RAMFC_WR(DMA_PUT, chan->pushbuf_base);
	RAMFC_WR(DMA_GET, chan->pushbuf_base);
	RAMFC_WR(DMA_INSTANCE, chan->pushbuf->instance >> 4);
	RAMFC_WR(DMA_FETCH, (NV_PFIFO_CACHE1_DMA_FETCH_TRIG_128_BYTES |
			     NV_PFIFO_CACHE1_DMA_FETCH_SIZE_128_BYTES |
			     NV_PFIFO_CACHE1_DMA_FETCH_MAX_REQS_8 |
#ifdef __BIG_ENDIAN
			     NV_PFIFO_CACHE1_BIG_ENDIAN |
#endif
			     0));

	/* enable the fifo dma operation */
	NV_WRITE(NV04_PFIFO_MODE,NV_READ(NV04_PFIFO_MODE) | (1<<chan->id));
	return 0;
}

void
nv04_fifo_destroy_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	NV_WRITE(NV04_PFIFO_MODE, NV_READ(NV04_PFIFO_MODE)&~(1<<chan->id));

	nouveau_gpuobj_ref_del(dev, &chan->ramfc);
}

int
nv04_fifo_load_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t tmp;

	NV_WRITE(NV03_PFIFO_CACHE1_PUSH1,
		 NV03_PFIFO_CACHE1_PUSH1_DMA | chan->id);

	NV_WRITE(NV04_PFIFO_CACHE1_DMA_GET, RAMFC_RD(DMA_GET));
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUT, RAMFC_RD(DMA_PUT));

	tmp = RAMFC_RD(DMA_INSTANCE);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_INSTANCE, tmp & 0xFFFF);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_DCOUNT, tmp >> 16);

	NV_WRITE(NV04_PFIFO_CACHE1_DMA_STATE, RAMFC_RD(DMA_STATE));
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_FETCH, RAMFC_RD(DMA_FETCH));
	NV_WRITE(NV04_PFIFO_CACHE1_ENGINE, RAMFC_RD(ENGINE));
	NV_WRITE(NV04_PFIFO_CACHE1_PULL1, RAMFC_RD(PULL1_ENGINE));

	/* Reset NV04_PFIFO_CACHE1_DMA_CTL_AT_INFO to INVALID */
	tmp = NV_READ(NV04_PFIFO_CACHE1_DMA_CTL) & ~(1<<31);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_CTL, tmp);

	return 0;
}

int
nv04_fifo_save_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t tmp;

	RAMFC_WR(DMA_PUT, NV_READ(NV04_PFIFO_CACHE1_DMA_PUT));
	RAMFC_WR(DMA_GET, NV_READ(NV04_PFIFO_CACHE1_DMA_GET));

	tmp  = NV_READ(NV04_PFIFO_CACHE1_DMA_DCOUNT) << 16;
	tmp |= NV_READ(NV04_PFIFO_CACHE1_DMA_INSTANCE);
	RAMFC_WR(DMA_INSTANCE, tmp);

	RAMFC_WR(DMA_STATE, NV_READ(NV04_PFIFO_CACHE1_DMA_STATE));
	RAMFC_WR(DMA_FETCH, NV_READ(NV04_PFIFO_CACHE1_DMA_FETCH));
	RAMFC_WR(ENGINE, NV_READ(NV04_PFIFO_CACHE1_ENGINE));
	RAMFC_WR(PULL1_ENGINE, NV_READ(NV04_PFIFO_CACHE1_PULL1));

	return 0;
}
