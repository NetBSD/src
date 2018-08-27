/*	$NetBSD: i915_trace.h,v 1.5 2018/08/27 06:18:17 riastradh Exp $	*/

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

#ifndef _I915_TRACE_H_
#define _I915_TRACE_H_

#include <sys/types.h>

#include "intel_drv.h"

static inline void
trace_i915_flip_request(enum plane plane __unused,
    struct drm_i915_gem_object *obj __unused)
{
}

static inline void
trace_i915_gem_evict(struct drm_device *dev __unused, int min_size __unused,
    unsigned int alignment __unused, bool mappable __unused)
{
}

static inline void
trace_i915_gem_evict_everything(struct drm_device *dev __unused)
{
}

static inline void
trace_i915_gem_evict_vm(struct i915_address_space *vm __unused)
{
}

static inline void
trace_i915_gem_object_bind(struct drm_i915_gem_object *obj __unused,
    bool map_and_fenceable __unused)
{
}

static inline void
trace_i915_gem_object_change_domain(struct drm_i915_gem_object *obj __unused,
    uint32_t read_domains __unused, uint32_t old_write_domain __unused)
{
}

static inline void
trace_i915_gem_object_clflush(struct drm_i915_gem_object *obj __unused)
{
}

static inline void
trace_i915_gem_object_create(struct drm_i915_gem_object *obj __unused)
{
}

static inline void
trace_i915_gem_object_destroy(struct drm_i915_gem_object *obj __unused)
{
}

static inline void
trace_i915_gem_object_fault(struct drm_i915_gem_object *obj __unused,
    pgoff_t page_offset __unused, bool gtt __unused, bool write __unused)
{
}

static inline void
trace_i915_gem_object_pread(struct drm_i915_gem_object *obj __unused,
    uint32_t offset __unused, uint32_t size __unused)
{
}

static inline void
trace_i915_gem_object_pwrite(struct drm_i915_gem_object *obj __unused,
    uint32_t offset __unused, uint32_t size __unused)
{
}

static inline void
trace_i915_gem_object_unbind(struct drm_i915_gem_object *obj __unused)
{
}

static inline void
trace_i915_gem_request_add(struct drm_i915_gem_request *request)
{
}

static inline void
trace_i915_gem_request_retire(struct drm_i915_gem_request *request __unused)
{
}

static inline void
trace_i915_gem_request_wait_begin(struct drm_i915_gem_request *request __unused,
    uint32_t seqno __unused)
{
}

static inline void
trace_i915_gem_request_wait_end(struct drm_i915_gem_request *request __unused,
    uint32_t seqno __unused)
{
}

static inline void
trace_i915_gem_ring_dispatch(struct drm_i915_gem_request *request __unused,
    uint32_t seqno __unused, uint32_t flags __unused)
{
}

static inline void
trace_i915_gem_ring_flush(struct drm_i915_gem_request *request __unused,
    uint32_t invalidate __unused, uint32_t flags __unused)
{
}

static inline void
trace_i915_gem_ring_sync_to(struct drm_i915_gem_request *from __unused,
    struct drm_i915_gem_request *to __unused, u32 seqno __unused)
{
}

static inline void
trace_i915_reg_rw(bool write __unused, uint32_t reg __unused,
    uint64_t value __unused, size_t len __unused, bool trace __unused)
{
}

static inline void
trace_i915_vma_bind(struct i915_vma *vma __unused, uint64_t flags __unused)
{
}

static inline void
trace_i915_vma_unbind(struct i915_vma *vma __unused)
{
}

static inline void
trace_intel_gpu_freq_change(unsigned int freq __unused)
{
}

#endif  /* _I915_TRACE_H_ */
