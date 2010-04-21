/*
 * Copyright 2007 Nouveau Project
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "nouveau_private.h"

int
nouveau_channel_alloc(struct nouveau_device *dev, uint32_t fb_ctxdma,
		      uint32_t tt_ctxdma, struct nouveau_channel **chan)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	struct nouveau_channel_priv *nvchan;
	unsigned i;
	int ret;

	if (!nvdev || !chan || *chan)
	    return -EINVAL;

	nvchan = calloc(1, sizeof(struct nouveau_channel_priv));
	if (!nvchan)
		return -ENOMEM;
	nvchan->base.device = dev;

	nvchan->drm.fb_ctxdma_handle = fb_ctxdma;
	nvchan->drm.tt_ctxdma_handle = tt_ctxdma;
	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_CHANNEL_ALLOC,
				  &nvchan->drm, sizeof(nvchan->drm));
	if (ret) {
		free(nvchan);
		return ret;
	}

	nvchan->base.id = nvchan->drm.channel;
	if (nouveau_grobj_ref(&nvchan->base, nvchan->drm.fb_ctxdma_handle,
			      &nvchan->base.vram) ||
	    nouveau_grobj_ref(&nvchan->base, nvchan->drm.tt_ctxdma_handle,
		    	      &nvchan->base.gart)) {
		nouveau_channel_free((void *)&nvchan);
		return -EINVAL;
	}

	/* Mark all DRM-assigned subchannels as in-use */
	for (i = 0; i < nvchan->drm.nr_subchan; i++) {
		struct nouveau_grobj_priv *gr = calloc(1, sizeof(*gr));

		gr->base.bound = NOUVEAU_GROBJ_BOUND_EXPLICIT;
		gr->base.subc = i;
		gr->base.handle = nvchan->drm.subchan[i].handle;
		gr->base.grclass = nvchan->drm.subchan[i].grclass;
		gr->base.channel = &nvchan->base;

		nvchan->base.subc[i].gr = &gr->base;
	}

	ret = drmMap(nvdev->fd, nvchan->drm.notifier, nvchan->drm.notifier_size,
		     (drmAddressPtr)&nvchan->notifier_block);
	if (ret) {
		nouveau_channel_free((void *)&nvchan);
		return ret;
	}

	ret = nouveau_grobj_alloc(&nvchan->base, 0x00000000, 0x0030,
				  &nvchan->base.nullobj);
	if (ret) {
		nouveau_channel_free((void *)&nvchan);
		return ret;
	}

	if (!nvdev->mm_enabled) {
		ret = drmMap(nvdev->fd, nvchan->drm.ctrl, nvchan->drm.ctrl_size,
			     (void*)&nvchan->user);
		if (ret) {
			nouveau_channel_free((void *)&nvchan);
			return ret;
		}
		nvchan->put     = &nvchan->user[0x40/4];
		nvchan->get     = &nvchan->user[0x44/4];
		nvchan->ref_cnt = &nvchan->user[0x48/4];

		ret = drmMap(nvdev->fd, nvchan->drm.cmdbuf,
			     nvchan->drm.cmdbuf_size, (void*)&nvchan->pushbuf);
		if (ret) {
			nouveau_channel_free((void *)&nvchan);
			return ret;
		}

		nouveau_dma_channel_init(&nvchan->base);
	}

	nouveau_pushbuf_init(&nvchan->base);

	if (!nvdev->mm_enabled && dev->chipset < 0x10) {
		ret = nouveau_grobj_alloc(&nvchan->base, 0xbeef3904, 0x5039,
					  &nvchan->fence_grobj);
		if (ret) {
			nouveau_channel_free((void *)&nvchan);
			return ret;
		}

		ret = nouveau_notifier_alloc(&nvchan->base, 0xbeef3905, 1,
					     &nvchan->fence_ntfy);
		if (ret) {
			nouveau_channel_free((void *)&nvchan);
			return ret;
		}

		BEGIN_RING(&nvchan->base, nvchan->fence_grobj, 0x0180, 1);
		OUT_RING  (&nvchan->base, nvchan->fence_ntfy->handle);
		nvchan->fence_grobj->bound = NOUVEAU_GROBJ_BOUND_EXPLICIT;
	}

	*chan = &nvchan->base;
	return 0;
}

void
nouveau_channel_free(struct nouveau_channel **chan)
{
	struct nouveau_channel_priv *nvchan;
	struct nouveau_device_priv *nvdev;
	struct drm_nouveau_channel_free cf;

	if (!chan || !*chan)
		return;
	nvchan = nouveau_channel(*chan);
	*chan = NULL;
	nvdev = nouveau_device(nvchan->base.device);
	
	FIRE_RING(&nvchan->base);

	if (!nvdev->mm_enabled) {
		struct nouveau_fence *fence = NULL;

		/* Make sure all buffer objects on delayed delete queue
		 * actually get freed.
		 */
		nouveau_fence_new(&nvchan->base, &fence);
		nouveau_fence_emit(fence);
		nouveau_fence_wait(&fence);
	}

	if (nvchan->notifier_block)
		drmUnmap(nvchan->notifier_block, nvchan->drm.notifier_size);

	nouveau_grobj_free(&nvchan->base.vram);
	nouveau_grobj_free(&nvchan->base.gart);
	nouveau_grobj_free(&nvchan->base.nullobj);
	nouveau_grobj_free(&nvchan->fence_grobj);
	nouveau_notifier_free(&nvchan->fence_ntfy);

	cf.channel = nvchan->drm.channel;
	drmCommandWrite(nvdev->fd, DRM_NOUVEAU_CHANNEL_FREE, &cf, sizeof(cf));
	free(nvchan);
}


