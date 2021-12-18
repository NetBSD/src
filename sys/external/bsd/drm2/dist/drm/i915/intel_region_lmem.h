/*	$NetBSD: intel_region_lmem.h,v 1.2 2021/12/18 23:45:28 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_REGION_LMEM_H
#define __INTEL_REGION_LMEM_H

struct drm_i915_private;

extern const struct intel_memory_region_ops intel_region_lmem_ops;

struct intel_memory_region *
intel_setup_fake_lmem(struct drm_i915_private *i915);

#endif /* !__INTEL_REGION_LMEM_H */
