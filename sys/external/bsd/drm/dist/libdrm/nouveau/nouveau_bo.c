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

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include "nouveau_private.h"

int
nouveau_bo_init(struct nouveau_device *dev)
{
	return 0;
}

void
nouveau_bo_takedown(struct nouveau_device *dev)
{
}

static int
nouveau_bo_allocated(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->sysmem || nvbo->handle || (nvbo->flags & NOUVEAU_BO_PIN))
		return 1;
	return 0;
}

static int
nouveau_bo_ualloc(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->user || nvbo->sysmem) {
		assert(nvbo->sysmem);
		return 0;
	}

	nvbo->sysmem = malloc(nvbo->size);
	if (!nvbo->sysmem)
		return -ENOMEM;

	return 0;
}

static void
nouveau_bo_ufree(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->sysmem) {
		if (!nvbo->user)
			free(nvbo->sysmem);
		nvbo->sysmem = NULL;
	}
}

static void
nouveau_bo_kfree_nomm(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_nouveau_mem_free req;

	if (nvbo->map) {
		drmUnmap(nvbo->map, nvbo->size);
		nvbo->map = NULL;
	}

	req.offset = nvbo->offset;
	if (nvbo->domain & NOUVEAU_BO_GART)
		req.flags = NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI;
	else
	if (nvbo->domain & NOUVEAU_BO_VRAM)
		req.flags = NOUVEAU_MEM_FB;
	drmCommandWrite(nvdev->fd, DRM_NOUVEAU_MEM_FREE, &req, sizeof(req));

	nvbo->handle = 0;
}

static void
nouveau_bo_kfree(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_gem_close req;

	if (!nvbo->handle)
		return;

	if (!nvdev->mm_enabled) {
		nouveau_bo_kfree_nomm(nvbo);
		return;
	}

	if (nvbo->map) {
		munmap(nvbo->map, nvbo->size);
		nvbo->map = NULL;
	}

	req.handle = nvbo->handle;
	nvbo->handle = 0;
	ioctl(nvdev->fd, DRM_IOCTL_GEM_CLOSE, &req);
}

static int
nouveau_bo_kalloc_nomm(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_nouveau_mem_alloc req;
	int ret;

	if (nvbo->handle)
		return 0;

	if (!(nvbo->flags & (NOUVEAU_BO_VRAM|NOUVEAU_BO_GART)))
		nvbo->flags |= (NOUVEAU_BO_GART | NOUVEAU_BO_VRAM);

	req.size = nvbo->size;
	req.alignment = nvbo->align;
	req.flags = 0;
	if (nvbo->flags & NOUVEAU_BO_VRAM)
		req.flags |= NOUVEAU_MEM_FB;
	if (nvbo->flags & NOUVEAU_BO_GART)
		req.flags |= (NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI);
	if (nvbo->flags & NOUVEAU_BO_TILED) {
		req.flags |= NOUVEAU_MEM_TILE;
		if (nvbo->flags & NOUVEAU_BO_ZTILE)
			req.flags |= NOUVEAU_MEM_TILE_ZETA;
	}
	req.flags |= NOUVEAU_MEM_MAPPED;

	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_MEM_ALLOC,
				  &req, sizeof(req));
	if (ret)
		return ret;

	nvbo->handle = req.map_handle;
	nvbo->size = req.size;
	nvbo->offset = req.offset;
	if (req.flags & (NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI))
		nvbo->domain = NOUVEAU_BO_GART;
	else
	if (req.flags & NOUVEAU_MEM_FB)
		nvbo->domain = NOUVEAU_BO_VRAM;

	return 0;
}

static int
nouveau_bo_kalloc(struct nouveau_bo_priv *nvbo, struct nouveau_channel *chan)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_nouveau_gem_new req;
	int ret;

	if (nvbo->handle || (nvbo->flags & NOUVEAU_BO_PIN))
		return 0;

	if (!nvdev->mm_enabled)
		return nouveau_bo_kalloc_nomm(nvbo);

	req.channel_hint = chan ? chan->id : 0;

	req.size = nvbo->size;
	req.align = nvbo->align;

	req.domain = 0;

	if (nvbo->flags & NOUVEAU_BO_VRAM)
		req.domain |= NOUVEAU_GEM_DOMAIN_VRAM;

	if (nvbo->flags & NOUVEAU_BO_GART)
		req.domain |= NOUVEAU_GEM_DOMAIN_GART;

	if (nvbo->flags & NOUVEAU_BO_TILED) {
		req.domain |= NOUVEAU_GEM_DOMAIN_TILE;
		if (nvbo->flags & NOUVEAU_BO_ZTILE)
			req.domain |= NOUVEAU_GEM_DOMAIN_TILE_ZETA;
	}

	if (!req.domain) {
		req.domain |= (NOUVEAU_GEM_DOMAIN_VRAM |
			       NOUVEAU_GEM_DOMAIN_GART);
	}

	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_GEM_NEW,
				  &req, sizeof(req));
	if (ret)
		return ret;
	nvbo->handle = nvbo->base.handle = req.handle;
	nvbo->size = req.size;
	nvbo->domain = req.domain;
	nvbo->offset = req.offset;

	return 0;
}

static int
nouveau_bo_kmap_nomm(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	int ret;

	ret = drmMap(nvdev->fd, nvbo->handle, nvbo->size, &nvbo->map);
	if (ret) {
		nvbo->map = NULL;
		return ret;
	}

	return 0;
}

static int
nouveau_bo_kmap(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_nouveau_gem_mmap req;
	int ret;

	if (nvbo->map)
		return 0;

	if (!nvbo->handle)
		return -EINVAL;

	if (!nvdev->mm_enabled)
		return nouveau_bo_kmap_nomm(nvbo);

	req.handle = nvbo->handle;
	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_GEM_MMAP,
				  &req, sizeof(req));
	if (ret)
		return ret;

	nvbo->map = (void *)(unsigned long)req.vaddr;
	return 0;
}

int
nouveau_bo_new(struct nouveau_device *dev, uint32_t flags, int align,
	       int size, struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;
	int ret;

	if (!dev || !bo || *bo)
		return -EINVAL;

	nvbo = calloc(1, sizeof(struct nouveau_bo_priv));
	if (!nvbo)
		return -ENOMEM;
	nvbo->base.device = dev;
	nvbo->base.size = size;

	nvbo->refcount = 1;
	/* Don't set NOUVEAU_BO_PIN here, or nouveau_bo_allocated() will
	 * decided the buffer's already allocated when it's not.  The
	 * call to nouveau_bo_pin() later will set this flag.
	 */
	nvbo->flags = (flags & ~NOUVEAU_BO_PIN);
	nvbo->size = size;
	nvbo->align = align;

	/*XXX: murder me violently */
	if (flags & NOUVEAU_BO_TILED) {
		nvbo->base.tiled = 1;
		if (flags & NOUVEAU_BO_ZTILE)
			nvbo->base.tiled |= 2;
	}

	if (flags & NOUVEAU_BO_PIN) {
		ret = nouveau_bo_pin((void *)nvbo, nvbo->flags);
		if (ret) {
			nouveau_bo_ref(NULL, (void *)nvbo);
			return ret;
		}
	}

	*bo = &nvbo->base;
	return 0;
}

int
nouveau_bo_user(struct nouveau_device *dev, void *ptr, int size,
		struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;
	int ret;

	ret = nouveau_bo_new(dev, 0, 0, size, bo);
	if (ret)
		return ret;
	nvbo = nouveau_bo(*bo);

	nvbo->sysmem = ptr;
	nvbo->user = 1;
	return 0;
}

int
nouveau_bo_fake(struct nouveau_device *dev, uint64_t offset, uint32_t flags,
		uint32_t size, void *map, struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;
	int ret;

	ret = nouveau_bo_new(dev, flags & ~NOUVEAU_BO_PIN, 0, size, bo);
	if (ret)
		return ret;
	nvbo = nouveau_bo(*bo);

	nvbo->flags = flags | NOUVEAU_BO_PIN;
	nvbo->domain = (flags & (NOUVEAU_BO_VRAM|NOUVEAU_BO_GART));
	nvbo->offset = offset;
	nvbo->size = nvbo->base.size = size;
	nvbo->map = map;
	nvbo->base.flags = nvbo->flags;
	nvbo->base.offset = nvbo->offset;
	return 0;
}

int
nouveau_bo_handle_get(struct nouveau_bo *bo, uint32_t *handle)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;
 
	if (!bo || !handle)
		return -EINVAL;

	if (!nvbo->global_handle) {
		struct drm_gem_flink req;
 
		ret = nouveau_bo_kalloc(nvbo, NULL);
		if (ret)
			return ret;

		if (nvdev->mm_enabled) {
			req.handle = nvbo->handle;
			ret = ioctl(nvdev->fd, DRM_IOCTL_GEM_FLINK, &req);
			if (ret) {
				nouveau_bo_kfree(nvbo);
				return ret;
			}
	 
			nvbo->global_handle = req.name;
		} else {
			nvbo->global_handle = nvbo->offset;
		}
	}
 
	*handle = nvbo->global_handle;
	return 0;
}
 
int
nouveau_bo_handle_ref(struct nouveau_device *dev, uint32_t handle,
		      struct nouveau_bo **bo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	struct nouveau_bo_priv *nvbo;
	struct drm_gem_open req;
	int ret;

	ret = nouveau_bo_new(dev, 0, 0, 0, bo);
	if (ret)
		return ret;
	nvbo = nouveau_bo(*bo);

	if (!nvdev->mm_enabled) {
		nvbo->handle = 0;
		nvbo->offset =  handle;
		nvbo->domain = NOUVEAU_BO_VRAM;
		nvbo->flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_PIN;
		nvbo->base.offset = nvbo->offset;
		nvbo->base.flags = nvbo->flags;
	} else {
		req.name = handle;
		ret = ioctl(nvdev->fd, DRM_IOCTL_GEM_OPEN, &req);
		if (ret) {
			nouveau_bo_ref(NULL, bo);
			return ret;
		}

		nvbo->size = req.size;
		nvbo->handle = req.handle;
	}

	nvbo->base.handle = nvbo->handle;
	return 0;
} 

static void
nouveau_bo_del_cb(void *priv)
{
	struct nouveau_bo_priv *nvbo = priv;

	nouveau_fence_ref(NULL, &nvbo->fence);
	nouveau_fence_ref(NULL, &nvbo->wr_fence);
	nouveau_bo_kfree(nvbo);
	free(nvbo);
}

static void
nouveau_bo_del(struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;

	if (!bo || !*bo)
		return;
	nvbo = nouveau_bo(*bo);
	*bo = NULL;

	if (--nvbo->refcount)
		return;

	if (nvbo->pending) {
		nvbo->pending = NULL;
		nouveau_pushbuf_flush(nvbo->pending_channel, 0);
	}

	nouveau_bo_ufree(nvbo);

	if (!nouveau_device(nvbo->base.device)->mm_enabled && nvbo->fence) {
		nouveau_fence_flush(nvbo->fence->channel);
		if (nouveau_fence(nvbo->fence)->signalled) {
			nouveau_bo_del_cb(nvbo);
		} else {
			nouveau_fence_signal_cb(nvbo->fence,
					        nouveau_bo_del_cb, nvbo);
		}
	} else {
		nouveau_bo_del_cb(nvbo);
	}
}

int
nouveau_bo_ref(struct nouveau_bo *ref, struct nouveau_bo **pbo)
{
	if (!pbo)
		return -EINVAL;

	if (ref)
		nouveau_bo(ref)->refcount++;

	if (*pbo)
		nouveau_bo_del(pbo);

	*pbo = ref;
	return 0;
}

static int
nouveau_bo_wait_nomm(struct nouveau_bo *bo, int cpu_write)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret = 0;

	if (cpu_write)
		ret = nouveau_fence_wait(&nvbo->fence);
	else
		ret = nouveau_fence_wait(&nvbo->wr_fence);
	if (ret)
		return ret;

	nvbo->write_marker = 0;
	return 0;
}

static int
nouveau_bo_wait(struct nouveau_bo *bo, int cpu_write)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_cpu_prep req;
	int ret;

	if (!nvbo->global_handle && !nvbo->write_marker && !cpu_write)
		return 0;

	if (nvbo->pending &&
	    (nvbo->pending->write_domains || cpu_write)) {
		nvbo->pending = NULL;
		nouveau_pushbuf_flush(nvbo->pending_channel, 0);
	}

	if (!nvdev->mm_enabled)
		return nouveau_bo_wait_nomm(bo, cpu_write);

	req.handle = nvbo->handle;
	ret = drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_CPU_PREP,
			      &req, sizeof(req));
	if (ret)
		return ret;

	nvbo->write_marker = 0;
	return 0;
}

int
nouveau_bo_map(struct nouveau_bo *bo, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;

	if (!nvbo || bo->map)
		return -EINVAL;

	if (!nouveau_bo_allocated(nvbo)) {
		if (nvbo->flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)) {
			ret = nouveau_bo_kalloc(nvbo, NULL);
			if (ret)
				return ret;
		}

		if (!nouveau_bo_allocated(nvbo)) {
			ret = nouveau_bo_ualloc(nvbo);
			if (ret)
				return ret;
		}
	}

	if (nvbo->sysmem) {
		bo->map = nvbo->sysmem;
	} else {
		ret = nouveau_bo_kmap(nvbo);
		if (ret)
			return ret;

		ret = nouveau_bo_wait(bo, (flags & NOUVEAU_BO_WR));
		if (ret)
			return ret;

		bo->map = nvbo->map;
	}

	return 0;
}

void
nouveau_bo_unmap(struct nouveau_bo *bo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (nvdev->mm_enabled && bo->map && !nvbo->sysmem) {
		struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
		struct drm_nouveau_gem_cpu_fini req;

		req.handle = nvbo->handle;
		drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_CPU_FINI,
				&req, sizeof(req));
	}

	bo->map = NULL;
}

int
nouveau_bo_validate_nomm(struct nouveau_bo_priv *nvbo, uint32_t flags)
{
	struct nouveau_bo *new = NULL;
	uint32_t t_handle, t_domain, t_offset, t_size;
	void *t_map;
	int ret;

	if ((flags & NOUVEAU_BO_VRAM) && nvbo->domain == NOUVEAU_BO_VRAM)
		return 0;
	if ((flags & NOUVEAU_BO_GART) && nvbo->domain == NOUVEAU_BO_GART)
		return 0;
	assert(flags & (NOUVEAU_BO_VRAM|NOUVEAU_BO_GART));

	/* Keep tiling info */
	flags |= (nvbo->flags & (NOUVEAU_BO_TILED|NOUVEAU_BO_ZTILE));

	ret = nouveau_bo_new(nvbo->base.device, flags, 0, nvbo->size, &new);
	if (ret)
		return ret;

	ret = nouveau_bo_kalloc(nouveau_bo(new), NULL);
	if (ret) {
		nouveau_bo_ref(NULL, &new);
		return ret;
	}

	if (nvbo->handle || nvbo->sysmem) {
	nouveau_bo_kmap(nouveau_bo(new));

	if (!nvbo->base.map) {
		nouveau_bo_map(&nvbo->base, NOUVEAU_BO_RD);
		memcpy(nouveau_bo(new)->map, nvbo->base.map, nvbo->base.size);
		nouveau_bo_unmap(&nvbo->base);
	} else {
		memcpy(nouveau_bo(new)->map, nvbo->base.map, nvbo->base.size);
	}
	}

	t_handle = nvbo->handle;
	t_domain = nvbo->domain;
	t_offset = nvbo->offset;
	t_size = nvbo->size;
	t_map = nvbo->map;

	nvbo->handle = nouveau_bo(new)->handle;
	nvbo->domain = nouveau_bo(new)->domain;
	nvbo->offset = nouveau_bo(new)->offset;
	nvbo->size = nouveau_bo(new)->size;
	nvbo->map = nouveau_bo(new)->map;

	nouveau_bo(new)->handle = t_handle;
	nouveau_bo(new)->domain = t_domain;
	nouveau_bo(new)->offset = t_offset;
	nouveau_bo(new)->size = t_size;
	nouveau_bo(new)->map = t_map;

	nouveau_bo_ref(NULL, &new);

	return 0;
}

static int
nouveau_bo_pin_nomm(struct nouveau_bo *bo, uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;

	if (!nvbo->handle) {
		if (!(flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)))
			return -EINVAL;

		ret = nouveau_bo_validate_nomm(nvbo, flags & ~NOUVEAU_BO_PIN);
		if (ret)
			return ret;
	}

	nvbo->pinned = 1;

	/* Fill in public nouveau_bo members */
	bo->flags = nvbo->domain;
	bo->offset = nvbo->offset;

	return 0;
}

int
nouveau_bo_pin(struct nouveau_bo *bo, uint32_t flags)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_pin req;
	int ret;

	if (nvbo->pinned)
		return 0;

	if (!nvdev->mm_enabled)
		return nouveau_bo_pin_nomm(bo, flags);

	/* Ensure we have a kernel object... */
	if (!nvbo->handle) {
		if (!(flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)))
			return -EINVAL;
		nvbo->flags = flags;

		ret = nouveau_bo_kalloc(nvbo, NULL);
		if (ret)
			return ret;
	}

	/* Now force it to stay put :) */
	req.handle = nvbo->handle;
	req.domain = 0;
	if (nvbo->flags & NOUVEAU_BO_VRAM)
		req.domain |= NOUVEAU_GEM_DOMAIN_VRAM;
	if (nvbo->flags & NOUVEAU_BO_GART)
		req.domain |= NOUVEAU_GEM_DOMAIN_GART;

	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_GEM_PIN, &req,
				  sizeof(struct drm_nouveau_gem_pin));
	if (ret)
		return ret;
	nvbo->offset = req.offset;
	nvbo->domain = req.domain;
	nvbo->pinned = 1;
	nvbo->flags |= NOUVEAU_BO_PIN;

	/* Fill in public nouveau_bo members */
	if (nvbo->domain & NOUVEAU_GEM_DOMAIN_VRAM)
		bo->flags = NOUVEAU_BO_VRAM;
	if (nvbo->domain & NOUVEAU_GEM_DOMAIN_GART)
		bo->flags = NOUVEAU_BO_GART;
	bo->offset = nvbo->offset;

	return 0;
}

void
nouveau_bo_unpin(struct nouveau_bo *bo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_unpin req;

	if (!nvbo->pinned)
		return;

	if (nvdev->mm_enabled) {
		req.handle = nvbo->handle;
		drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_UNPIN,
				&req, sizeof(req));
	}

	nvbo->pinned = bo->offset = bo->flags = 0;
}

int
nouveau_bo_tile(struct nouveau_bo *bo, uint32_t flags, uint32_t delta,
		uint32_t size)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	uint32_t kern_flags = 0;
	int ret = 0;

	if (flags & NOUVEAU_BO_TILED) {
		kern_flags |= NOUVEAU_MEM_TILE;
		if (flags & NOUVEAU_BO_ZTILE)
			kern_flags |= NOUVEAU_MEM_TILE_ZETA;
	}

	if (nvdev->mm_enabled) {
		struct drm_nouveau_gem_tile req;

		req.handle = nvbo->handle;
		req.delta = delta;
		req.size = size;
		req.flags = kern_flags;
		ret = drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_TILE,
				      &req, sizeof(req));
	} else {
		struct drm_nouveau_mem_tile req;

		req.offset = nvbo->offset;
		req.delta = delta;
		req.size = size;
		req.flags = kern_flags;

		if (flags & NOUVEAU_BO_VRAM)
			req.flags |= NOUVEAU_MEM_FB;
		if (flags & NOUVEAU_BO_GART)
			req.flags |= NOUVEAU_MEM_AGP;

		ret = drmCommandWrite(nvdev->fd, DRM_NOUVEAU_MEM_TILE,
				      &req, sizeof(req));
	}

	return 0;
}

int
nouveau_bo_busy(struct nouveau_bo *bo, uint32_t access)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (!nvdev->mm_enabled) {
		struct nouveau_fence *fence;

		if (nvbo->pending && (nvbo->pending->write_domains ||
				      (access & NOUVEAU_BO_WR)))
			return 1;

		if (access & NOUVEAU_BO_WR)
			fence = nvbo->fence;
		else
			fence = nvbo->wr_fence;
		return !nouveau_fence(fence)->signalled;
	}

	return 1;
}

struct drm_nouveau_gem_pushbuf_bo *
nouveau_bo_emit_buffer(struct nouveau_channel *chan, struct nouveau_bo *bo)
{
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(chan->pushbuf);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_pushbuf_bo *pbbo;
	struct nouveau_bo *ref = NULL;
	int ret;

	if (nvbo->pending)
		return nvbo->pending;

	if (!nvbo->handle) {
		ret = nouveau_bo_kalloc(nvbo, chan);
		if (ret)
			return NULL;

		if (nvbo->sysmem) {
			void *sysmem_tmp = nvbo->sysmem;

			nvbo->sysmem = NULL;
			ret = nouveau_bo_map(bo, NOUVEAU_BO_WR);
			if (ret)
				return NULL;
			nvbo->sysmem = sysmem_tmp;

			memcpy(bo->map, nvbo->sysmem, nvbo->base.size);
			nouveau_bo_unmap(bo);
			nouveau_bo_ufree(nvbo);
		}
	}

	if (nvpb->nr_buffers >= NOUVEAU_PUSHBUF_MAX_BUFFERS)
		return NULL;
	pbbo = nvpb->buffers + nvpb->nr_buffers++;
	nvbo->pending = pbbo;
	nvbo->pending_channel = chan;

	nouveau_bo_ref(bo, &ref);
	pbbo->user_priv = (uint64_t)(unsigned long)ref;
	pbbo->handle = nvbo->handle;
	pbbo->valid_domains = NOUVEAU_GEM_DOMAIN_VRAM | NOUVEAU_GEM_DOMAIN_GART;
	pbbo->read_domains = 0;
	pbbo->write_domains = 0;
	pbbo->presumed_domain = nvbo->domain;
	pbbo->presumed_offset = nvbo->offset;
	pbbo->presumed_ok = 1;
	return pbbo;
}
