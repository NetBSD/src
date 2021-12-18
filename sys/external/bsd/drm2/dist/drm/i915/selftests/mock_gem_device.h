/*	$NetBSD: mock_gem_device.h,v 1.2 2021/12/18 23:45:31 riastradh Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MOCK_GEM_DEVICE_H__
#define __MOCK_GEM_DEVICE_H__

struct drm_i915_private;

struct drm_i915_private *mock_gem_device(void);
void mock_device_flush(struct drm_i915_private *i915);

#endif /* !__MOCK_GEM_DEVICE_H__ */
