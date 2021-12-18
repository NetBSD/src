/*	$NetBSD: mock_gem_object.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2016 Intel Corporation
 */

#ifndef __MOCK_GEM_OBJECT_H__
#define __MOCK_GEM_OBJECT_H__

#include "gem/i915_gem_object_types.h"

struct mock_object {
	struct drm_i915_gem_object base;
};

#endif /* !__MOCK_GEM_OBJECT_H__ */
