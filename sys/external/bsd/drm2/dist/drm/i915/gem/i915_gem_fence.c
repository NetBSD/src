/*	$NetBSD: i915_gem_fence.c,v 1.3 2021/12/19 11:30:56 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_gem_fence.c,v 1.3 2021/12/19 11:30:56 riastradh Exp $");

#include "i915_drv.h"
#include "i915_gem_object.h"

struct stub_fence {
	struct dma_fence dma;
	spinlock_t lock;
	struct i915_sw_fence chain;
};

static int __i915_sw_fence_call
stub_notify(struct i915_sw_fence *fence, enum i915_sw_fence_notify state)
{
	struct stub_fence *stub = container_of(fence, typeof(*stub), chain);

	switch (state) {
	case FENCE_COMPLETE:
		dma_fence_signal(&stub->dma);
		break;

	case FENCE_FREE:
		dma_fence_put(&stub->dma);
		break;
	}

	return NOTIFY_DONE;
}

static const char *stub_driver_name(struct dma_fence *fence)
{
	return DRIVER_NAME;
}

static const char *stub_timeline_name(struct dma_fence *fence)
{
	return "object";
}

static void stub_release(struct dma_fence *fence)
{
	struct stub_fence *stub = container_of(fence, typeof(*stub), dma);

	i915_sw_fence_fini(&stub->chain);

	BUILD_BUG_ON(offsetof(typeof(*stub), dma));
	dma_fence_free(&stub->dma);
	spin_lock_destroy(&stub->lock);
}

static const struct dma_fence_ops stub_fence_ops = {
	.get_driver_name = stub_driver_name,
	.get_timeline_name = stub_timeline_name,
	.release = stub_release,
};

struct dma_fence *
i915_gem_object_lock_fence(struct drm_i915_gem_object *obj)
{
	struct stub_fence *stub;

	assert_object_held(obj);

	stub = kmalloc(sizeof(*stub), GFP_KERNEL);
	if (!stub)
		return NULL;

	i915_sw_fence_init(&stub->chain, stub_notify);
	spin_lock_init(&stub->lock);
	dma_fence_init(&stub->dma, &stub_fence_ops, &stub->lock,
		       0, 0);

	if (i915_sw_fence_await_reservation(&stub->chain,
					    obj->base.resv, NULL,
					    true, I915_FENCE_TIMEOUT,
					    I915_FENCE_GFP) < 0)
		goto err;

	dma_resv_add_excl_fence(obj->base.resv, &stub->dma);

	return &stub->dma;

err:
	stub_release(&stub->dma);
	return NULL;
}

void i915_gem_object_unlock_fence(struct drm_i915_gem_object *obj,
				  struct dma_fence *fence)
{
	struct stub_fence *stub = container_of(fence, typeof(*stub), dma);

	i915_sw_fence_commit(&stub->chain);
}
