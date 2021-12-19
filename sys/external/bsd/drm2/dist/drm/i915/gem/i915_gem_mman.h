/*	$NetBSD: i915_gem_mman.h,v 1.8 2021/12/19 11:56:52 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef __I915_GEM_MMAN_H__
#define __I915_GEM_MMAN_H__

#include <linux/mm_types.h>
#include <linux/types.h>

struct drm_device;
struct drm_file;
struct drm_i915_gem_object;
struct file;
struct i915_mmap_offset;
struct mutex;

int i915_gem_mmap_gtt_version(void);
#ifdef __NetBSD__
extern const struct uvm_pagerops i915_gem_uvm_ops;
int i915_gem_mmap_object(struct drm_device *, off_t, size_t, int,
    struct uvm_object **, voff_t *, struct file *);
#else
int i915_gem_mmap(struct file *filp, struct vm_area_struct *vma);
#endif

int i915_gem_dumb_mmap_offset(struct drm_file *file_priv,
			      struct drm_device *dev,
			      u32 handle, u64 *offset);

void __i915_gem_object_release_mmap_gtt(struct drm_i915_gem_object *obj);
void i915_gem_object_release_mmap(struct drm_i915_gem_object *obj);
void i915_gem_object_release_mmap_offset(struct drm_i915_gem_object *obj);

#endif
