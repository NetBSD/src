/*	$NetBSD: igt_mmap.h,v 1.2 2021/12/18 23:45:31 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef IGT_MMAP_H
#define IGT_MMAP_H

struct drm_i915_private;
struct drm_vma_offset_node;

unsigned long igt_mmap_node(struct drm_i915_private *i915,
			    struct drm_vma_offset_node *node,
			    unsigned long addr,
			    unsigned long prot,
			    unsigned long flags);

#endif /* IGT_MMAP_H */
