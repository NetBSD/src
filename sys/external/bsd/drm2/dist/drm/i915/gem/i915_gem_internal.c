/*	$NetBSD: i915_gem_internal.c,v 1.4 2021/12/19 11:33:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2014-2016 Intel Corporation
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_gem_internal.c,v 1.4 2021/12/19 11:33:30 riastradh Exp $");

#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/swiotlb.h>

#include <drm/i915_drm.h>

#include "i915_drv.h"
#include "i915_gem.h"
#include "i915_gem_object.h"
#include "i915_scatterlist.h"
#include "i915_utils.h"

#ifndef __NetBSD__
#define QUIET (__GFP_NORETRY | __GFP_NOWARN)
#define MAYFAIL (__GFP_RETRY_MAYFAIL | __GFP_NOWARN)

static void internal_free_pages(struct sg_table *st)
{
	struct scatterlist *sg;

	for (sg = st->sgl; sg; sg = __sg_next(sg)) {
		if (sg_page(sg))
			__free_pages(sg_page(sg), get_order(sg->length));
	}

	sg_free_table(st);
	kfree(st);
}
#endif

static int i915_gem_object_get_pages_internal(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *i915 = to_i915(obj->base.dev);
#ifdef __NetBSD__
	bus_dma_tag_t dmat = i915->drm.dmat;
	struct sg_table *sgt = NULL;
	size_t nsegs;
	bool alloced = false, prepared = false;
	int ret;

	obj->mm.u.internal.rsegs = obj->mm.u.internal.nsegs = 0;

	KASSERT(obj->mm.u.internal.segs == NULL);
	nsegs = obj->base.size >> PAGE_SHIFT;
	if (nsegs > INT_MAX ||
	    nsegs > SIZE_MAX/sizeof(obj->mm.u.internal.segs[0])) {
		ret = -ENOMEM;
		goto out;
	}
	obj->mm.u.internal.segs = kmem_alloc(
	    nsegs * sizeof(obj->mm.u.internal.segs[0]),
	    KM_NOSLEEP);
	if (obj->mm.u.internal.segs == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	obj->mm.u.internal.nsegs = nsegs;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dmat, obj->base.size, PAGE_SIZE, 0,
	    obj->mm.u.internal.segs, nsegs, &obj->mm.u.internal.rsegs,
	    BUS_DMA_NOWAIT);
	if (ret)
		goto out;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (sgt == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	if (sg_alloc_table_from_bus_dmamem(sgt, dmat, obj->mm.u.internal.segs,
		obj->mm.u.internal.rsegs, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto out;
	}
	alloced = true;

	ret = i915_gem_gtt_prepare_pages(obj, sgt);
	if (ret)
		goto out;
	prepared = true;

	obj->mm.madv = I915_MADV_DONTNEED;
	__i915_gem_object_set_pages(obj, sgt, i915_sg_page_sizes(sgt->sgl));

	return 0;

out:	if (ret) {
		if (prepared)
			i915_gem_gtt_finish_pages(obj, sgt);
		if (alloced)
			sg_free_table(sgt);
		if (sgt) {
			kfree(sgt);
			sgt = NULL;
		}
		if (obj->mm.u.internal.rsegs) {
			bus_dmamem_free(dmat, obj->mm.u.internal.segs,
			    obj->mm.u.internal.rsegs);
			obj->mm.u.internal.rsegs = 0;
		}
		if (obj->mm.u.internal.nsegs) {
			kmem_free(obj->mm.u.internal.segs,
			    (obj->mm.u.internal.nsegs *
				sizeof(obj->mm.u.internal.segs[0])));
			obj->mm.u.internal.nsegs = 0;
			obj->mm.u.internal.segs = NULL;
		}
	}
	return ret;
#else
	struct sg_table *st;
	struct scatterlist *sg;
	unsigned int sg_page_sizes;
	unsigned int npages;
	int max_order;
	gfp_t gfp;

	max_order = MAX_ORDER;
#ifdef CONFIG_SWIOTLB
	if (swiotlb_nr_tbl()) {
		unsigned int max_segment;

		max_segment = swiotlb_max_segment();
		if (max_segment) {
			max_segment = max_t(unsigned int, max_segment,
					    PAGE_SIZE) >> PAGE_SHIFT;
			max_order = min(max_order, ilog2(max_segment));
		}
	}
#endif

	gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_RECLAIMABLE;
	if (IS_I965GM(i915) || IS_I965G(i915)) {
		/* 965gm cannot relocate objects above 4GiB. */
		gfp &= ~__GFP_HIGHMEM;
		gfp |= __GFP_DMA32;
	}

create_st:
	st = kmalloc(sizeof(*st), GFP_KERNEL);
	if (!st)
		return -ENOMEM;

	npages = obj->base.size / PAGE_SIZE;
	if (sg_alloc_table(st, npages, GFP_KERNEL)) {
		kfree(st);
		return -ENOMEM;
	}

	sg = st->sgl;
	st->nents = 0;
	sg_page_sizes = 0;

	do {
		int order = min(fls(npages) - 1, max_order);
		struct page *page;

		do {
			page = alloc_pages(gfp | (order ? QUIET : MAYFAIL),
					   order);
			if (page)
				break;
			if (!order--)
				goto err;

			/* Limit subsequent allocations as well */
			max_order = order;
		} while (1);

		sg_set_page(sg, page, PAGE_SIZE << order, 0);
		sg_page_sizes |= PAGE_SIZE << order;
		st->nents++;

		npages -= 1 << order;
		if (!npages) {
			sg_mark_end(sg);
			break;
		}

		sg = __sg_next(sg);
	} while (1);

	if (i915_gem_gtt_prepare_pages(obj, st)) {
		/* Failed to dma-map try again with single page sg segments */
		if (get_order(st->sgl->length)) {
			internal_free_pages(st);
			max_order = 0;
			goto create_st;
		}
		goto err;
	}

	__i915_gem_object_set_pages(obj, st, sg_page_sizes);

	return 0;

err:
	sg_set_page(sg, NULL, 0, 0);
	sg_mark_end(sg);
	internal_free_pages(st);

	return -ENOMEM;
#endif
}

static void i915_gem_object_put_pages_internal(struct drm_i915_gem_object *obj,
					       struct sg_table *pages)
{
	i915_gem_gtt_finish_pages(obj, pages);
#ifdef __NetBSD__
	sg_free_table(pages);
	kfree(pages);
	bus_dmamem_free(obj->base.dev->dmat, obj->mm.u.internal.segs,
	    obj->mm.u.internal.rsegs);
	obj->mm.u.internal.rsegs = 0;
	kmem_free(obj->mm.u.internal.segs,
	    obj->mm.u.internal.nsegs * sizeof(obj->mm.u.internal.segs[0]));
	obj->mm.u.internal.nsegs = 0;
	obj->mm.u.internal.segs = NULL;
#else
	internal_free_pages(pages);
#endif

	obj->mm.dirty = false;
}

static const struct drm_i915_gem_object_ops i915_gem_object_internal_ops = {
	.flags = I915_GEM_OBJECT_HAS_STRUCT_PAGE |
		 I915_GEM_OBJECT_IS_SHRINKABLE,
	.get_pages = i915_gem_object_get_pages_internal,
	.put_pages = i915_gem_object_put_pages_internal,
};

/**
 * i915_gem_object_create_internal: create an object with volatile pages
 * @i915: the i915 device
 * @size: the size in bytes of backing storage to allocate for the object
 *
 * Creates a new object that wraps some internal memory for private use.
 * This object is not backed by swappable storage, and as such its contents
 * are volatile and only valid whilst pinned. If the object is reaped by the
 * shrinker, its pages and data will be discarded. Equally, it is not a full
 * GEM object and so not valid for access from userspace. This makes it useful
 * for hardware interfaces like ringbuffers (which are pinned from the time
 * the request is written to the time the hardware stops accessing it), but
 * not for contexts (which need to be preserved when not active for later
 * reuse). Note that it is not cleared upon allocation.
 */
struct drm_i915_gem_object *
i915_gem_object_create_internal(struct drm_i915_private *i915,
				phys_addr_t size)
{
	static struct lock_class_key lock_class;
	struct drm_i915_gem_object *obj;
	unsigned int cache_level;

	GEM_BUG_ON(!size);
	GEM_BUG_ON(!IS_ALIGNED(size, PAGE_SIZE));

	if (overflows_type(size, obj->base.size))
		return ERR_PTR(-E2BIG);

	obj = i915_gem_object_alloc();
	if (!obj)
		return ERR_PTR(-ENOMEM);

	drm_gem_private_object_init(&i915->drm, &obj->base, size);
	i915_gem_object_init(obj, &i915_gem_object_internal_ops, &lock_class);

	/*
	 * Mark the object as volatile, such that the pages are marked as
	 * dontneed whilst they are still pinned. As soon as they are unpinned
	 * they are allowed to be reaped by the shrinker, and the caller is
	 * expected to repopulate - the contents of this object are only valid
	 * whilst active and pinned.
	 */
	i915_gem_object_set_volatile(obj);

	obj->read_domains = I915_GEM_DOMAIN_CPU;
	obj->write_domain = I915_GEM_DOMAIN_CPU;

	cache_level = HAS_LLC(i915) ? I915_CACHE_LLC : I915_CACHE_NONE;
	i915_gem_object_set_cache_coherency(obj, cache_level);

	return obj;
}
