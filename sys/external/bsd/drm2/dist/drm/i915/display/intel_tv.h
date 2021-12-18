/*	$NetBSD: intel_tv.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_TV_H__
#define __INTEL_TV_H__

struct drm_i915_private;

void intel_tv_init(struct drm_i915_private *dev_priv);

#endif /* __INTEL_TV_H__ */
