/*	$NetBSD: i915_gem_pm.h,v 1.1.1.1 2021/12/18 20:15:31 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef __I915_GEM_PM_H__
#define __I915_GEM_PM_H__

#include <linux/types.h>

struct drm_i915_private;
struct work_struct;

void i915_gem_resume(struct drm_i915_private *i915);

void i915_gem_idle_work_handler(struct work_struct *work);

void i915_gem_suspend(struct drm_i915_private *i915);
void i915_gem_suspend_late(struct drm_i915_private *i915);

#endif /* __I915_GEM_PM_H__ */
