/*	$NetBSD: i915_trace.h,v 1.18 2021/12/19 11:13:14 riastradh Exp $	*/

/*-
 * Copyright (c) 2013, 2018 The NetBSD Foundation, Inc.
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

#include "i915_drv.h"
#include "i915_request.h"

#include "display/intel_display_types.h"

/* Must come last.  */
#include <drm/drm_trace_netbsd.h>

DEFINE_TRACE3(i915,, cpu__fifo__underrun,
    "enum pipe_drmhack"/*pipe*/,
    "uint32_t"/*frame*/,
    "uint32_t"/*scanline*/);
static inline void
trace_intel_cpu_fifo_underrun(struct drm_i915_private *dev_priv,
    enum pipe pipe)
{
	TRACE3(i915,, cpu__fifo__underrun,
	    pipe,
	    dev_priv->drm.driver->get_vblank_counter(&dev_priv->drm, pipe),
	    intel_get_crtc_scanline(intel_get_crtc_for_pipe(dev_priv, pipe)));
}

DEFINE_TRACE3(i915,, pch__fifo__underrun,
    "enum pipe_drmhack"/*pipe*/,
    "uint32_t"/*frame*/,
    "uint32_t"/*scanline*/);
static inline void
trace_intel_pch_fifo_underrun(struct drm_i915_private *dev_priv,
    enum pipe pipe)
{
	TRACE3(i915,, pch__fifo__underrun,
	    pipe,
	    dev_priv->drm.driver->get_vblank_counter(&dev_priv->drm, pipe),
	    intel_get_crtc_scanline(intel_get_crtc_for_pipe(dev_priv, pipe)));
}

DEFINE_TRACE5(i915,, gem__evict,
    "int"/*devno*/,
    "struct i915_address_space *"/*vm*/,
    "uint64_t"/*size*/,
    "uint64_t"/*align*/,
    "unsigned"/*flags*/);
static inline void
trace_i915_gem_evict(struct i915_address_space *vm,
    uint64_t size, uint64_t align, unsigned flags)
{
	TRACE5(i915,, gem__evict,
	    vm->i915->drm.primary->index, vm, size, align, flags);
}

DEFINE_TRACE6(i915,, gem__evict__node,
    "int"/*devno*/,
    "struct i915_address_space *"/*vm*/,
    "uint64_t"/*start*/,
    "uint64_t"/*size*/,
    "unsigned long"/*color*/,
    "unsigned"/*flags*/);
static inline void
trace_i915_gem_evict_node(struct i915_address_space *vm,
    struct drm_mm_node *node, unsigned flags)
{
	TRACE6(i915,, gem__evict__node,
	    vm->i915->drm.primary->index, vm,
	    node->start, node->size, node->color,
	    flags);
}

DEFINE_TRACE2(i915,, gem__evict__vm,
    "int"/*devno*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_gem_evict_vm(struct i915_address_space *vm)
{
	TRACE2(i915,, gem__evict__vm,  vm->i915->drm.primary->index, vm);
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

#define	I915_DEFINE_TRACE_REQ(M, F, N)					      \
	DEFINE_TRACE6(M, F, N,						      \
	    "uint32_t"/*devno*/,					      \
	    "uint64_t"/*ctx*/,						      \
	    "uint16_t"/*class*/,					      \
	    "uint16_t"/*instance*/,					      \
	    "uint32_t"/*seqno*/,					      \
	    "uint32_t"/*flags*/)

#define	I915_TRACE_REQ(M, F, N, R, FLAGS)				      \
	TRACE6(M, F, N,							      \
	    (R)->i915->drm.primary->index,				      \
	    (R)->fence.context,						      \
	    (R)->engine->uabi_class,					      \
	    (R)->engine->uabi_instance,					      \
	    (R)->fence.seqno,						      \
	    (FLAGS))

I915_DEFINE_TRACE_REQ(i915,, request__queue);
static inline void
trace_i915_request_queue(struct i915_request *request, uint32_t flags)
{
	I915_TRACE_REQ(i915,, request__queue,  request, flags);
}

I915_DEFINE_TRACE_REQ(i915,, request__add);
static inline void
trace_i915_request_add(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__add,  request, 0);
}

I915_DEFINE_TRACE_REQ(i915,, request__submit);
static inline void
trace_i915_request_submit(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__submit,  request, 0);
}

I915_DEFINE_TRACE_REQ(i915,, request__execute);
static inline void
trace_i915_request_execute(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__execute,  request, 0);
}

I915_DEFINE_TRACE_REQ(i915,, request__in);
static inline void
trace_i915_request_in(struct i915_request *request, unsigned port)
{
	/* XXX prio */
	I915_TRACE_REQ(i915,, request__in,  request, port);
}

I915_DEFINE_TRACE_REQ(i915,, request__out);
static inline void
trace_i915_request_out(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__out,
	    request, i915_request_completed(request));
}

I915_DEFINE_TRACE_REQ(i915,, request__retire);
static inline void
trace_i915_request_retire(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__retire, request, 0);
}

I915_DEFINE_TRACE_REQ(i915,, request__wait__begin);
static inline void
trace_i915_request_wait_begin(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__wait__begin, request, 0);
}

I915_DEFINE_TRACE_REQ(i915,, request__wait__end);
static inline void
trace_i915_request_wait_end(struct i915_request *request)
{
	I915_TRACE_REQ(i915,, request__wait__end, request, 0);
}

DEFINE_TRACE3(i915,, register__read,
    "uint32_t"/*reg*/, "uint64_t"/*value*/, "size_t"/*len*/);
DEFINE_TRACE3(i915,, register__write,
    "uint32_t"/*reg*/, "uint64_t"/*value*/, "size_t"/*len*/);
static inline void
trace_i915_reg_rw(bool write, i915_reg_t reg, uint64_t value, size_t len,
    bool trace)
{
	uint32_t regoff = i915_mmio_reg_offset(reg);

	if (!trace)
		return;
	if (write) {
		TRACE3(i915,, register__read,  regoff, value, len);
	} else {
		TRACE3(i915,, register__write,  regoff, value, len);
	}
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
    "struct i915_gem_context *"/*ctx*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_context_create(struct i915_gem_context *ctx)
{
	TRACE3(i915,, context__create,
	    ctx->i915->drm.primary->index,
	    ctx,
	    rcu_access_pointer(ctx->vm));
}

DEFINE_TRACE3(i915,, context__free,
    "int"/*devno*/,
    "struct i915_gem_context *"/*ctx*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_context_free(struct i915_gem_context *ctx)
{
	TRACE3(i915,, context__free,
	    ctx->i915->drm.primary->index,
	    ctx,
	    rcu_access_pointer(ctx->vm));
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
	TRACE2(i915,, ppgtt__create,  vm->i915->drm.primary->index, vm);
}

DEFINE_TRACE2(i915,, ppgtt__release,
    "int"/*devno*/,
    "struct i915_address_space *"/*vm*/);
static inline void
trace_i915_ppgtt_release(struct i915_address_space *vm)
{
	TRACE2(i915,, ppgtt__release,  vm->i915->drm.primary->index, vm);
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
	    dev_priv->drm.primary->index, target, flags);
}

DEFINE_TRACE5(i915,, pipe__update__start,
    "enum pipe_drmhack"/*pipe*/,
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
    "enum pipe_drmhack"/*pipe*/,
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
    "enum pipe_drmhack"/*pipe*/,
    "uint32_t"/*frame*/,
    "int"/*scanline*/);
static inline void
trace_i915_pipe_update_end(struct intel_crtc *crtc, uint32_t frame,
    int scanline)
{
	TRACE3(i915,, pipe__update__end,  crtc->pipe, frame, scanline);
}

#endif  /* _I915_TRACE_H_ */
