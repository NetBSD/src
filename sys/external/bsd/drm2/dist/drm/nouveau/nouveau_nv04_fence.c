/*	$NetBSD: nouveau_nv04_fence.c,v 1.1.1.1 2014/08/06 12:36:23 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_nv04_fence.c,v 1.1.1.1 2014/08/06 12:36:23 riastradh Exp $");

#include <engine/fifo.h>

#include "nouveau_drm.h"
#include "nouveau_dma.h"
#include "nouveau_fence.h"

struct nv04_fence_chan {
	struct nouveau_fence_chan base;
};

struct nv04_fence_priv {
	struct nouveau_fence_priv base;
};

static int
nv04_fence_emit(struct nouveau_fence *fence)
{
	struct nouveau_channel *chan = fence->channel;
	int ret = RING_SPACE(chan, 2);
	if (ret == 0) {
		BEGIN_NV04(chan, NvSubSw, 0x0150, 1);
		OUT_RING  (chan, fence->sequence);
		FIRE_RING (chan);
	}
	return ret;
}

static int
nv04_fence_sync(struct nouveau_fence *fence,
		struct nouveau_channel *prev, struct nouveau_channel *chan)
{
	return -ENODEV;
}

static u32
nv04_fence_read(struct nouveau_channel *chan)
{
	struct nouveau_fifo_chan *fifo = (void *)chan->object;
	return atomic_read(&fifo->refcnt);
}

static void
nv04_fence_context_del(struct nouveau_channel *chan)
{
	struct nv04_fence_chan *fctx = chan->fence;
	nouveau_fence_context_del(&fctx->base);
	chan->fence = NULL;
	kfree(fctx);
}

static int
nv04_fence_context_new(struct nouveau_channel *chan)
{
	struct nv04_fence_chan *fctx = kzalloc(sizeof(*fctx), GFP_KERNEL);
	if (fctx) {
		nouveau_fence_context_new(&fctx->base);
		fctx->base.emit = nv04_fence_emit;
		fctx->base.sync = nv04_fence_sync;
		fctx->base.read = nv04_fence_read;
		chan->fence = fctx;
		return 0;
	}
	return -ENOMEM;
}

static void
nv04_fence_destroy(struct nouveau_drm *drm)
{
	struct nv04_fence_priv *priv = drm->fence;
	drm->fence = NULL;
	kfree(priv);
}

int
nv04_fence_create(struct nouveau_drm *drm)
{
	struct nv04_fence_priv *priv;

	priv = drm->fence = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base.dtor = nv04_fence_destroy;
	priv->base.context_new = nv04_fence_context_new;
	priv->base.context_del = nv04_fence_context_del;
	return 0;
}
