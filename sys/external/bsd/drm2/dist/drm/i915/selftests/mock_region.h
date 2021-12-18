/*	$NetBSD: mock_region.h,v 1.2 2021/12/18 23:45:31 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __MOCK_REGION_H
#define __MOCK_REGION_H

#include <linux/types.h>

struct drm_i915_private;
struct intel_memory_region;

struct intel_memory_region *
mock_region_create(struct drm_i915_private *i915,
		   resource_size_t start,
		   resource_size_t size,
		   resource_size_t min_page_size,
		   resource_size_t io_start);

#endif /* !__MOCK_REGION_H */
