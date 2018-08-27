/*	$NetBSD: i915_trace.h,v 1.13 2018/08/27 15:09:35 riastradh Exp $	*/

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
#include <sys/sdt.h>

#include "intel_drv.h"

/* Must come last.  */
#include <drm/drm_trace_netbsd.h>

DEFINE_TRACE2(i915,, flip__request,
    "enum plane"/*plane*/, "struct drm_i915_gem_object *"/*obj*/);
static inline void
trace_i915_flip_request(enum plane plane, struct drm_i915_gem_object *obj)
{
	TRACE2(i915,, flip__request,  plane, obj);
}

DEFINE_TRACE2(i915,, flip__complete,
    "enum plane"/*plane*/, "struct drm_i915_gem_object *"/*obj*/);
static inline void
trace_i915_flip_complete(enum plane plane, struct drm_i915_gem_object *obj)
{
	TRACE2(i915,, flip__complete,  plane, obj);
}

DEFINE_TRACE4(i915,, gem__evict,
    "int"/*devno*/,
    "int"/*min_size*/, "unsigned"/*alignment*/, "unsigned"/*flags*/);
static inline void
trace_i915_gem_evict(struct drm_device *dev, int min_size, unsigned alignment,
    unsigned flags)
{
	TRACE4(i915,, gem__evict,
	    dev->primary->index, min_size, alignment, flags);
}

DEFINE_TRACE2(i915,, gem__evict__vm,
    "int"/*devno*/, "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_gem_evict_vm(struct i915_address_space *vm)
{
	TRACE2(i915,, gem__evict__vm,  vm->dev->primary->index, vm);
}

DEFINE_TRACE3(i915,, gem__object__change__domain,
    "struct drm_i915_gem_object *"/*obj*/,
    "uint32_t"/*read_domains*/,
    "uint32_t"/*write_domain*/);
static inline void
trace_i915_gem_object_change_domain(struct drm_i915_gem_object *obj,
    uint32_t old_read_domains, uint32_t old_write_domain)
{
	TRACE3(i915,, gem__object__change__domain,
	    obj,
	    obj->base.read_domains | (old_read_domains << 16),
	    obj->base.write_domain | (old_write_domain << 16));
}

DEFINE_TRACE1(i915,, gem__object__clflush,
    "struct drm_i915_gem_object *"/*obj*/);
static inline void
trace_i915_gem_object_clflush(struct drm_i915_gem_object *obj)
{
	TRACE1(i915,, gem__object__clflush,  obj);
}

DEFINE_TRACE2(i915,, gem__object__create,
    "struct drm_i915_gem_object *"/*obj*/,
    "size_t"/*size*/);
static inline void
trace_i915_gem_object_create(struct drm_i915_gem_object *obj)
{
	TRACE2(i915,, gem__object__create,  obj, obj->base.size);
}

DEFINE_TRACE1(i915,, gem__object__destroy,
    "struct drm_i915_gem_object *"/*obj*/);
static inline void
trace_i915_gem_object_destroy(struct drm_i915_gem_object *obj)
{
	TRACE1(i915,, gem__object__destroy,  obj);
}

DEFINE_TRACE4(i915,, gem__object__fault,
    "struct drm_i915_gem_object *"/*obj*/,
    "pgoff_t"/*page_offset*/,
    "bool"/*gtt*/,
    "bool"/*write*/);
static inline void
trace_i915_gem_object_fault(struct drm_i915_gem_object *obj,
    pgoff_t page_offset, bool gtt, bool write)
{
	TRACE4(i915,, gem__object__fault,  obj, page_offset, gtt, write);
}

/* XXX Not sure about size/offset types here.  */
DEFINE_TRACE3(i915,, gem__object__pread,
    "struct drm_i915_gem_object *"/*obj*/,
    "off_t"/*offset*/,
    "size_t"/*size*/);
static inline void
trace_i915_gem_object_pread(struct drm_i915_gem_object *obj, off_t offset,
    size_t size)
{
	TRACE3(i915,, gem__object__pread,  obj, offset, size);
}

DEFINE_TRACE3(i915,, gem__object__write,
    "struct drm_i915_gem_object *"/*obj*/,
    "off_t"/*offset*/,
    "size_t"/*size*/);
static inline void
trace_i915_gem_object_pwrite(struct drm_i915_gem_object *obj, off_t offset,
    size_t size)
{
	TRACE3(i915,, gem__object__write,  obj, offset, size);
}

DEFINE_TRACE3(i915,, gem__request__add,
    "int"/*devno*/, "int"/*ringid*/, "uint32_t"/*seqno*/);
static inline void
trace_i915_gem_request_add(struct drm_i915_gem_request *request)
{
	TRACE3(i915,, gem__request__add,
	    request->ring->dev->primary->index,
	    request->ring->id,
	    request->seqno);
}

DEFINE_TRACE3(i915,, gem__request__retire,
    "int"/*devno*/, "int"/*ringid*/, "uint32_t"/*seqno*/);
static inline void
trace_i915_gem_request_retire(struct drm_i915_gem_request *request)
{
	TRACE3(i915,, gem__request__retire,
	    request->ring->dev->primary->index,
	    request->ring->id,
	    request->seqno);
}

DEFINE_TRACE3(i915,, gem__request__wait__begin,
    "int"/*devno*/, "int"/*ringid*/, "uint32_t"/*seqno*/);
static inline void
trace_i915_gem_request_wait_begin(struct drm_i915_gem_request *request)
{
	TRACE3(i915,, gem__request__wait__begin,
	    request->ring->dev->primary->index,
	    request->ring->id,
	    request->seqno);
}

DEFINE_TRACE3(i915,, gem__request__wait__end,
    "int"/*devno*/, "int"/*ringid*/, "uint32_t"/*seqno*/);
static inline void
trace_i915_gem_request_wait_end(struct drm_i915_gem_request *request)
{
	TRACE3(i915,, gem__request__wait__end,
	    request->ring->dev->primary->index,
	    request->ring->id,
	    request->seqno);
}

DEFINE_TRACE3(i915,, gem__request__notify,
    "int"/*devno*/, "int"/*ringid*/, "uint32_t"/*seqno*/);
static inline void
trace_i915_gem_request_notify(struct intel_engine_cs *ring)
{
	TRACE3(i915,, gem__request__notify,
	    ring->dev->primary->index, ring->id, ring->get_seqno(ring, false));
}

/* XXX Why no request in the trace upstream?  */
DEFINE_TRACE4(i915,, gem__ring__dispatch,
    "int"/*devno*/, "int"/*ringid*/, "uint32_t"/*seqno*/, "uint32_t"/*flags*/);
static inline void
trace_i915_gem_ring_dispatch(struct drm_i915_gem_request *request,
    uint32_t flags)
{
	TRACE4(i915,, gem__ring__dispatch,
	    request->ring->dev->primary->index,
	    request->ring->id,
	    request->seqno,
	    flags);
	/* XXX i915_trace_irq_get?  Doesn't seem to be used.  */
}

DEFINE_TRACE4(i915,, gem__ring__flush,
    "int"/*devno*/,
    "int"/*ringid*/,
    "uint32_t"/*invalidate*/,
    "uint32_t"/*flags*/);
static inline void
trace_i915_gem_ring_flush(struct drm_i915_gem_request *request,
    uint32_t invalidate, uint32_t flags)
{
	TRACE4(i915,, gem__ring__flush,
	    request->ring->dev->primary->index,
	    request->ring->id,
	    invalidate,
	    flags);
}

DEFINE_TRACE4(i915,, gem__ring__sync__to,
    "int"/*devno*/,
    "int"/*from_ringid*/,
    "int"/*to_ringid*/,
    "uint32_t"/*seqno*/);
static inline void
trace_i915_gem_ring_sync_to(struct drm_i915_gem_request *to_req,
    struct intel_engine_cs *from, struct drm_i915_gem_request *from_req)
{
	TRACE4(i915,, gem__ring__sync__to,
	    from->dev->primary->index,
	    from->id,
	    to_req->ring->id,
	    i915_gem_request_get_seqno(from_req));
}

DEFINE_TRACE3(i915,, register__read,
    "uint32_t"/*reg*/, "uint64_t"/*value*/, "size_t"/*len*/);
DEFINE_TRACE3(i915,, register__write,
    "uint32_t"/*reg*/, "uint64_t"/*value*/, "size_t"/*len*/);
static inline void
trace_i915_reg_rw(bool write, uint32_t reg, uint64_t value, size_t len,
    bool trace)
{
	if (!trace)
		return;
	if (write)
		TRACE3(i915,, register__read,  reg, value, len);
	else
		TRACE3(i915,, register__write,  reg, value, len);
}

DEFINE_TRACE5(i915,, vma__bind,
    "struct drm_i915_gem_object *"/*obj*/,
    "struct i915_address_space *"/*vm*/,
    "uint64_t"/*offset*/,
    "uint64_t"/*size*/,
    "uint64_t"/*flags*/);
static inline void
trace_i915_vma_bind(struct i915_vma *vma, uint64_t flags)
{
	TRACE5(i915,, vma__bind,
	    vma->obj, vma->vm, vma->node.start, vma->node.size, flags);
}

DEFINE_TRACE4(i915,, vma__unbind,
    "struct drm_i915_gem_object *"/*obj*/,
    "struct i915_address_space *"/*vm*/,
    "uint64_t"/*offset*/,
    "uint64_t"/*size*/);
static inline void
trace_i915_vma_unbind(struct i915_vma *vma)
{
	TRACE4(i915,, vma__unbind,
	    vma->obj, vma->vm, vma->node.start, vma->node.size);
}

DEFINE_TRACE1(i915,, gpu__freq__change,
    "int"/*freq*/);
static inline void
trace_intel_gpu_freq_change(int freq)
{
	TRACE1(i915,, gpu__freq__change,  freq);
}

DEFINE_TRACE3(i915,, context__create,
    "int"/*devno*/,
    "struct intel_context *"/*ctx*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_context_create(struct intel_context *ctx)
{
	TRACE3(i915,, context__create,
	    ctx->i915->dev->primary->index,
	    ctx,
	    (ctx->ppgtt ? &ctx->ppgtt->base : NULL));
}

DEFINE_TRACE3(i915,, context__free,
    "int"/*devno*/,
    "struct intel_context *"/*ctx*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_context_free(struct intel_context *ctx)
{
	TRACE3(i915,, context__free,
	    ctx->i915->dev->primary->index,
	    ctx,
	    (ctx->ppgtt ? &ctx->ppgtt->base : NULL));
}

DEFINE_TRACE4(i915,, page_directory_entry_alloc,
    "struct i915_address_space *"/*vm*/,
    "uint32_t"/*pdpe*/,
    "uint64_t"/*start*/,
    "uint64_t"/*pde_shift*/);
static inline void
trace_i915_page_directory_entry_alloc(struct i915_address_space *vm,
    uint32_t pdpe, uint64_t start, uint64_t pde_shift)
{
	TRACE4(i915,, page_directory_entry_alloc,  vm, pdpe, start, pde_shift);
}

DEFINE_TRACE4(i915,, page_directory_pointer_entry_alloc,
    "struct i915_address_space *"/*vm*/,
    "uint32_t"/*pml4e*/,
    "uint64_t"/*start*/,
    "uint64_t"/*pde_shift*/);
static inline void
trace_i915_page_directory_pointer_entry_alloc(struct i915_address_space *vm,
    uint32_t pml4e, uint64_t start, uint64_t pde_shift)
{
	TRACE4(i915,, page_directory_pointer_entry_alloc,
	    vm, pml4e, start, pde_shift);
}

DEFINE_TRACE4(i915,, page_table_entry_alloc,
    "struct i915_address_space *"/*vm*/,
    "uint32_t"/*pde*/,
    "uint64_t"/*start*/,
    "uint64_t"/*pde_shift*/);
static inline void
trace_i915_page_table_entry_alloc(struct i915_address_space *vm, uint32_t pde,
    uint64_t start, uint64_t pde_shift)
{
	TRACE4(i915,, page_table_entry_alloc,  vm, pde, start, pde_shift);
}

DEFINE_TRACE6(i915,, page_table_entry_map,
    "struct i915_address_space *"/*vm*/,
    "uint32_t"/*pde*/,
    "struct i915_page_table *"/*pt*/,
    "uint32_t"/*first*/,
    "uint32_t"/*count*/,
    "uint32_t"/*bits*/);
static inline void
trace_i915_page_table_entry_map(struct i915_address_space *vm, uint32_t pde,
    struct i915_page_table *pt, uint32_t first, uint32_t count, uint32_t bits)
{
	TRACE6(i915,, page_table_entry_map,  vm, pde, pt, first, count, bits);
}

DEFINE_TRACE2(i915,, ppgtt__create,
    "int"/*devno*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_ppgtt_create(struct i915_address_space *vm)
{
	TRACE2(i915,, ppgtt__create,  vm->dev->primary->index, vm);
}

DEFINE_TRACE2(i915,, ppgtt__release,
    "int"/*devno*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_ppgtt_release(struct i915_address_space *vm)
{
	TRACE2(i915,, ppgtt__release,  vm->dev->primary->index, vm);
}

#define	VM_TO_TRACE_NAME(vm)	(i915_is_ggtt(vm) ? "G" : "P")

DEFINE_TRACE4(i915,, va__alloc,
    "struct i915_address_space *"/*vm*/,
    "uint64_t"/*start*/,
    "uint64_t"/*end*/,
    "const char *"/*name*/);
static inline void
trace_i915_va_alloc(struct i915_address_space *vm, uint64_t start,
    uint64_t length, const char *name)
{
	/* XXX Why start/end upstream?  */
	TRACE4(i915,, va__alloc,  vm, start, start + length - 1, name);
}

DEFINE_TRACE3(i915,, gem__shrink,
    "int"/*devno*/,
    "unsigned long"/*target*/,
    "unsigned"/*flags*/);
static inline void
trace_i915_gem_shrink(struct drm_i915_private *dev_priv, unsigned long target,
    unsigned flags)
{
	TRACE3(i915,, gem__shrink,
	    dev_priv->dev->primary->index, target, flags);
}

DEFINE_TRACE5(i915,, pipe__update__start,
    "enum i915_pipe"/*pipe*/,
    "uint32_t"/*frame*/,
    "int"/*scanline*/,
    "uint32_t"/*min*/,
    "uint32_t"/*max*/);
static inline void
trace_i915_pipe_update_start(struct intel_crtc *crtc)
{
	TRACE5(i915,, pipe__update__start,
	    crtc->pipe,
	    crtc->base.dev->driver->get_vblank_counter(crtc->base.dev,
		crtc->pipe),
	    intel_get_crtc_scanline(crtc),
	    crtc->debug.min_vbl,
	    crtc->debug.max_vbl);
}

DEFINE_TRACE5(i915,, pipe__update__vblank__evaded,
    "enum i915_pipe"/*pipe*/,
    "uint32_t"/*frame*/,
    "int"/*scanline*/,
    "uint32_t"/*min*/,
    "uint32_t"/*max*/);
static inline void
trace_i915_pipe_update_vblank_evaded(struct intel_crtc *crtc)
{
	TRACE5(i915,, pipe__update__vblank__evaded,
	    crtc->pipe,
	    crtc->debug.start_vbl_count,
	    crtc->debug.scanline_start,
	    crtc->debug.min_vbl,
	    crtc->debug.max_vbl);
}

DEFINE_TRACE3(i915,, pipe__update__end,
    "enum i915_pipe"/*pipe*/,
    "uint32_t"/*frame*/,
    "int"/*scanline*/);
static inline void
trace_i915_pipe_update_end(struct intel_crtc *crtc, uint32_t frame,
    int scanline)
{
	TRACE3(i915,, pipe__update__end,  crtc->pipe, frame, scanline);
}

DEFINE_TRACE4(i915,, switch__mm,
    "int"/*devno*/,
    "int"/*ringid*/,
    "struct intel_context *"/*to*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_switch_mm(struct intel_engine_cs *ring, struct intel_context *to)
{
	TRACE4(i915,, switch__mm,
	    ring->dev->primary->index,
	    ring->id,
	    to,
	    to->ppgtt ? &to->ppgtt->base : NULL);
}

#endif  /* _I915_TRACE_H_ */
