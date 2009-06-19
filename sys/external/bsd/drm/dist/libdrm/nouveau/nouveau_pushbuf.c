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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nouveau_private.h"
#include "nouveau_dma.h"

#define PB_BUFMGR_DWORDS   (4096 / 2)
#define PB_MIN_USER_DWORDS  2048

static uint32_t
nouveau_pushbuf_calc_reloc(struct drm_nouveau_gem_pushbuf_bo *pbbo,
			   struct drm_nouveau_gem_pushbuf_reloc *r,
			   int mm_enabled)
{
	uint32_t push = 0;
	const unsigned is_vram = mm_enabled ? NOUVEAU_GEM_DOMAIN_VRAM :
					      NOUVEAU_BO_VRAM;

	if (r->flags & NOUVEAU_GEM_RELOC_LOW)
		push = (pbbo->presumed_offset + r->data);
	else
	if (r->flags & NOUVEAU_GEM_RELOC_HIGH)
		push = (pbbo->presumed_offset + r->data) >> 32;
	else
		push = r->data;

	if (r->flags & NOUVEAU_GEM_RELOC_OR) {
		if (pbbo->presumed_domain & is_vram)
			push |= r->vor;
		else
			push |= r->tor;
	}

	return push;
}

int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *chan, void *ptr,
			   struct nouveau_bo *bo, uint32_t data, uint32_t flags,
			   uint32_t vor, uint32_t tor)
{
	struct nouveau_device_priv *nvdev = nouveau_device(chan->device);
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(chan->pushbuf);
	struct drm_nouveau_gem_pushbuf_reloc *r;
	struct drm_nouveau_gem_pushbuf_bo *pbbo;
	uint32_t domains = 0;

	if (nvpb->nr_relocs >= NOUVEAU_PUSHBUF_MAX_RELOCS)
		return -ENOMEM;

	if (nouveau_bo(bo)->user && (flags & NOUVEAU_BO_WR)) {
		fprintf(stderr, "write to user buffer!!\n");
		return -EINVAL;
	}

	pbbo = nouveau_bo_emit_buffer(chan, bo);
	if (!pbbo)
		return -ENOMEM;

	if (flags & NOUVEAU_BO_VRAM)
		domains |= NOUVEAU_GEM_DOMAIN_VRAM;
	if (flags & NOUVEAU_BO_GART)
		domains |= NOUVEAU_GEM_DOMAIN_GART;
	pbbo->valid_domains &= domains;
	assert(pbbo->valid_domains);

	if (!nvdev->mm_enabled) {
		struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

		nouveau_fence_ref(nvpb->fence, &nvbo->fence);
		if (flags & NOUVEAU_BO_WR)
			nouveau_fence_ref(nvpb->fence, &nvbo->wr_fence);
	}

	assert(flags & NOUVEAU_BO_RDWR);
	if (flags & NOUVEAU_BO_RD) {
		pbbo->read_domains |= domains;
	}
	if (flags & NOUVEAU_BO_WR) {
		pbbo->write_domains |= domains;
		nouveau_bo(bo)->write_marker = 1;
	}

	r = nvpb->relocs + nvpb->nr_relocs++;
	r->bo_index = pbbo - nvpb->buffers;
	r->reloc_index = (uint32_t *)ptr - nvpb->pushbuf;
	r->flags = 0;
	if (flags & NOUVEAU_BO_LOW)
		r->flags |= NOUVEAU_GEM_RELOC_LOW;
	if (flags & NOUVEAU_BO_HIGH)
		r->flags |= NOUVEAU_GEM_RELOC_HIGH;
	if (flags & NOUVEAU_BO_OR)
		r->flags |= NOUVEAU_GEM_RELOC_OR;
	r->data = data;
	r->vor = vor;
	r->tor = tor;

	*(uint32_t *)ptr = (flags & NOUVEAU_BO_DUMMY) ? 0 :
		nouveau_pushbuf_calc_reloc(pbbo, r, nvdev->mm_enabled);
	return 0;
}

static int
nouveau_pushbuf_space(struct nouveau_channel *chan, unsigned min)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;

	if (nvpb->pushbuf) {
		free(nvpb->pushbuf);
		nvpb->pushbuf = NULL;
	}

	nvpb->size = min < PB_MIN_USER_DWORDS ? PB_MIN_USER_DWORDS : min;	
	nvpb->pushbuf = malloc(sizeof(uint32_t) * nvpb->size);

	nvpb->base.channel = chan;
	nvpb->base.remaining = nvpb->size;
	nvpb->base.cur = nvpb->pushbuf;
	
	if (!nouveau_device(chan->device)->mm_enabled) {
		nouveau_fence_ref(NULL, &nvpb->fence);
		nouveau_fence_new(chan, &nvpb->fence);
	}

	return 0;
}

int
nouveau_pushbuf_init(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;

	nouveau_pushbuf_space(chan, 0);

	nvpb->buffers = calloc(NOUVEAU_PUSHBUF_MAX_BUFFERS,
			       sizeof(struct drm_nouveau_gem_pushbuf_bo));
	nvpb->relocs = calloc(NOUVEAU_PUSHBUF_MAX_RELOCS,
			      sizeof(struct drm_nouveau_gem_pushbuf_reloc));
	
	chan->pushbuf = &nvpb->base;
	return 0;
}

static int
nouveau_pushbuf_flush_nomm(struct nouveau_channel_priv *nvchan)
{
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	struct drm_nouveau_gem_pushbuf_bo *bo = nvpb->buffers;
	struct drm_nouveau_gem_pushbuf_reloc *reloc = nvpb->relocs;
	unsigned b, r;
	int ret;

	for (b = 0; b < nvpb->nr_buffers; b++) {
		struct nouveau_bo_priv *nvbo =
			(void *)(unsigned long)bo[b].user_priv;
		uint32_t flags = 0;

		if (bo[b].valid_domains & NOUVEAU_GEM_DOMAIN_VRAM)
			flags |= NOUVEAU_BO_VRAM;
		if (bo[b].valid_domains & NOUVEAU_GEM_DOMAIN_GART)
			flags |= NOUVEAU_BO_GART;

		ret = nouveau_bo_validate_nomm(nvbo, flags);
		if (ret)
			return ret;

		if (1 || bo[b].presumed_domain != nvbo->domain ||
		    bo[b].presumed_offset != nvbo->offset) {
			bo[b].presumed_ok = 0;
			bo[b].presumed_domain = nvbo->domain;
			bo[b].presumed_offset = nvbo->offset;
		}
	}

	for (r = 0; r < nvpb->nr_relocs; r++, reloc++) {
		uint32_t push;

		if (bo[reloc->bo_index].presumed_ok)
			continue;

		push = nouveau_pushbuf_calc_reloc(&bo[reloc->bo_index], reloc, 0);
		nvpb->pushbuf[reloc->reloc_index] = push;
	}

	nouveau_dma_space(&nvchan->base, nvpb->size);
	nouveau_dma_outp (&nvchan->base, nvpb->pushbuf, nvpb->size);
	nouveau_fence_emit(nvpb->fence);

	return 0;
}

int
nouveau_pushbuf_flush(struct nouveau_channel *chan, unsigned min)
{
	struct nouveau_device_priv *nvdev = nouveau_device(chan->device);
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	struct drm_nouveau_gem_pushbuf req;
	unsigned i;
	int ret;

	if (nvpb->base.remaining == nvpb->size)
		return 0;
	nvpb->size -= nvpb->base.remaining;

	if (nvdev->mm_enabled) {
		req.channel = chan->id;
		req.nr_dwords = nvpb->size;
		req.dwords = (uint64_t)(unsigned long)nvpb->pushbuf;
		req.nr_buffers = nvpb->nr_buffers;
		req.buffers = (uint64_t)(unsigned long)nvpb->buffers;
		req.nr_relocs = nvpb->nr_relocs;
		req.relocs = (uint64_t)(unsigned long)nvpb->relocs;
		ret = drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_PUSHBUF,
				      &req, sizeof(req));
	} else {
		nouveau_fence_flush(chan);
		ret = nouveau_pushbuf_flush_nomm(nvchan);
	}
	assert(ret == 0);


	/* Update presumed offset/domain for any buffers that moved.
	 * Dereference all buffers on validate list
	 */
	for (i = 0; i < nvpb->nr_buffers; i++) {
		struct drm_nouveau_gem_pushbuf_bo *pbbo = &nvpb->buffers[i];
		struct nouveau_bo *bo = (void *)(unsigned long)pbbo->user_priv;

		if (pbbo->presumed_ok == 0) {
			nouveau_bo(bo)->domain = pbbo->presumed_domain;
			nouveau_bo(bo)->offset = pbbo->presumed_offset;
		}

		nouveau_bo(bo)->pending = NULL;
		nouveau_bo_ref(NULL, &bo);
	}
	nvpb->nr_buffers = 0;
	nvpb->nr_relocs = 0;

	/* Allocate space for next push buffer */
	ret = nouveau_pushbuf_space(chan, min);
	assert(!ret);

	if (chan->flush_notify)
		chan->flush_notify(chan);

	return 0;
}

