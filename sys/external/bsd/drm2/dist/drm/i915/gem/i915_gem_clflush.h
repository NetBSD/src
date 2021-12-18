/*	$NetBSD: i915_gem_clflush.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2016 Intel Corporation
 */

#ifndef __I915_GEM_CLFLUSH_H__
#define __I915_GEM_CLFLUSH_H__

#include <linux/types.h>

struct drm_i915_private;
struct drm_i915_gem_object;

bool i915_gem_clflush_object(struct drm_i915_gem_object *obj,
			     unsigned int flags);
#define I915_CLFLUSH_FORCE BIT(0)
#define I915_CLFLUSH_SYNC BIT(1)

#endif /* __I915_GEM_CLFLUSH_H__ */
