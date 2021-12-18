/*	$NetBSD: i915_switcheroo.h,v 1.2 2021/12/18 23:45:28 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __I915_SWITCHEROO__
#define __I915_SWITCHEROO__

struct drm_i915_private;

int i915_switcheroo_register(struct drm_i915_private *i915);
void i915_switcheroo_unregister(struct drm_i915_private *i915);

#endif /* __I915_SWITCHEROO__ */
