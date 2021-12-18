/*	$NetBSD: i915_sysfs.h,v 1.2 2021/12/18 23:45:28 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __I915_SYSFS_H__
#define __I915_SYSFS_H__

struct drm_i915_private;

void i915_setup_sysfs(struct drm_i915_private *i915);
void i915_teardown_sysfs(struct drm_i915_private *i915);

#endif /* __I915_SYSFS_H__ */
