/*	$NetBSD: igt_flush_test.h,v 1.2 2021/12/18 23:45:31 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2018 Intel Corporation
 */

#ifndef IGT_FLUSH_TEST_H
#define IGT_FLUSH_TEST_H

struct drm_i915_private;

int igt_flush_test(struct drm_i915_private *i915);

#endif /* IGT_FLUSH_TEST_H */
