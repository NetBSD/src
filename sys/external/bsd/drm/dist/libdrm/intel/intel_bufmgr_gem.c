/**************************************************************************
 *
 * Copyright © 2007 Red Hat Inc.
 * Copyright © 2007 Intel Corporation
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 *          Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 *	    Eric Anholt <eric@anholt.net>
 *	    Dave Airlie <airlied@linux.ie>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xf86drm.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "errno.h"
#include "libdrm_lists.h"
#include "intel_bufmgr.h"
#include "intel_bufmgr_priv.h"
#include "intel_chipset.h"
#include "string.h"

#include "i915_drm.h"

#define DBG(...) do {					\
   if (bufmgr_gem->bufmgr.debug)			\
      fprintf(stderr, __VA_ARGS__);			\
} while (0)

typedef struct _drm_intel_bo_gem drm_intel_bo_gem;

struct drm_intel_gem_bo_bucket {
   drmMMListHead head;

   /**
    * Limit on the number of entries in this bucket.
    *
    * 0 means that this caching at this bucket size is disabled.
    * -1 means that there is no limit to caching at this size.
    */
   int max_entries;
   int num_entries;
};

/* Only cache objects up to 64MB.  Bigger than that, and the rounding of the
 * size makes many operations fail that wouldn't otherwise.
 */
#define DRM_INTEL_GEM_BO_BUCKETS	14
typedef struct _drm_intel_bufmgr_gem {
    drm_intel_bufmgr bufmgr;

    int fd;

    int max_relocs;

    pthread_mutex_t lock;

    struct drm_i915_gem_exec_object *exec_objects;
    drm_intel_bo **exec_bos;
    int exec_size;
    int exec_count;

    /** Array of lists of cached gem objects of power-of-two sizes */
    struct drm_intel_gem_bo_bucket cache_bucket[DRM_INTEL_GEM_BO_BUCKETS];

    uint64_t gtt_size;
    int available_fences;
    int pci_device;
} drm_intel_bufmgr_gem;

struct _drm_intel_bo_gem {
    drm_intel_bo bo;

    int refcount;
    /** Boolean whether the mmap ioctl has been called for this buffer yet. */
    uint32_t gem_handle;
    const char *name;

    /**
     * Kenel-assigned global name for this object
     */
    unsigned int global_name;
    
    /**
     * Index of the buffer within the validation list while preparing a
     * batchbuffer execution.
     */
    int validate_index;

    /**
     * Boolean whether we've started swrast
     * Set when the buffer has been mapped
     * Cleared when the buffer is unmapped
     */
    int swrast;

    /**
     * Current tiling mode
     */
    uint32_t tiling_mode;
    uint32_t swizzle_mode;

    /** Array passed to the DRM containing relocation information. */
    struct drm_i915_gem_relocation_entry *relocs;
    /** Array of bos corresponding to relocs[i].target_handle */
    drm_intel_bo **reloc_target_bo;
    /** Number of entries in relocs */
    int reloc_count;
    /** Mapped address for the buffer, saved across map/unmap cycles */
    void *mem_virtual;
    /** GTT virtual address for the buffer, saved across map/unmap cycles */
    void *gtt_virtual;

    /** BO cache list */
    drmMMListHead head;

    /**
     * Boolean of whether this BO and its children have been included in
     * the current drm_intel_bufmgr_check_aperture_space() total.
     */
    char included_in_check_aperture;

    /**
     * Boolean of whether this buffer has been used as a relocation
     * target and had its size accounted for, and thus can't have any
     * further relocations added to it.
     */
     char used_as_reloc_target;

    /**
     * Boolean of whether this buffer can be re-used
     */
    char reusable;

    /**
     * Size in bytes of this buffer and its relocation descendents.
     *
     * Used to avoid costly tree walking in drm_intel_bufmgr_check_aperture in
     * the common case.
     */
    int reloc_tree_size;
    /**
     * Number of potential fence registers required by this buffer and its
     * relocations.
     */
    int reloc_tree_fences;
};

static void drm_intel_gem_bo_reference_locked(drm_intel_bo *bo);

static unsigned int
drm_intel_gem_estimate_batch_space(drm_intel_bo **bo_array, int count);

static unsigned int
drm_intel_gem_compute_batch_space(drm_intel_bo **bo_array, int count);

static int
drm_intel_gem_bo_get_tiling(drm_intel_bo *bo, uint32_t *tiling_mode,
			    uint32_t *swizzle_mode);

static int
drm_intel_gem_bo_set_tiling(drm_intel_bo *bo, uint32_t *tiling_mode,
			    uint32_t stride);

static void
drm_intel_gem_bo_unreference(drm_intel_bo *bo);

static int
logbase2(int n)
{
   int i = 1;
   int log2 = 0;

   while (n > i) {
      i *= 2;
      log2++;
   }

   return log2;
}

static struct drm_intel_gem_bo_bucket *
drm_intel_gem_bo_bucket_for_size(drm_intel_bufmgr_gem *bufmgr_gem,
				 unsigned long size)
{
    int i;

    /* We only do buckets in power of two increments */
    if ((size & (size - 1)) != 0)
	return NULL;

    /* We should only see sizes rounded to pages. */
    assert((size % 4096) == 0);

    /* We always allocate in units of pages */
    i = ffs(size / 4096) - 1;
    if (i >= DRM_INTEL_GEM_BO_BUCKETS)
	return NULL;

    return &bufmgr_gem->cache_bucket[i];
}


static void drm_intel_gem_dump_validation_list(drm_intel_bufmgr_gem *bufmgr_gem)
{
    int i, j;

    for (i = 0; i < bufmgr_gem->exec_count; i++) {
	drm_intel_bo *bo = bufmgr_gem->exec_bos[i];
	drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

	if (bo_gem->relocs == NULL) {
	    DBG("%2d: %d (%s)\n", i, bo_gem->gem_handle, bo_gem->name);
	    continue;
	}

	for (j = 0; j < bo_gem->reloc_count; j++) {
	    drm_intel_bo *target_bo = bo_gem->reloc_target_bo[j];
	    drm_intel_bo_gem *target_gem = (drm_intel_bo_gem *)target_bo;

	    DBG("%2d: %d (%s)@0x%08llx -> %d (%s)@0x%08lx + 0x%08x\n",
		i,
		bo_gem->gem_handle, bo_gem->name,
		(unsigned long long)bo_gem->relocs[j].offset,
		target_gem->gem_handle, target_gem->name, target_bo->offset,
		bo_gem->relocs[j].delta);
	}
    }
}

/**
 * Adds the given buffer to the list of buffers to be validated (moved into the
 * appropriate memory type) with the next batch submission.
 *
 * If a buffer is validated multiple times in a batch submission, it ends up
 * with the intersection of the memory type flags and the union of the
 * access flags.
 */
static void
drm_intel_add_validate_buffer(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    int index;

    if (bo_gem->validate_index != -1)
	return;

    /* Extend the array of validation entries as necessary. */
    if (bufmgr_gem->exec_count == bufmgr_gem->exec_size) {
	int new_size = bufmgr_gem->exec_size * 2;

	if (new_size == 0)
	    new_size = 5;

	bufmgr_gem->exec_objects =
	    realloc(bufmgr_gem->exec_objects,
		    sizeof(*bufmgr_gem->exec_objects) * new_size);
	bufmgr_gem->exec_bos =
	    realloc(bufmgr_gem->exec_bos,
		    sizeof(*bufmgr_gem->exec_bos) * new_size);
	bufmgr_gem->exec_size = new_size;
    }

    index = bufmgr_gem->exec_count;
    bo_gem->validate_index = index;
    /* Fill in array entry */
    bufmgr_gem->exec_objects[index].handle = bo_gem->gem_handle;
    bufmgr_gem->exec_objects[index].relocation_count = bo_gem->reloc_count;
    bufmgr_gem->exec_objects[index].relocs_ptr = (uintptr_t)bo_gem->relocs;
    bufmgr_gem->exec_objects[index].alignment = 0;
    bufmgr_gem->exec_objects[index].offset = 0;
    bufmgr_gem->exec_bos[index] = bo;
    drm_intel_gem_bo_reference_locked(bo);
    bufmgr_gem->exec_count++;
}


#define RELOC_BUF_SIZE(x) ((I915_RELOC_HEADER + x * I915_RELOC0_STRIDE) * \
	sizeof(uint32_t))

static int
drm_intel_setup_reloc_list(drm_intel_bo *bo)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;

    bo_gem->relocs = malloc(bufmgr_gem->max_relocs *
			    sizeof(struct drm_i915_gem_relocation_entry));
    bo_gem->reloc_target_bo = malloc(bufmgr_gem->max_relocs *
				     sizeof(drm_intel_bo *));

    return 0;
}

static drm_intel_bo *
drm_intel_gem_bo_alloc_internal(drm_intel_bufmgr *bufmgr, const char *name,
				unsigned long size, unsigned int alignment,
				int for_render)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bufmgr;
    drm_intel_bo_gem *bo_gem;
    unsigned int page_size = getpagesize();
    int ret;
    struct drm_intel_gem_bo_bucket *bucket;
    int alloc_from_cache = 0;
    unsigned long bo_size;

    /* Round the allocated size up to a power of two number of pages. */
    bo_size = 1 << logbase2(size);
    if (bo_size < page_size)
	bo_size = page_size;
    bucket = drm_intel_gem_bo_bucket_for_size(bufmgr_gem, bo_size);

    /* If we don't have caching at this size, don't actually round the
     * allocation up.
     */
    if (bucket == NULL || bucket->max_entries == 0) {
	bo_size = size;
	if (bo_size < page_size)
	    bo_size = page_size;
    }

    pthread_mutex_lock(&bufmgr_gem->lock);
    /* Get a buffer out of the cache if available */
    if (bucket != NULL && bucket->num_entries > 0) {
	struct drm_i915_gem_busy busy;

	if (for_render) {
	    /* Allocate new render-target BOs from the tail (MRU)
	     * of the list, as it will likely be hot in the GPU cache
	     * and in the aperture for us.
	     */
	    bo_gem = DRMLISTENTRY(drm_intel_bo_gem, bucket->head.prev, head);
	    DRMLISTDEL(&bo_gem->head);
	    bucket->num_entries--;
	    alloc_from_cache = 1;
	} else {
	    /* For non-render-target BOs (where we're probably going to map it
	     * first thing in order to fill it with data), check if the
	     * last BO in the cache is unbusy, and only reuse in that case.
	     * Otherwise, allocating a new buffer is probably faster than
	     * waiting for the GPU to finish.
	     */
	    bo_gem = DRMLISTENTRY(drm_intel_bo_gem, bucket->head.next, head);

	    memset(&busy, 0, sizeof(busy));
	    busy.handle = bo_gem->gem_handle;

	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_BUSY, &busy);
	    alloc_from_cache = (ret == 0 && busy.busy == 0);

	    if (alloc_from_cache) {
		DRMLISTDEL(&bo_gem->head);
		bucket->num_entries--;
	    }
	}
    }
    pthread_mutex_unlock(&bufmgr_gem->lock);

    if (!alloc_from_cache) {
	struct drm_i915_gem_create create;

	bo_gem = calloc(1, sizeof(*bo_gem));
	if (!bo_gem)
	    return NULL;

	bo_gem->bo.size = bo_size;
	memset(&create, 0, sizeof(create));
	create.size = bo_size;

	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	bo_gem->gem_handle = create.handle;
	bo_gem->bo.handle = bo_gem->gem_handle;
	if (ret != 0) {
	    free(bo_gem);
	    return NULL;
	}
	bo_gem->bo.bufmgr = bufmgr;
    }

    bo_gem->name = name;
    bo_gem->refcount = 1;
    bo_gem->validate_index = -1;
    bo_gem->reloc_tree_size = bo_gem->bo.size;
    bo_gem->reloc_tree_fences = 0;
    bo_gem->used_as_reloc_target = 0;
    bo_gem->tiling_mode = I915_TILING_NONE;
    bo_gem->swizzle_mode = I915_BIT_6_SWIZZLE_NONE;
    bo_gem->reusable = 1;

    DBG("bo_create: buf %d (%s) %ldb\n",
	bo_gem->gem_handle, bo_gem->name, size);

    return &bo_gem->bo;
}

static drm_intel_bo *
drm_intel_gem_bo_alloc_for_render(drm_intel_bufmgr *bufmgr, const char *name,
				  unsigned long size, unsigned int alignment)
{
    return drm_intel_gem_bo_alloc_internal(bufmgr, name, size, alignment, 1);
}

static drm_intel_bo *
drm_intel_gem_bo_alloc(drm_intel_bufmgr *bufmgr, const char *name,
		       unsigned long size, unsigned int alignment)
{
    return drm_intel_gem_bo_alloc_internal(bufmgr, name, size, alignment, 0);
}

/**
 * Returns a drm_intel_bo wrapping the given buffer object handle.
 *
 * This can be used when one application needs to pass a buffer object
 * to another.
 */
drm_intel_bo *
drm_intel_bo_gem_create_from_name(drm_intel_bufmgr *bufmgr, const char *name,
				  unsigned int handle)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bufmgr;
    drm_intel_bo_gem *bo_gem;
    int ret;
    struct drm_gem_open open_arg;
    struct drm_i915_gem_get_tiling get_tiling;

    bo_gem = calloc(1, sizeof(*bo_gem));
    if (!bo_gem)
	return NULL;

    memset(&open_arg, 0, sizeof(open_arg));
    open_arg.name = handle;
    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_OPEN, &open_arg);
    if (ret != 0) {
	fprintf(stderr, "Couldn't reference %s handle 0x%08x: %s\n",
	       name, handle, strerror(errno));
	free(bo_gem);
	return NULL;
    }
    bo_gem->bo.size = open_arg.size;
    bo_gem->bo.offset = 0;
    bo_gem->bo.virtual = NULL;
    bo_gem->bo.bufmgr = bufmgr;
    bo_gem->name = name;
    bo_gem->refcount = 1;
    bo_gem->validate_index = -1;
    bo_gem->gem_handle = open_arg.handle;
    bo_gem->global_name = handle;
    bo_gem->reusable = 0;

    memset(&get_tiling, 0, sizeof(get_tiling));
    get_tiling.handle = bo_gem->gem_handle;
    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_GET_TILING, &get_tiling);
    if (ret != 0) {
	drm_intel_gem_bo_unreference(&bo_gem->bo);
	return NULL;
    }
    bo_gem->tiling_mode = get_tiling.tiling_mode;
    bo_gem->swizzle_mode = get_tiling.swizzle_mode;
    if (bo_gem->tiling_mode == I915_TILING_NONE)
	bo_gem->reloc_tree_fences = 0;
    else
	bo_gem->reloc_tree_fences = 1;

    DBG("bo_create_from_handle: %d (%s)\n", handle, bo_gem->name);

    return &bo_gem->bo;
}

static void
drm_intel_gem_bo_reference(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

    assert(bo_gem->refcount > 0);
    pthread_mutex_lock(&bufmgr_gem->lock);
    bo_gem->refcount++;
    pthread_mutex_unlock(&bufmgr_gem->lock);
}

static void
drm_intel_gem_bo_reference_locked(drm_intel_bo *bo)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

    assert(bo_gem->refcount > 0);
    bo_gem->refcount++;
}

static void
drm_intel_gem_bo_free(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_gem_close close;
    int ret;

    if (bo_gem->mem_virtual)
	munmap (bo_gem->mem_virtual, bo_gem->bo.size);
    if (bo_gem->gtt_virtual)
	munmap (bo_gem->gtt_virtual, bo_gem->bo.size);

    /* Close this object */
    memset(&close, 0, sizeof(close));
    close.handle = bo_gem->gem_handle;
    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_CLOSE, &close);
    if (ret != 0) {
	fprintf(stderr,
		"DRM_IOCTL_GEM_CLOSE %d failed (%s): %s\n",
		bo_gem->gem_handle, bo_gem->name, strerror(errno));
    }
    free(bo);
}

static void
drm_intel_gem_bo_unreference_locked(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

    assert(bo_gem->refcount > 0);
    if (--bo_gem->refcount == 0) {
	struct drm_intel_gem_bo_bucket *bucket;
	uint32_t tiling_mode;

	if (bo_gem->relocs != NULL) {
	    int i;

	    /* Unreference all the target buffers */
	    for (i = 0; i < bo_gem->reloc_count; i++)
		 drm_intel_gem_bo_unreference_locked(bo_gem->reloc_target_bo[i]);
	    free(bo_gem->reloc_target_bo);
	    free(bo_gem->relocs);
	}

	DBG("bo_unreference final: %d (%s)\n",
	    bo_gem->gem_handle, bo_gem->name);

	bucket = drm_intel_gem_bo_bucket_for_size(bufmgr_gem, bo->size);
	/* Put the buffer into our internal cache for reuse if we can. */
	tiling_mode = I915_TILING_NONE;
	if (bo_gem->reusable &&
	    bucket != NULL &&
	    (bucket->max_entries == -1 ||
	     (bucket->max_entries > 0 &&
	      bucket->num_entries < bucket->max_entries)) &&
	    drm_intel_gem_bo_set_tiling(bo, &tiling_mode, 0) == 0)
	{
	    bo_gem->name = NULL;
	    bo_gem->validate_index = -1;
	    bo_gem->relocs = NULL;
	    bo_gem->reloc_target_bo = NULL;
	    bo_gem->reloc_count = 0;

	    DRMLISTADDTAIL(&bo_gem->head, &bucket->head);
	    bucket->num_entries++;
	} else {
	    drm_intel_gem_bo_free(bo);
	}
    }
}

static void
drm_intel_gem_bo_unreference(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;

    pthread_mutex_lock(&bufmgr_gem->lock);
    drm_intel_gem_bo_unreference_locked(bo);
    pthread_mutex_unlock(&bufmgr_gem->lock);
}

static int
drm_intel_gem_bo_map(drm_intel_bo *bo, int write_enable)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_set_domain set_domain;
    int ret;

    pthread_mutex_lock(&bufmgr_gem->lock);

    /* Allow recursive mapping. Mesa may recursively map buffers with
     * nested display loops.
     */
    if (!bo_gem->mem_virtual) {
	struct drm_i915_gem_mmap mmap_arg;

	DBG("bo_map: %d (%s)\n", bo_gem->gem_handle, bo_gem->name);

	memset(&mmap_arg, 0, sizeof(mmap_arg));
	mmap_arg.handle = bo_gem->gem_handle;
	mmap_arg.offset = 0;
	mmap_arg.size = bo->size;
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg);
	if (ret != 0) {
	    fprintf(stderr, "%s:%d: Error mapping buffer %d (%s): %s .\n",
		    __FILE__, __LINE__,
		    bo_gem->gem_handle, bo_gem->name, strerror(errno));
	    pthread_mutex_unlock(&bufmgr_gem->lock);
	    return ret;
	}
	bo_gem->mem_virtual = (void *)(uintptr_t)mmap_arg.addr_ptr;
	bo_gem->swrast = 0;
    }
    DBG("bo_map: %d (%s) -> %p\n", bo_gem->gem_handle, bo_gem->name,
	bo_gem->mem_virtual);
    bo->virtual = bo_gem->mem_virtual;

    if (bo_gem->global_name != 0 || !bo_gem->swrast) {
	set_domain.handle = bo_gem->gem_handle;
	set_domain.read_domains = I915_GEM_DOMAIN_CPU;
	if (write_enable)
	    set_domain.write_domain = I915_GEM_DOMAIN_CPU;
	else
	    set_domain.write_domain = 0;
	do {
	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_SET_DOMAIN,
			&set_domain);
	} while (ret == -1 && errno == EINTR);
	if (ret != 0) {
	    fprintf (stderr, "%s:%d: Error setting swrast %d: %s\n",
		     __FILE__, __LINE__, bo_gem->gem_handle, strerror (errno));
	    pthread_mutex_unlock(&bufmgr_gem->lock);
	    return ret;
	}
	bo_gem->swrast = 1;
    }

    pthread_mutex_unlock(&bufmgr_gem->lock);

    return 0;
}

int
drm_intel_gem_bo_map_gtt(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_set_domain set_domain;
    int ret;

    pthread_mutex_lock(&bufmgr_gem->lock);

    /* Get a mapping of the buffer if we haven't before. */
    if (bo_gem->gtt_virtual == NULL) {
	struct drm_i915_gem_mmap_gtt mmap_arg;

	DBG("bo_map_gtt: %d (%s)\n", bo_gem->gem_handle, bo_gem->name);

	memset(&mmap_arg, 0, sizeof(mmap_arg));
	mmap_arg.handle = bo_gem->gem_handle;

	/* Get the fake offset back... */
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &mmap_arg);
	if (ret != 0) {
	    fprintf(stderr,
		    "%s:%d: Error preparing buffer map %d (%s): %s .\n",
		    __FILE__, __LINE__,
		    bo_gem->gem_handle, bo_gem->name,
		    strerror(errno));
	    pthread_mutex_unlock(&bufmgr_gem->lock);
	    return ret;
	}

	/* and mmap it */
	bo_gem->gtt_virtual = mmap(0, bo->size, PROT_READ | PROT_WRITE,
				   MAP_SHARED, bufmgr_gem->fd,
				   mmap_arg.offset);
	if (bo_gem->gtt_virtual == MAP_FAILED) {
	    fprintf(stderr,
		    "%s:%d: Error mapping buffer %d (%s): %s .\n",
		    __FILE__, __LINE__,
		    bo_gem->gem_handle, bo_gem->name,
		    strerror(errno));
	    pthread_mutex_unlock(&bufmgr_gem->lock);
	    return errno;
	}
    }

    bo->virtual = bo_gem->gtt_virtual;

    DBG("bo_map: %d (%s) -> %p\n", bo_gem->gem_handle, bo_gem->name,
	bo_gem->gtt_virtual);

    /* Now move it to the GTT domain so that the CPU caches are flushed */
    set_domain.handle = bo_gem->gem_handle;
    set_domain.read_domains = I915_GEM_DOMAIN_GTT;
    set_domain.write_domain = I915_GEM_DOMAIN_GTT;
    do {
	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_SET_DOMAIN,
			&set_domain);
    } while (ret == -1 && errno == EINTR);

    if (ret != 0) {
	    fprintf (stderr, "%s:%d: Error setting domain %d: %s\n",
		     __FILE__, __LINE__, bo_gem->gem_handle, strerror (errno));
    }

    pthread_mutex_unlock(&bufmgr_gem->lock);

    return 0;
}

int
drm_intel_gem_bo_unmap_gtt(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    int ret = 0;

    if (bo == NULL)
	return 0;

    assert(bo_gem->gtt_virtual != NULL);

    pthread_mutex_lock(&bufmgr_gem->lock);
    bo->virtual = NULL;
    pthread_mutex_unlock(&bufmgr_gem->lock);

    return ret;
}

static int
drm_intel_gem_bo_unmap(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_sw_finish sw_finish;
    int ret;

    if (bo == NULL)
	return 0;

    assert(bo_gem->mem_virtual != NULL);

    pthread_mutex_lock(&bufmgr_gem->lock);
    if (bo_gem->swrast) {
	sw_finish.handle = bo_gem->gem_handle;
	do {
	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_SW_FINISH,
			&sw_finish);
	} while (ret == -1 && errno == EINTR);
	bo_gem->swrast = 0;
    }
    bo->virtual = NULL;
    pthread_mutex_unlock(&bufmgr_gem->lock);
    return 0;
}

static int
drm_intel_gem_bo_subdata (drm_intel_bo *bo, unsigned long offset,
			  unsigned long size, const void *data)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_pwrite pwrite;
    int ret;

    memset (&pwrite, 0, sizeof (pwrite));
    pwrite.handle = bo_gem->gem_handle;
    pwrite.offset = offset;
    pwrite.size = size;
    pwrite.data_ptr = (uint64_t) (uintptr_t) data;
    do {
	ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite);
    } while (ret == -1 && errno == EINTR);
    if (ret != 0) {
	fprintf (stderr, "%s:%d: Error writing data to buffer %d: (%d %d) %s .\n",
		 __FILE__, __LINE__,
		 bo_gem->gem_handle, (int) offset, (int) size,
		 strerror (errno));
    }
    return 0;
}

static int
drm_intel_gem_get_pipe_from_crtc_id (drm_intel_bufmgr *bufmgr, int crtc_id)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *) bufmgr;
    struct drm_i915_get_pipe_from_crtc_id get_pipe_from_crtc_id;
    int ret;

    get_pipe_from_crtc_id.crtc_id = crtc_id;
    ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_I915_GET_PIPE_FROM_CRTC_ID,
		 &get_pipe_from_crtc_id);
    if (ret != 0) {
	/* We return -1 here to signal that we don't
	 * know which pipe is associated with this crtc.
	 * This lets the caller know that this information
	 * isn't available; using the wrong pipe for
	 * vblank waiting can cause the chipset to lock up
	 */
	return -1;
    }

    return get_pipe_from_crtc_id.pipe;
}

static int
drm_intel_gem_bo_get_subdata (drm_intel_bo *bo, unsigned long offset,
			      unsigned long size, void *data)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_pread pread;
    int ret;

    memset (&pread, 0, sizeof (pread));
    pread.handle = bo_gem->gem_handle;
    pread.offset = offset;
    pread.size = size;
    pread.data_ptr = (uint64_t) (uintptr_t) data;
    do {
	ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_I915_GEM_PREAD, &pread);
    } while (ret == -1 && errno == EINTR);
    if (ret != 0) {
	fprintf (stderr, "%s:%d: Error reading data from buffer %d: (%d %d) %s .\n",
		 __FILE__, __LINE__,
		 bo_gem->gem_handle, (int) offset, (int) size,
		 strerror (errno));
    }
    return 0;
}

/** Waits for all GPU rendering to the object to have completed. */
static void
drm_intel_gem_bo_wait_rendering(drm_intel_bo *bo)
{
    return drm_intel_gem_bo_start_gtt_access(bo, 0);
}

/**
 * Sets the object to the GTT read and possibly write domain, used by the X
 * 2D driver in the absence of kernel support to do drm_intel_gem_bo_map_gtt().
 *
 * In combination with drm_intel_gem_bo_pin() and manual fence management, we
 * can do tiled pixmaps this way.
 */
void
drm_intel_gem_bo_start_gtt_access(drm_intel_bo *bo, int write_enable)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_set_domain set_domain;
    int ret;

    set_domain.handle = bo_gem->gem_handle;
    set_domain.read_domains = I915_GEM_DOMAIN_GTT;
    set_domain.write_domain = write_enable ? I915_GEM_DOMAIN_GTT : 0;
    do {
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    } while (ret == -1 && errno == EINTR);
    if (ret != 0) {
	fprintf (stderr, "%s:%d: Error setting memory domains %d (%08x %08x): %s .\n",
		 __FILE__, __LINE__,
		 bo_gem->gem_handle, set_domain.read_domains, set_domain.write_domain,
		 strerror (errno));
    }
}

static void
drm_intel_bufmgr_gem_destroy(drm_intel_bufmgr *bufmgr)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bufmgr;
    int i;

    free(bufmgr_gem->exec_objects);
    free(bufmgr_gem->exec_bos);

    pthread_mutex_destroy(&bufmgr_gem->lock);

    /* Free any cached buffer objects we were going to reuse */
    for (i = 0; i < DRM_INTEL_GEM_BO_BUCKETS; i++) {
	struct drm_intel_gem_bo_bucket *bucket = &bufmgr_gem->cache_bucket[i];
	drm_intel_bo_gem *bo_gem;

	while (!DRMLISTEMPTY(&bucket->head)) {
	    bo_gem = DRMLISTENTRY(drm_intel_bo_gem, bucket->head.next, head);
	    DRMLISTDEL(&bo_gem->head);
	    bucket->num_entries--;

	    drm_intel_gem_bo_free(&bo_gem->bo);
	}
    }

    free(bufmgr);
}

/**
 * Adds the target buffer to the validation list and adds the relocation
 * to the reloc_buffer's relocation list.
 *
 * The relocation entry at the given offset must already contain the
 * precomputed relocation value, because the kernel will optimize out
 * the relocation entry write when the buffer hasn't moved from the
 * last known offset in target_bo.
 */
static int
drm_intel_gem_bo_emit_reloc(drm_intel_bo *bo, uint32_t offset,
			    drm_intel_bo *target_bo, uint32_t target_offset,
			    uint32_t read_domains, uint32_t write_domain)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    drm_intel_bo_gem *target_bo_gem = (drm_intel_bo_gem *)target_bo;

    pthread_mutex_lock(&bufmgr_gem->lock);

    /* Create a new relocation list if needed */
    if (bo_gem->relocs == NULL)
	drm_intel_setup_reloc_list(bo);

    /* Check overflow */
    assert(bo_gem->reloc_count < bufmgr_gem->max_relocs);

    /* Check args */
    assert (offset <= bo->size - 4);
    assert ((write_domain & (write_domain-1)) == 0);

    /* Make sure that we're not adding a reloc to something whose size has
     * already been accounted for.
     */
    assert(!bo_gem->used_as_reloc_target);
    bo_gem->reloc_tree_size += target_bo_gem->reloc_tree_size;
    bo_gem->reloc_tree_fences += target_bo_gem->reloc_tree_fences;

    /* Flag the target to disallow further relocations in it. */
    target_bo_gem->used_as_reloc_target = 1;

    bo_gem->relocs[bo_gem->reloc_count].offset = offset;
    bo_gem->relocs[bo_gem->reloc_count].delta = target_offset;
    bo_gem->relocs[bo_gem->reloc_count].target_handle =
	target_bo_gem->gem_handle;
    bo_gem->relocs[bo_gem->reloc_count].read_domains = read_domains;
    bo_gem->relocs[bo_gem->reloc_count].write_domain = write_domain;
    bo_gem->relocs[bo_gem->reloc_count].presumed_offset = target_bo->offset;

    bo_gem->reloc_target_bo[bo_gem->reloc_count] = target_bo;
    drm_intel_gem_bo_reference_locked(target_bo);

    bo_gem->reloc_count++;

    pthread_mutex_unlock(&bufmgr_gem->lock);

    return 0;
}

/**
 * Walk the tree of relocations rooted at BO and accumulate the list of
 * validations to be performed and update the relocation buffers with
 * index values into the validation list.
 */
static void
drm_intel_gem_bo_process_reloc(drm_intel_bo *bo)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    int i;

    if (bo_gem->relocs == NULL)
	return;

    for (i = 0; i < bo_gem->reloc_count; i++) {
	drm_intel_bo *target_bo = bo_gem->reloc_target_bo[i];

	/* Continue walking the tree depth-first. */
	drm_intel_gem_bo_process_reloc(target_bo);

	/* Add the target to the validate list */
	drm_intel_add_validate_buffer(target_bo);
    }
}

static void
drm_intel_update_buffer_offsets (drm_intel_bufmgr_gem *bufmgr_gem)
{
    int i;

    for (i = 0; i < bufmgr_gem->exec_count; i++) {
	drm_intel_bo *bo = bufmgr_gem->exec_bos[i];
	drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

	/* Update the buffer offset */
	if (bufmgr_gem->exec_objects[i].offset != bo->offset) {
	    DBG("BO %d (%s) migrated: 0x%08lx -> 0x%08llx\n",
		bo_gem->gem_handle, bo_gem->name, bo->offset,
		(unsigned long long)bufmgr_gem->exec_objects[i].offset);
	    bo->offset = bufmgr_gem->exec_objects[i].offset;
	}
    }
}

static int
drm_intel_gem_bo_exec(drm_intel_bo *bo, int used,
		      drm_clip_rect_t *cliprects, int num_cliprects,
		      int DR4)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    struct drm_i915_gem_execbuffer execbuf;
    int ret, i;

    pthread_mutex_lock(&bufmgr_gem->lock);
    /* Update indices and set up the validate list. */
    drm_intel_gem_bo_process_reloc(bo);

    /* Add the batch buffer to the validation list.  There are no relocations
     * pointing to it.
     */
    drm_intel_add_validate_buffer(bo);

    execbuf.buffers_ptr = (uintptr_t)bufmgr_gem->exec_objects;
    execbuf.buffer_count = bufmgr_gem->exec_count;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = used;
    execbuf.cliprects_ptr = (uintptr_t)cliprects;
    execbuf.num_cliprects = num_cliprects;
    execbuf.DR1 = 0;
    execbuf.DR4 = DR4;

    do {
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_EXECBUFFER, &execbuf);
    } while (ret != 0 && errno == EAGAIN);

    if (ret != 0 && errno == ENOMEM) {
	fprintf(stderr, "Execbuffer fails to pin. Estimate: %u. Actual: %u. Available: %u\n",
		drm_intel_gem_estimate_batch_space(bufmgr_gem->exec_bos,
						   bufmgr_gem->exec_count),
		drm_intel_gem_compute_batch_space(bufmgr_gem->exec_bos,
						  bufmgr_gem->exec_count),
		(unsigned int) bufmgr_gem->gtt_size);
    }
    drm_intel_update_buffer_offsets (bufmgr_gem);

    if (bufmgr_gem->bufmgr.debug)
	drm_intel_gem_dump_validation_list(bufmgr_gem);

    for (i = 0; i < bufmgr_gem->exec_count; i++) {
	drm_intel_bo *bo = bufmgr_gem->exec_bos[i];
	drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

	/* Need to call swrast on next bo_map */
	bo_gem->swrast = 0;

	/* Disconnect the buffer from the validate list */
	bo_gem->validate_index = -1;
	drm_intel_gem_bo_unreference_locked(bo);
	bufmgr_gem->exec_bos[i] = NULL;
    }
    bufmgr_gem->exec_count = 0;
    pthread_mutex_unlock(&bufmgr_gem->lock);

    return 0;
}

static int
drm_intel_gem_bo_pin(drm_intel_bo *bo, uint32_t alignment)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_pin pin;
    int ret;

    memset(&pin, 0, sizeof(pin));
    pin.handle = bo_gem->gem_handle;
    pin.alignment = alignment;

    do {
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_PIN, &pin);
    } while (ret == -1 && errno == EINTR);

    if (ret != 0)
	return -errno;

    bo->offset = pin.offset;
    return 0;
}

static int
drm_intel_gem_bo_unpin(drm_intel_bo *bo)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_unpin unpin;
    int ret;

    memset(&unpin, 0, sizeof(unpin));
    unpin.handle = bo_gem->gem_handle;

    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_UNPIN, &unpin);
    if (ret != 0)
	return -errno;

    return 0;
}

static int
drm_intel_gem_bo_set_tiling(drm_intel_bo *bo, uint32_t *tiling_mode,
			    uint32_t stride)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_i915_gem_set_tiling set_tiling;
    int ret;

    if (bo_gem->global_name == 0 && *tiling_mode == bo_gem->tiling_mode)
	return 0;

    /* If we're going from non-tiling to tiling, bump fence count */
    if (bo_gem->tiling_mode == I915_TILING_NONE)
	bo_gem->reloc_tree_fences++;

    memset(&set_tiling, 0, sizeof(set_tiling));
    set_tiling.handle = bo_gem->gem_handle;
    set_tiling.tiling_mode = *tiling_mode;
    set_tiling.stride = stride;

    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_SET_TILING, &set_tiling);
    if (ret != 0) {
	*tiling_mode = bo_gem->tiling_mode;
	return -errno;
    }
    bo_gem->tiling_mode = set_tiling.tiling_mode;
    bo_gem->swizzle_mode = set_tiling.swizzle_mode;

    /* If we're going from tiling to non-tiling, drop fence count */
    if (bo_gem->tiling_mode == I915_TILING_NONE)
	bo_gem->reloc_tree_fences--;

    *tiling_mode = bo_gem->tiling_mode;
    return 0;
}

static int
drm_intel_gem_bo_get_tiling(drm_intel_bo *bo, uint32_t *tiling_mode,
			    uint32_t *swizzle_mode)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

    *tiling_mode = bo_gem->tiling_mode;
    *swizzle_mode = bo_gem->swizzle_mode;
    return 0;
}

static int
drm_intel_gem_bo_flink(drm_intel_bo *bo, uint32_t *name)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo->bufmgr;
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    struct drm_gem_flink flink;
    int ret;

    if (!bo_gem->global_name) {
	memset(&flink, 0, sizeof(flink));
	flink.handle = bo_gem->gem_handle;
    
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_FLINK, &flink);
	if (ret != 0)
	    return -errno;
	bo_gem->global_name = flink.name;
	bo_gem->reusable = 0;
    }
    
    *name = bo_gem->global_name;
    return 0;
}

/**
 * Enables unlimited caching of buffer objects for reuse.
 *
 * This is potentially very memory expensive, as the cache at each bucket
 * size is only bounded by how many buffers of that size we've managed to have
 * in flight at once.
 */
void
drm_intel_bufmgr_gem_enable_reuse(drm_intel_bufmgr *bufmgr)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bufmgr;
    int i;

    for (i = 0; i < DRM_INTEL_GEM_BO_BUCKETS; i++) {
	bufmgr_gem->cache_bucket[i].max_entries = -1;
    }
}

/**
 * Return the additional aperture space required by the tree of buffer objects
 * rooted at bo.
 */
static int
drm_intel_gem_bo_get_aperture_space(drm_intel_bo *bo)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    int i;
    int total = 0;

    if (bo == NULL || bo_gem->included_in_check_aperture)
	return 0;

    total += bo->size;
    bo_gem->included_in_check_aperture = 1;

    for (i = 0; i < bo_gem->reloc_count; i++)
	total += drm_intel_gem_bo_get_aperture_space(bo_gem->reloc_target_bo[i]);

    return total;
}

/**
 * Count the number of buffers in this list that need a fence reg
 *
 * If the count is greater than the number of available regs, we'll have
 * to ask the caller to resubmit a batch with fewer tiled buffers.
 *
 * This function over-counts if the same buffer is used multiple times.
 */
static unsigned int
drm_intel_gem_total_fences(drm_intel_bo **bo_array, int count)
{
    int i;
    unsigned int total = 0;

    for (i = 0; i < count; i++) {
	drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo_array[i];

	if (bo_gem == NULL)
	    continue;

	total += bo_gem->reloc_tree_fences;
    }
    return total;
}

/**
 * Clear the flag set by drm_intel_gem_bo_get_aperture_space() so we're ready
 * for the next drm_intel_bufmgr_check_aperture_space() call.
 */
static void
drm_intel_gem_bo_clear_aperture_space_flag(drm_intel_bo *bo)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;
    int i;

    if (bo == NULL || !bo_gem->included_in_check_aperture)
	return;

    bo_gem->included_in_check_aperture = 0;

    for (i = 0; i < bo_gem->reloc_count; i++)
	drm_intel_gem_bo_clear_aperture_space_flag(bo_gem->reloc_target_bo[i]);
}

/**
 * Return a conservative estimate for the amount of aperture required
 * for a collection of buffers. This may double-count some buffers.
 */
static unsigned int
drm_intel_gem_estimate_batch_space(drm_intel_bo **bo_array, int count)
{
    int i;
    unsigned int total = 0;

    for (i = 0; i < count; i++) {
	drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo_array[i];
	if (bo_gem != NULL)
		total += bo_gem->reloc_tree_size;
    }
    return total;
}

/**
 * Return the amount of aperture needed for a collection of buffers.
 * This avoids double counting any buffers, at the cost of looking
 * at every buffer in the set.
 */
static unsigned int
drm_intel_gem_compute_batch_space(drm_intel_bo **bo_array, int count)
{
    int i;
    unsigned int total = 0;

    for (i = 0; i < count; i++) {
	total += drm_intel_gem_bo_get_aperture_space(bo_array[i]);
	/* For the first buffer object in the array, we get an accurate count
	 * back for its reloc_tree size (since nothing had been flagged as
	 * being counted yet).  We can save that value out as a more
	 * conservative reloc_tree_size that avoids double-counting target
	 * buffers.  Since the first buffer happens to usually be the batch
	 * buffer in our callers, this can pull us back from doing the tree
	 * walk on every new batch emit.
	 */
	if (i == 0) {
	    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo_array[i];
	    bo_gem->reloc_tree_size = total;
	}
    }

    for (i = 0; i < count; i++)
	drm_intel_gem_bo_clear_aperture_space_flag(bo_array[i]);
    return total;
}

/**
 * Return -1 if the batchbuffer should be flushed before attempting to
 * emit rendering referencing the buffers pointed to by bo_array.
 *
 * This is required because if we try to emit a batchbuffer with relocations
 * to a tree of buffers that won't simultaneously fit in the aperture,
 * the rendering will return an error at a point where the software is not
 * prepared to recover from it.
 *
 * However, we also want to emit the batchbuffer significantly before we reach
 * the limit, as a series of batchbuffers each of which references buffers
 * covering almost all of the aperture means that at each emit we end up
 * waiting to evict a buffer from the last rendering, and we get synchronous
 * performance.  By emitting smaller batchbuffers, we eat some CPU overhead to
 * get better parallelism.
 */
static int
drm_intel_gem_check_aperture_space(drm_intel_bo **bo_array, int count)
{
    drm_intel_bufmgr_gem *bufmgr_gem = (drm_intel_bufmgr_gem *)bo_array[0]->bufmgr;
    unsigned int total = 0;
    unsigned int threshold = bufmgr_gem->gtt_size * 3 / 4;
    int total_fences;

    /* Check for fence reg constraints if necessary */
    if (bufmgr_gem->available_fences) {
	total_fences = drm_intel_gem_total_fences(bo_array, count);
	if (total_fences > bufmgr_gem->available_fences)
	    return -1;
    }

    total = drm_intel_gem_estimate_batch_space(bo_array, count);

    if (total > threshold)
	total = drm_intel_gem_compute_batch_space(bo_array, count);

    if (total > threshold) {
	DBG("check_space: overflowed available aperture, %dkb vs %dkb\n",
	    total / 1024, (int)bufmgr_gem->gtt_size / 1024);
	return -1;
    } else {
	DBG("drm_check_space: total %dkb vs bufgr %dkb\n", total / 1024 ,
	    (int)bufmgr_gem->gtt_size / 1024);
	return 0;
    }
}

/*
 * Disable buffer reuse for objects which are shared with the kernel
 * as scanout buffers
 */
static int
drm_intel_gem_bo_disable_reuse(drm_intel_bo *bo)
{
    drm_intel_bo_gem *bo_gem = (drm_intel_bo_gem *)bo;

    bo_gem->reusable = 0;
    return 0;
}

/**
 * Initializes the GEM buffer manager, which uses the kernel to allocate, map,
 * and manage map buffer objections.
 *
 * \param fd File descriptor of the opened DRM device.
 */
drm_intel_bufmgr *
drm_intel_bufmgr_gem_init(int fd, int batch_size)
{
    drm_intel_bufmgr_gem *bufmgr_gem;
    struct drm_i915_gem_get_aperture aperture;
    drm_i915_getparam_t gp;
    int ret, i;

    bufmgr_gem = calloc(1, sizeof(*bufmgr_gem));
    bufmgr_gem->fd = fd;

    if (pthread_mutex_init(&bufmgr_gem->lock, NULL) != 0) {
      free(bufmgr_gem);
      return NULL;
   }

    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_GET_APERTURE, &aperture);

    if (ret == 0)
	bufmgr_gem->gtt_size = aperture.aper_available_size;
    else {
	fprintf(stderr, "DRM_IOCTL_I915_GEM_APERTURE failed: %s\n",
		strerror(errno));
	bufmgr_gem->gtt_size = 128 * 1024 * 1024;
	fprintf(stderr, "Assuming %dkB available aperture size.\n"
		"May lead to reduced performance or incorrect rendering.\n",
		(int)bufmgr_gem->gtt_size / 1024);
    }

    gp.param = I915_PARAM_CHIPSET_ID;
    gp.value = &bufmgr_gem->pci_device;
    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GETPARAM, &gp);
    if (ret) {
	fprintf(stderr, "get chip id failed: %d\n", ret);
	fprintf(stderr, "param: %d, val: %d\n", gp.param, *gp.value);
    }

    if (!IS_I965G(bufmgr_gem)) {
	gp.param = I915_PARAM_NUM_FENCES_AVAIL;
	gp.value = &bufmgr_gem->available_fences;
	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GETPARAM, &gp);
	if (ret) {
	    fprintf(stderr, "get fences failed: %d\n", ret);
	    fprintf(stderr, "param: %d, val: %d\n", gp.param, *gp.value);
	    bufmgr_gem->available_fences = 0;
	}
    }

    /* Let's go with one relocation per every 2 dwords (but round down a bit
     * since a power of two will mean an extra page allocation for the reloc
     * buffer).
     *
     * Every 4 was too few for the blender benchmark.
     */
    bufmgr_gem->max_relocs = batch_size / sizeof(uint32_t) / 2 - 2;

    bufmgr_gem->bufmgr.bo_alloc = drm_intel_gem_bo_alloc;
    bufmgr_gem->bufmgr.bo_alloc_for_render = drm_intel_gem_bo_alloc_for_render;
    bufmgr_gem->bufmgr.bo_reference = drm_intel_gem_bo_reference;
    bufmgr_gem->bufmgr.bo_unreference = drm_intel_gem_bo_unreference;
    bufmgr_gem->bufmgr.bo_map = drm_intel_gem_bo_map;
    bufmgr_gem->bufmgr.bo_unmap = drm_intel_gem_bo_unmap;
    bufmgr_gem->bufmgr.bo_subdata = drm_intel_gem_bo_subdata;
    bufmgr_gem->bufmgr.bo_get_subdata = drm_intel_gem_bo_get_subdata;
    bufmgr_gem->bufmgr.bo_wait_rendering = drm_intel_gem_bo_wait_rendering;
    bufmgr_gem->bufmgr.bo_emit_reloc = drm_intel_gem_bo_emit_reloc;
    bufmgr_gem->bufmgr.bo_pin = drm_intel_gem_bo_pin;
    bufmgr_gem->bufmgr.bo_unpin = drm_intel_gem_bo_unpin;
    bufmgr_gem->bufmgr.bo_get_tiling = drm_intel_gem_bo_get_tiling;
    bufmgr_gem->bufmgr.bo_set_tiling = drm_intel_gem_bo_set_tiling;
    bufmgr_gem->bufmgr.bo_flink = drm_intel_gem_bo_flink;
    bufmgr_gem->bufmgr.bo_exec = drm_intel_gem_bo_exec;
    bufmgr_gem->bufmgr.destroy = drm_intel_bufmgr_gem_destroy;
    bufmgr_gem->bufmgr.debug = 0;
    bufmgr_gem->bufmgr.check_aperture_space = drm_intel_gem_check_aperture_space;
    bufmgr_gem->bufmgr.bo_disable_reuse = drm_intel_gem_bo_disable_reuse;
    bufmgr_gem->bufmgr.get_pipe_from_crtc_id = drm_intel_gem_get_pipe_from_crtc_id;
    /* Initialize the linked lists for BO reuse cache. */
    for (i = 0; i < DRM_INTEL_GEM_BO_BUCKETS; i++)
	DRMINITLISTHEAD(&bufmgr_gem->cache_bucket[i].head);

    return &bufmgr_gem->bufmgr;
}

