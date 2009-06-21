/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/**
 * @file intel_bufmgr.h
 *
 * Public definitions of Intel-specific bufmgr functions.
 */

#ifndef INTEL_BUFMGR_H
#define INTEL_BUFMGR_H

#include <stdint.h>

typedef struct _drm_intel_bufmgr drm_intel_bufmgr;
typedef struct _drm_intel_bo drm_intel_bo;

struct _drm_intel_bo {
    /**
     * Size in bytes of the buffer object.
     *
     * The size may be larger than the size originally requested for the
     * allocation, such as being aligned to page size.
     */
    unsigned long size;
    /**
     * Alignment requirement for object
     *
     * Used for GTT mapping & pinning the object.
     */
    unsigned long align;

    /**
     * Card virtual address (offset from the beginning of the aperture) for the
     * object.  Only valid while validated.
     */
    unsigned long offset;
    /**
     * Virtual address for accessing the buffer data.  Only valid while mapped.
     */
    void *virtual;

    /** Buffer manager context associated with this buffer object */
    drm_intel_bufmgr *bufmgr;

    /**
     * MM-specific handle for accessing object
     */
    int handle;
};

drm_intel_bo *drm_intel_bo_alloc(drm_intel_bufmgr *bufmgr, const char *name,
				 unsigned long size, unsigned int alignment);
drm_intel_bo *drm_intel_bo_alloc_for_render(drm_intel_bufmgr *bufmgr,
					    const char *name,
					    unsigned long size,
					    unsigned int alignment);
void drm_intel_bo_reference(drm_intel_bo *bo);
void drm_intel_bo_unreference(drm_intel_bo *bo);
int drm_intel_bo_map(drm_intel_bo *bo, int write_enable);
int drm_intel_bo_unmap(drm_intel_bo *bo);

int drm_intel_bo_subdata(drm_intel_bo *bo, unsigned long offset,
		     unsigned long size, const void *data);
int drm_intel_bo_get_subdata(drm_intel_bo *bo, unsigned long offset,
			 unsigned long size, void *data);
void drm_intel_bo_wait_rendering(drm_intel_bo *bo);

void drm_intel_bufmgr_set_debug(drm_intel_bufmgr *bufmgr, int enable_debug);
void drm_intel_bufmgr_destroy(drm_intel_bufmgr *bufmgr);
int drm_intel_bo_exec(drm_intel_bo *bo, int used,
		      drm_clip_rect_t *cliprects, int num_cliprects,
		      int DR4);
int drm_intel_bufmgr_check_aperture_space(drm_intel_bo **bo_array, int count);

int drm_intel_bo_emit_reloc(drm_intel_bo *bo, uint32_t offset,
			    drm_intel_bo *target_bo, uint32_t target_offset,
			    uint32_t read_domains, uint32_t write_domain);
int drm_intel_bo_pin(drm_intel_bo *bo, uint32_t alignment);
int drm_intel_bo_unpin(drm_intel_bo *bo);
int drm_intel_bo_set_tiling(drm_intel_bo *bo, uint32_t *tiling_mode,
			    uint32_t stride);
int drm_intel_bo_get_tiling(drm_intel_bo *bo, uint32_t *tiling_mode,
			uint32_t *swizzle_mode);
int drm_intel_bo_flink(drm_intel_bo *bo, uint32_t *name);

int drm_intel_bo_disable_reuse(drm_intel_bo *bo);

/* drm_intel_bufmgr_gem.c */
drm_intel_bufmgr *drm_intel_bufmgr_gem_init(int fd, int batch_size);
drm_intel_bo *drm_intel_bo_gem_create_from_name(drm_intel_bufmgr *bufmgr,
						const char *name,
						unsigned int handle);
void drm_intel_bufmgr_gem_enable_reuse(drm_intel_bufmgr *bufmgr);
int drm_intel_gem_bo_map_gtt(drm_intel_bo *bo);
int drm_intel_gem_bo_unmap_gtt(drm_intel_bo *bo);
void drm_intel_gem_bo_start_gtt_access(drm_intel_bo *bo, int write_enable);

int drm_intel_get_pipe_from_crtc_id(drm_intel_bufmgr *bufmgr, int crtc_id);

/* drm_intel_bufmgr_fake.c */
drm_intel_bufmgr *drm_intel_bufmgr_fake_init(int fd,
					     unsigned long low_offset,
					     void *low_virtual,
					     unsigned long size,
					     volatile unsigned int *last_dispatch);
void drm_intel_bufmgr_fake_set_last_dispatch(drm_intel_bufmgr *bufmgr,
					     volatile unsigned int *last_dispatch);
void drm_intel_bufmgr_fake_set_exec_callback(drm_intel_bufmgr *bufmgr,
					     int (*exec)(drm_intel_bo *bo,
							 unsigned int used,
							 void *priv),
					     void *priv);
void drm_intel_bufmgr_fake_set_fence_callback(drm_intel_bufmgr *bufmgr,
					      unsigned int (*emit)(void *priv),
					      void (*wait)(unsigned int fence,
							   void *priv),
					      void *priv);
drm_intel_bo *drm_intel_bo_fake_alloc_static(drm_intel_bufmgr *bufmgr,
					     const char *name,
					     unsigned long offset, unsigned long size,
					     void *virtual);
void drm_intel_bo_fake_disable_backing_store(drm_intel_bo *bo,
					     void (*invalidate_cb)(drm_intel_bo *bo,
								   void *ptr),
					     void *ptr);

void drm_intel_bufmgr_fake_contended_lock_take(drm_intel_bufmgr *bufmgr);
void drm_intel_bufmgr_fake_evict_all(drm_intel_bufmgr *bufmgr);

/** @{ Compatibility defines to keep old code building despite the symbol rename
 * from dri_* to drm_intel_*
 */
#define dri_bo drm_intel_bo
#define dri_bufmgr drm_intel_bufmgr
#define dri_bo_alloc drm_intel_bo_alloc
#define dri_bo_reference drm_intel_bo_reference
#define dri_bo_unreference drm_intel_bo_unreference
#define dri_bo_map drm_intel_bo_map
#define dri_bo_unmap drm_intel_bo_unmap
#define dri_bo_subdata drm_intel_bo_subdata
#define dri_bo_get_subdata drm_intel_bo_get_subdata
#define dri_bo_wait_rendering drm_intel_bo_wait_rendering
#define dri_bufmgr_set_debug drm_intel_bufmgr_set_debug
#define dri_bufmgr_destroy drm_intel_bufmgr_destroy
#define dri_bo_exec drm_intel_bo_exec
#define dri_bufmgr_check_aperture_space drm_intel_bufmgr_check_aperture_space
#define dri_bo_emit_reloc(reloc_bo, read, write, target_offset,		\
			  reloc_offset, target_bo)			\
	drm_intel_bo_emit_reloc(reloc_bo, reloc_offset,			\
			    target_bo, target_offset,			\
			    read, write);
#define dri_bo_pin drm_intel_bo_pin
#define dri_bo_unpin drm_intel_bo_unpin
#define dri_bo_get_tiling drm_intel_bo_get_tiling
#define dri_bo_set_tiling(bo, mode) drm_intel_bo_set_tiling(bo, mode, 0)
#define dri_bo_flink drm_intel_bo_flink
#define intel_bufmgr_gem_init drm_intel_bufmgr_gem_init
#define intel_bo_gem_create_from_name drm_intel_bo_gem_create_from_name
#define intel_bufmgr_gem_enable_reuse drm_intel_bufmgr_gem_enable_reuse
#define intel_bufmgr_fake_init drm_intel_bufmgr_fake_init
#define intel_bufmgr_fake_set_last_dispatch drm_intel_bufmgr_fake_set_last_dispatch
#define intel_bufmgr_fake_set_exec_callback drm_intel_bufmgr_fake_set_exec_callback
#define intel_bufmgr_fake_set_fence_callback drm_intel_bufmgr_fake_set_fence_callback
#define intel_bo_fake_alloc_static drm_intel_bo_fake_alloc_static
#define intel_bo_fake_disable_backing_store drm_intel_bo_fake_disable_backing_store
#define intel_bufmgr_fake_contended_lock_take drm_intel_bufmgr_fake_contended_lock_take
#define intel_bufmgr_fake_evict_all drm_intel_bufmgr_fake_evict_all

/** @{ */

#endif /* INTEL_BUFMGR_H */

