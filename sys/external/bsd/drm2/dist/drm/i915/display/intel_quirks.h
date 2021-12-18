/*	$NetBSD: intel_quirks.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_QUIRKS_H__
#define __INTEL_QUIRKS_H__

struct drm_i915_private;

void intel_init_quirks(struct drm_i915_private *dev_priv);

#endif /* __INTEL_QUIRKS_H__ */
