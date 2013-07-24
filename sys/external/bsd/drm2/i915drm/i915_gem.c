/*	$NetBSD: i915_gem.c,v 1.1.2.2 2013/07/24 04:04:45 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* i915_gem*.c stubs */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_gem.c,v 1.1.2.2 2013/07/24 04:04:45 riastradh Exp $");

#include "i915_drv.h"

int
i915_add_request(struct intel_ring_buffer *ring __unused,
    struct drm_file *file __unused, u32 *out_seqno __unused)
{
	return ENOTTY;
}

struct drm_i915_gem_object *
i915_gem_alloc_object(struct drm_device *dev __unused, size_t size __unused)
{
	return NULL;
}

int
i915_gem_attach_phys_object(struct drm_device *dev __unused,
    struct drm_i915_gem_object *obj __unused, int id __unused,
    int align __unused)
{
	return ENOTTY;
}

int
i915_gem_busy_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

void
i915_gem_cleanup_aliasing_ppgtt(struct drm_device *dev __unused)
{
}

void
i915_gem_cleanup_ringbuffer(struct drm_device *dev __unused)
{
}

void
i915_gem_cleanup_stolen(struct drm_device *dev __unused)
{
}

void
i915_gem_context_close(struct drm_device *dev __unused,
    struct drm_file *file __unused)
{
}

void
i915_gem_context_fini(struct drm_device *dev __unused)
{
}

int
i915_gem_context_create_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_context_destroy_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

void
i915_gem_context_init(struct drm_device *dev __unused)
{
}

int
i915_gem_create_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

void
i915_gem_detach_phys_object(struct drm_device *dev __unused,
    struct drm_i915_gem_object *obj __unused)
{
}

int
i915_gem_dumb_create(struct drm_file *file __unused,
    struct drm_device *dev __unused,
    struct drm_mode_create_dumb *args __unused)
{
	return ENOTTY;
}

int
i915_gem_dumb_destroy(struct drm_file *file __unused,
    struct drm_device *dev __unused,
    uint32_t handle __unused)
{
	return ENOTTY;
}

int
i915_gem_entervt_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_execbuffer(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_execbuffer2(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

void
i915_gem_free_all_phys_object(struct drm_device *dev __unused)
{
}

void
i915_gem_free_object(struct drm_gem_object *obj)
{
}

int
i915_gem_get_aperture_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_get_caching_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_get_tiling(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_gtt_init(struct drm_device *dev __unused)
{
	return 0;
}

void
i915_gem_gtt_fini(struct drm_device *dev __unused)
{
}

int
i915_gem_idle(struct drm_device *dev __unused)
{
	return 0;
}

int
i915_gem_init_hw(struct drm_device *dev __unused)
{
	return 0;
}

int
i915_gem_init_object(struct drm_gem_object *obj __unused)
{
	return ENOTTY;
}

void
i915_gem_init_ppgtt(struct drm_device *dev __unused)
{
}

int
i915_gem_init_stolen(struct drm_device *dev __unused)
{
	return 0;
}

void
i915_gem_init_swizzling(struct drm_device *dev __unused)
{
}

int
i915_gem_init(struct drm_device *dev __unused)
{
	return 0;
}

int
i915_gem_init_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

void
i915_gem_lastclose(struct drm_device *dev __unused)
{
}

int
i915_gem_leavevt_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

static void
retire_work_handler(struct work_struct *work __unused)
{
}

void
i915_gem_load(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;

	INIT_DELAYED_WORK(&dev_priv->mm.retire_work, &retire_work_handler);
}

int
i915_gem_madvise_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_mmap_gtt(struct drm_file *file __unused,
    struct drm_device *dev __unused, uint32_t handle __unused,
    uint64_t *offset __unused)
{
	return ENOTTY;
}

int
i915_gem_mmap_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_mmap_gtt_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_object_get_fence(struct drm_i915_gem_object *obj __unused)
{
	return ENOTTY;
}

int
i915_gem_object_pin(struct drm_i915_gem_object *obj __unused,
    uint32_t alignment __unused, bool map_and_fenceable __unused,
    bool nonblocking __unused)
{
	return ENOTTY;
}

int
i915_gem_object_pin_to_display_plane(struct drm_i915_gem_object *obj __unused,
    u32 alignment __unused, struct intel_ring_buffer *pipelined __unused)
{
	return ENOTTY;
}

int
i915_gem_object_put_fence(struct drm_i915_gem_object *obj __unused)
{
	return ENOTTY;
}

int
i915_gem_object_set_to_gtt_domain(struct drm_i915_gem_object *obj __unused,
    bool write __unused)
{
	return ENOTTY;
}

void
i915_gem_object_unpin(struct drm_i915_gem_object *obj __unused)
{
}

int
i915_gem_pin_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_pread_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_pwrite_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

void
i915_gem_release(struct drm_device *dev __unused,
    struct drm_file *file __unused)
{
}

void
i915_gem_reset(struct drm_device *dev __unused)
{
}

void
i915_gem_restore_gtt_mappings(struct drm_device *dev __unused)
{
}

void
i915_gem_retire_requests(struct drm_device *dev __unused)
{
}

int
i915_gem_set_caching_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_set_domain_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_set_tiling(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_sw_finish_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_throttle_ioctl(struct drm_device *dev, void *data,
    struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_unpin_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gem_wait_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	return ENOTTY;
}

int
i915_gpu_idle(struct drm_device *dev __unused)
{
	return 0;
}

int
i915_mutex_lock_interruptible(struct drm_device *dev __unused)
{
	return ENOTTY;
}

void
i915_update_gfx_val(struct drm_i915_private *dev_priv __unused)
{
}

int
i915_wait_seqno(struct intel_ring_buffer *ring __unused,
    uint32_t seqno __unused)
{
	return ENOTTY;
}
