/*
 * Copyright © 2011-2012 Intel Corporation
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Ben Widawsky <ben@bwidawsk.net>
 *
 */

/*
 * This file implements HW context support. On gen5+ a HW context consists of an
 * opaque GPU object which is referenced at times of context saves and restores.
 * With RC6 enabled, the context is also referenced as the GPU enters and exists
 * from RC6 (GPU has it's own internal power context, except on gen5). Though
 * something like a context does exist for the media ring, the code only
 * supports contexts for the render ring.
 *
 * In software, there is a distinction between contexts created by the user,
 * and the default HW context. The default HW context is used by GPU clients
 * that do not request setup of their own hardware context. The default
 * context's state is never restored to help prevent programming errors. This
 * would happen if a client ran and piggy-backed off another clients GPU state.
 * The default context only exists to give the GPU some offset to load as the
 * current to invoke a save of the context we actually care about. In fact, the
 * code could likely be constructed, albeit in a more complicated fashion, to
 * never use the default context, though that limits the driver's ability to
 * swap out, and/or destroy other contexts.
 *
 * All other contexts are created as a request by the GPU client. These contexts
 * store GPU state, and thus allow GPU clients to not re-emit state (and
 * potentially query certain state) at any time. The kernel driver makes
 * certain that the appropriate commands are inserted.
 *
 * The context life cycle is semi-complicated in that context BOs may live
 * longer than the context itself because of the way the hardware, and object
 * tracking works. Below is a very crude representation of the state machine
 * describing the context life.
 *                                         refcount     pincount     active
 * S0: initial state                          0            0           0
 * S1: context created                        1            0           0
 * S2: context is currently running           2            1           X
 * S3: GPU referenced, but not current        2            0           1
 * S4: context is current, but destroyed      1            1           0
 * S5: like S3, but destroyed                 1            0           1
 *
 * The most common (but not all) transitions:
 * S0->S1: client creates a context
 * S1->S2: client submits execbuf with context
 * S2->S3: other clients submits execbuf with context
 * S3->S1: context object was retired
 * S3->S2: clients submits another execbuf
 * S2->S4: context destroy called with current context
 * S3->S5->S0: destroy path
 * S4->S5->S0: destroy path on current context
 *
 * There are two confusing terms used above:
 *  The "current context" means the context which is currently running on the
 *  GPU. The GPU has loaded its state already and has stored away the gtt
 *  offset of the BO. The GPU is not actively referencing the data at this
 *  offset, but it will on the next context switch. The only way to avoid this
 *  is to do a GPU reset.
 *
 *  An "active context' is one which was previously the "current context" and is
 *  on the active list waiting for the next context switch to occur. Until this
 *  happens, the object must remain at the same gtt offset. It is therefore
 *  possible to destroy a context, but it is still active.
 *
 */

#include <linux/err.h>
#include <drm/drmP.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"

/* This is a HW constraint. The value below is the largest known requirement
 * I've seen in a spec to date, and that was a workaround for a non-shipping
 * part. It should be safe to decrease this, but it's more future proof as is.
 */
#define GEN6_CONTEXT_ALIGN (64<<10)
#define GEN7_CONTEXT_ALIGN 4096

static void do_ppgtt_cleanup(struct i915_hw_ppgtt *ppgtt)
{
	struct drm_device *dev = ppgtt->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_address_space *vm = &ppgtt->base;

	if (ppgtt == dev_priv->mm.aliasing_ppgtt ||
	    (list_empty(&vm->active_list) && list_empty(&vm->inactive_list))) {
		ppgtt->base.cleanup(&ppgtt->base);
		return;
	}

	/*
	 * Make sure vmas are unbound before we take down the drm_mm
	 *
	 * FIXME: Proper refcounting should take care of this, this shouldn't be
	 * needed at all.
	 */
	if (!list_empty(&vm->active_list)) {
		struct i915_vma *vma;

		list_for_each_entry(vma, &vm->active_list, mm_list)
			if (WARN_ON(list_empty(&vma->vma_link) ||
				    list_is_singular(&vma->vma_link)))
				break;

		i915_gem_evict_vm(&ppgtt->base, true);
	} else {
		i915_gem_retire_requests(dev);
		i915_gem_evict_vm(&ppgtt->base, false);
	}

	ppgtt->base.cleanup(&ppgtt->base);
}

static void ppgtt_release(struct kref *kref)
{
	struct i915_hw_ppgtt *ppgtt =
		container_of(kref, struct i915_hw_ppgtt, ref);

	do_ppgtt_cleanup(ppgtt);
	kfree(ppgtt);
}

static size_t get_context_alignment(struct drm_device *dev)
{
	if (IS_GEN6(dev))
		return GEN6_CONTEXT_ALIGN;

	return GEN7_CONTEXT_ALIGN;
}

static int get_context_size(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;
	u32 reg;

	switch (INTEL_INFO(dev)->gen) {
	case 6:
		reg = I915_READ(CXT_SIZE);
		ret = GEN6_CXT_TOTAL_SIZE(reg) * 64;
		break;
	case 7:
		reg = I915_READ(GEN7_CXT_SIZE);
		if (IS_HASWELL(dev))
			ret = HSW_CXT_TOTAL_SIZE;
		else
			ret = GEN7_CXT_TOTAL_SIZE(reg) * 64;
		break;
	case 8:
		ret = GEN8_CXT_TOTAL_SIZE;
		break;
	default:
		BUG();
	}

	return ret;
}

void i915_gem_context_free(struct kref *ctx_ref)
{
	struct i915_hw_context *ctx = container_of(ctx_ref,
						   typeof(*ctx), ref);
	struct i915_hw_ppgtt *ppgtt = NULL;

	if (ctx->obj) {
		/* We refcount even the aliasing PPGTT to keep the code symmetric */
		if (USES_PPGTT(ctx->obj->base.dev))
			ppgtt = ctx_to_ppgtt(ctx);

		/* XXX: Free up the object before tearing down the address space, in
		 * case we're bound in the PPGTT */
		drm_gem_object_unreference(&ctx->obj->base);
	}

	if (ppgtt)
		kref_put(&ppgtt->ref, ppgtt_release);
	list_del(&ctx->link);
	kfree(ctx);
}

static struct i915_hw_ppgtt *
create_vm_for_ctx(struct drm_device *dev, struct i915_hw_context *ctx)
{
	struct i915_hw_ppgtt *ppgtt;
	int ret;

	ppgtt = kzalloc(sizeof(*ppgtt), GFP_KERNEL);
	if (!ppgtt)
		return ERR_PTR(-ENOMEM);

	ret = i915_gem_init_ppgtt(dev, ppgtt);
	if (ret) {
		kfree(ppgtt);
		return ERR_PTR(ret);
	}

	ppgtt->ctx = ctx;
	return ppgtt;
}

static struct i915_hw_context *
__create_hw_context(struct drm_device *dev,
		  struct drm_i915_file_private *file_priv)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_hw_context *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
		return ERR_PTR(-ENOMEM);

	kref_init(&ctx->ref);
	list_add_tail(&ctx->link, &dev_priv->context_list);

	if (dev_priv->hw_context_size) {
		ctx->obj = i915_gem_alloc_object(dev, dev_priv->hw_context_size);
		if (ctx->obj == NULL) {
			ret = -ENOMEM;
			goto err_out;
		}

		if (INTEL_INFO(dev)->gen >= 7) {
			ret = i915_gem_object_set_cache_level(ctx->obj,
							      I915_CACHE_L3_LLC);
			/* Failure shouldn't ever happen this early */
			if (WARN_ON(ret))
				goto err_out;
		}
	}

	/* Default context will never have a file_priv */
	if (file_priv != NULL) {
		idr_preload(GFP_KERNEL);
		ret = idr_alloc(&file_priv->context_idr, ctx,
				DEFAULT_CONTEXT_ID, 0, GFP_KERNEL);
		idr_preload_end();
		if (ret < 0)
			goto err_out;
	} else
		ret = DEFAULT_CONTEXT_ID;

	ctx->file_priv = file_priv;
	ctx->id = ret;
	/* NB: Mark all slices as needing a remap so that when the context first
	 * loads it will restore whatever remap state already exists. If there
	 * is no remap info, it will be a NOP. */
	ctx->remap_slice = (1 << NUM_L3_SLICES(dev)) - 1;

	return ctx;

err_out:
	i915_gem_context_unreference(ctx);
	return ERR_PTR(ret);
}

/**
 * The default context needs to exist per ring that uses contexts. It stores the
 * context state of the GPU for applications that don't utilize HW contexts, as
 * well as an idle case.
 */
static struct i915_hw_context *
i915_gem_create_context(struct drm_device *dev,
			struct drm_i915_file_private *file_priv,
			bool create_vm)
{
	const bool is_global_default_ctx = file_priv == NULL;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_hw_context *ctx;
	int ret = 0;

	BUG_ON(!mutex_is_locked(&dev->struct_mutex));

	ctx = __create_hw_context(dev, file_priv);
	if (IS_ERR(ctx))
		return ctx;

	if (is_global_default_ctx && ctx->obj) {
		/* We may need to do things with the shrinker which
		 * require us to immediately switch back to the default
		 * context. This can cause a problem as pinning the
		 * default context also requires GTT space which may not
		 * be available. To avoid this we always pin the default
		 * context.
		 */
		ret = i915_gem_obj_ggtt_pin(ctx->obj,
					    get_context_alignment(dev), 0);
		if (ret) {
			DRM_DEBUG_DRIVER("Couldn't pin %d\n", ret);
			goto err_destroy;
		}
	}

	if (create_vm) {
		struct i915_hw_ppgtt *ppgtt = create_vm_for_ctx(dev, ctx);

		if (IS_ERR_OR_NULL(ppgtt)) {
			DRM_DEBUG_DRIVER("PPGTT setup failed (%ld)\n",
					 PTR_ERR(ppgtt));
			ret = PTR_ERR(ppgtt);
			goto err_unpin;
		} else
			ctx->vm = &ppgtt->base;

		/* This case is reserved for the global default context and
		 * should only happen once. */
		if (is_global_default_ctx) {
			if (WARN_ON(dev_priv->mm.aliasing_ppgtt)) {
				ret = -EEXIST;
				goto err_unpin;
			}

			dev_priv->mm.aliasing_ppgtt = ppgtt;
		}
	} else if (USES_PPGTT(dev)) {
		/* For platforms which only have aliasing PPGTT, we fake the
		 * address space and refcounting. */
		ctx->vm = &dev_priv->mm.aliasing_ppgtt->base;
		kref_get(&dev_priv->mm.aliasing_ppgtt->ref);
	} else
		ctx->vm = &dev_priv->gtt.base;

	return ctx;

err_unpin:
	if (is_global_default_ctx && ctx->obj)
		i915_gem_object_ggtt_unpin(ctx->obj);
err_destroy:
	i915_gem_context_unreference(ctx);
	return ERR_PTR(ret);
}

void i915_gem_context_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int i;

	/* Prevent the hardware from restoring the last context (which hung) on
	 * the next switch */
	for (i = 0; i < I915_NUM_RINGS; i++) {
		struct intel_ring_buffer *ring = &dev_priv->ring[i];
		struct i915_hw_context *dctx = ring->default_context;

		/* Do a fake switch to the default context */
		if (ring->last_context == dctx)
			continue;

		if (!ring->last_context)
			continue;

		if (dctx->obj && i == RCS) {
			WARN_ON(i915_gem_obj_ggtt_pin(dctx->obj,
						      get_context_alignment(dev), 0));
			/* Fake a finish/inactive */
			dctx->obj->base.write_domain = 0;
			dctx->obj->active = 0;
		}

		i915_gem_context_unreference(ring->last_context);
		i915_gem_context_reference(dctx);
		ring->last_context = dctx;
	}
}

int i915_gem_context_init(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_hw_context *ctx;
	int i;

	/* Init should only be called once per module load. Eventually the
	 * restriction on the context_disabled check can be loosened. */
	if (WARN_ON(dev_priv->ring[RCS].default_context))
		return 0;

	if (HAS_HW_CONTEXTS(dev)) {
		dev_priv->hw_context_size = round_up(get_context_size(dev), 4096);
		if (dev_priv->hw_context_size > (1<<20)) {
			DRM_DEBUG_DRIVER("Disabling HW Contexts; invalid size %d\n",
					 dev_priv->hw_context_size);
			dev_priv->hw_context_size = 0;
		}
	}

	ctx = i915_gem_create_context(dev, NULL, USES_PPGTT(dev));
	if (IS_ERR(ctx)) {
		DRM_ERROR("Failed to create default global context (error %ld)\n",
			  PTR_ERR(ctx));
		return PTR_ERR(ctx);
	}

	/* NB: RCS will hold a ref for all rings */
	for (i = 0; i < I915_NUM_RINGS; i++)
		dev_priv->ring[i].default_context = ctx;

	DRM_DEBUG_DRIVER("%s context support initialized\n", dev_priv->hw_context_size ? "HW" : "fake");
	return 0;
}

void i915_gem_context_fini(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_hw_context *dctx = dev_priv->ring[RCS].default_context;
	int i;

	if (dctx->obj) {
		/* The only known way to stop the gpu from accessing the hw context is
		 * to reset it. Do this as the very last operation to avoid confusing
		 * other code, leading to spurious errors. */
		intel_gpu_reset(dev);

		/* When default context is created and switched to, base object refcount
		 * will be 2 (+1 from object creation and +1 from do_switch()).
		 * i915_gem_context_fini() will be called after gpu_idle() has switched
		 * to default context. So we need to unreference the base object once
		 * to offset the do_switch part, so that i915_gem_context_unreference()
		 * can then free the base object correctly. */
		WARN_ON(!dev_priv->ring[RCS].last_context);
		if (dev_priv->ring[RCS].last_context == dctx) {
			/* Fake switch to NULL context */
			WARN_ON(dctx->obj->active);
			i915_gem_object_ggtt_unpin(dctx->obj);
			i915_gem_context_unreference(dctx);
			dev_priv->ring[RCS].last_context = NULL;
		}
	}

	for (i = 0; i < I915_NUM_RINGS; i++) {
		struct intel_ring_buffer *ring = &dev_priv->ring[i];

		if (ring->last_context)
			i915_gem_context_unreference(ring->last_context);

		ring->default_context = NULL;
		ring->last_context = NULL;
	}

	if (dctx->obj)
		i915_gem_object_ggtt_unpin(dctx->obj);
	i915_gem_context_unreference(dctx);
}

int i915_gem_context_enable(struct drm_i915_private *dev_priv)
{
	struct intel_ring_buffer *ring;
	int ret, i;

	/* This is the only place the aliasing PPGTT gets enabled, which means
	 * it has to happen before we bail on reset */
	if (dev_priv->mm.aliasing_ppgtt) {
		struct i915_hw_ppgtt *ppgtt = dev_priv->mm.aliasing_ppgtt;
		ppgtt->enable(ppgtt);
	}

	/* FIXME: We should make this work, even in reset */
	if (i915_reset_in_progress(&dev_priv->gpu_error))
		return 0;

	BUG_ON(!dev_priv->ring[RCS].default_context);

	for_each_ring(ring, dev_priv, i) {
		ret = i915_switch_context(ring, ring->default_context);
		if (ret)
			return ret;
	}

	return 0;
}

static int context_idr_cleanup(int id, void *p, void *data)
{
	struct i915_hw_context *ctx = p;

	/* Ignore the default context because close will handle it */
	if (i915_gem_context_is_default(ctx))
		return 0;

	i915_gem_context_unreference(ctx);
	return 0;
}

int i915_gem_context_open(struct drm_device *dev, struct drm_file *file)
{
	struct drm_i915_file_private *file_priv = file->driver_priv;

	idr_init(&file_priv->context_idr);

	mutex_lock(&dev->struct_mutex);
	file_priv->private_default_ctx =
		i915_gem_create_context(dev, file_priv, USES_FULL_PPGTT(dev));
	mutex_unlock(&dev->struct_mutex);

	if (IS_ERR(file_priv->private_default_ctx)) {
		idr_destroy(&file_priv->context_idr);
		return PTR_ERR(file_priv->private_default_ctx);
	}

	return 0;
}

void i915_gem_context_close(struct drm_device *dev, struct drm_file *file)
{
	struct drm_i915_file_private *file_priv = file->driver_priv;

	idr_for_each(&file_priv->context_idr, context_idr_cleanup, NULL);
	idr_destroy(&file_priv->context_idr);

	i915_gem_context_unreference(file_priv->private_default_ctx);
}

struct i915_hw_context *
i915_gem_context_get(struct drm_i915_file_private *file_priv, u32 id)
{
	struct i915_hw_context *ctx;

	ctx = (struct i915_hw_context *)idr_find(&file_priv->context_idr, id);
	if (!ctx)
		return ERR_PTR(-ENOENT);

	return ctx;
}

static inline int
mi_set_context(struct intel_ring_buffer *ring,
	       struct i915_hw_context *new_context,
	       u32 hw_flags)
{
	int ret;

	/* w/a: If Flush TLB Invalidation Mode is enabled, driver must do a TLB
	 * invalidation prior to MI_SET_CONTEXT. On GEN6 we don't set the value
	 * explicitly, so we rely on the value at ring init, stored in
	 * itlb_before_ctx_switch.
	 */
	if (IS_GEN6(ring->dev) && ring->itlb_before_ctx_switch) {
		ret = ring->flush(ring, I915_GEM_GPU_DOMAINS, 0);
		if (ret)
			return ret;
	}

	ret = intel_ring_begin(ring, 6);
	if (ret)
		return ret;

	/* WaProgramMiArbOnOffAroundMiSetContext:ivb,vlv,hsw */
	if (IS_GEN7(ring->dev))
		intel_ring_emit(ring, MI_ARB_ON_OFF | MI_ARB_DISABLE);
	else
		intel_ring_emit(ring, MI_NOOP);

	intel_ring_emit(ring, MI_NOOP);
	intel_ring_emit(ring, MI_SET_CONTEXT);
	intel_ring_emit(ring, i915_gem_obj_ggtt_offset(new_context->obj) |
			MI_MM_SPACE_GTT |
			MI_SAVE_EXT_STATE_EN |
			MI_RESTORE_EXT_STATE_EN |
			hw_flags);
	/*
	 * w/a: MI_SET_CONTEXT must always be followed by MI_NOOP
	 * WaMiSetContext_Hang:snb,ivb,vlv
	 */
	intel_ring_emit(ring, MI_NOOP);

	if (IS_GEN7(ring->dev))
		intel_ring_emit(ring, MI_ARB_ON_OFF | MI_ARB_ENABLE);
	else
		intel_ring_emit(ring, MI_NOOP);

	intel_ring_advance(ring);

	return ret;
}

static int do_switch(struct intel_ring_buffer *ring,
		     struct i915_hw_context *to)
{
	struct drm_i915_private *dev_priv = ring->dev->dev_private;
	struct i915_hw_context *from = ring->last_context;
	struct i915_hw_ppgtt *ppgtt = ctx_to_ppgtt(to);
	u32 hw_flags = 0;
	int ret, i;

	if (from != NULL && ring == &dev_priv->ring[RCS]) {
		BUG_ON(from->obj == NULL);
		BUG_ON(!i915_gem_obj_is_pinned(from->obj));
	}

	if (from == to && from->last_ring == ring && !to->remap_slice)
		return 0;

	/* Trying to pin first makes error handling easier. */
	if (ring == &dev_priv->ring[RCS]) {
		ret = i915_gem_obj_ggtt_pin(to->obj,
					    get_context_alignment(ring->dev), 0);
		if (ret)
			return ret;
	}

	/*
	 * Pin can switch back to the default context if we end up calling into
	 * evict_everything - as a last ditch gtt defrag effort that also
	 * switches to the default context. Hence we need to reload from here.
	 */
	from = ring->last_context;

	if (USES_FULL_PPGTT(ring->dev)) {
		ret = ppgtt->switch_mm(ppgtt, ring, false);
		if (ret)
			goto unpin_out;
	}

	if (ring != &dev_priv->ring[RCS]) {
		if (from)
			i915_gem_context_unreference(from);
		goto done;
	}

	/*
	 * Clear this page out of any CPU caches for coherent swap-in/out. Note
	 * that thanks to write = false in this call and us not setting any gpu
	 * write domains when putting a context object onto the active list
	 * (when switching away from it), this won't block.
	 *
	 * XXX: We need a real interface to do this instead of trickery.
	 */
	ret = i915_gem_object_set_to_gtt_domain(to->obj, false);
	if (ret)
		goto unpin_out;

	if (!to->obj->has_global_gtt_mapping) {
		struct i915_vma *vma = i915_gem_obj_to_vma(to->obj,
							   &dev_priv->gtt.base);
		vma->bind_vma(vma, to->obj->cache_level, GLOBAL_BIND);
	}

	if (!to->is_initialized || i915_gem_context_is_default(to))
		hw_flags |= MI_RESTORE_INHIBIT;

	ret = mi_set_context(ring, to, hw_flags);
	if (ret)
		goto unpin_out;

	for (i = 0; i < MAX_L3_SLICES; i++) {
		if (!(to->remap_slice & (1<<i)))
			continue;

		ret = i915_gem_l3_remap(ring, i);
		/* If it failed, try again next round */
		if (ret)
			DRM_DEBUG_DRIVER("L3 remapping failed\n");
		else
			to->remap_slice &= ~(1<<i);
	}

	/* The backing object for the context is done after switching to the
	 * *next* context. Therefore we cannot retire the previous context until
	 * the next context has already started running. In fact, the below code
	 * is a bit suboptimal because the retiring can occur simply after the
	 * MI_SET_CONTEXT instead of when the next seqno has completed.
	 */
	if (from != NULL) {
		from->obj->base.read_domains = I915_GEM_DOMAIN_INSTRUCTION;
		i915_vma_move_to_active(i915_gem_obj_to_ggtt(from->obj), ring);
		/* As long as MI_SET_CONTEXT is serializing, ie. it flushes the
		 * whole damn pipeline, we don't need to explicitly mark the
		 * object dirty. The only exception is that the context must be
		 * correct in case the object gets swapped out. Ideally we'd be
		 * able to defer doing this until we know the object would be
		 * swapped, but there is no way to do that yet.
		 */
		from->obj->dirty = 1;
		BUG_ON(from->obj->ring != ring);

		/* obj is kept alive until the next request by its active ref */
		i915_gem_object_ggtt_unpin(from->obj);
		i915_gem_context_unreference(from);
	}

	to->is_initialized = true;

done:
	i915_gem_context_reference(to);
	ring->last_context = to;
	to->last_ring = ring;

	return 0;

unpin_out:
	if (ring->id == RCS)
		i915_gem_object_ggtt_unpin(to->obj);
	return ret;
}

/**
 * i915_switch_context() - perform a GPU context switch.
 * @ring: ring for which we'll execute the context switch
 * @to: the context to switch to
 *
 * The context life cycle is simple. The context refcount is incremented and
 * decremented by 1 and create and destroy. If the context is in use by the GPU,
 * it will have a refoucnt > 1. This allows us to destroy the context abstract
 * object while letting the normal object tracking destroy the backing BO.
 */
int i915_switch_context(struct intel_ring_buffer *ring,
			struct i915_hw_context *to)
{
	struct drm_i915_private *dev_priv = ring->dev->dev_private;

	WARN_ON(!mutex_is_locked(&dev_priv->dev->struct_mutex));

	if (to->obj == NULL) { /* We have the fake context */
		if (to != ring->last_context) {
			i915_gem_context_reference(to);
			if (ring->last_context)
				i915_gem_context_unreference(ring->last_context);
			ring->last_context = to;
		}
		return 0;
	}

	return do_switch(ring, to);
}

static bool hw_context_enabled(struct drm_device *dev)
{
	return to_i915(dev)->hw_context_size;
}

int i915_gem_context_create_ioctl(struct drm_device *dev, void *data,
				  struct drm_file *file)
{
	struct drm_i915_gem_context_create *args = data;
	struct drm_i915_file_private *file_priv = file->driver_priv;
	struct i915_hw_context *ctx;
	int ret;

	if (!hw_context_enabled(dev))
		return -ENODEV;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	ctx = i915_gem_create_context(dev, file_priv, USES_FULL_PPGTT(dev));
	mutex_unlock(&dev->struct_mutex);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	args->ctx_id = ctx->id;
	DRM_DEBUG_DRIVER("HW context %d created\n", args->ctx_id);

	return 0;
}

int i915_gem_context_destroy_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file)
{
	struct drm_i915_gem_context_destroy *args = data;
	struct drm_i915_file_private *file_priv = file->driver_priv;
	struct i915_hw_context *ctx;
	int ret;

	if (args->ctx_id == DEFAULT_CONTEXT_ID)
		return -ENOENT;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	ctx = i915_gem_context_get(file_priv, args->ctx_id);
	if (IS_ERR(ctx)) {
		mutex_unlock(&dev->struct_mutex);
		return PTR_ERR(ctx);
	}

	idr_remove(&ctx->file_priv->context_idr, ctx->id);
	i915_gem_context_unreference(ctx);
	mutex_unlock(&dev->struct_mutex);

	DRM_DEBUG_DRIVER("HW context %d destroyed\n", args->ctx_id);
	return 0;
}
