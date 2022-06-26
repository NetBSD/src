/*	$NetBSD: ttm_tt.c,v 1.19 2022/06/26 17:53:06 riastradh Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 OR MIT */
/**************************************************************************
 *
 * Copyright (c) 2006-2009 VMware, Inc., Palo Alto, CA., USA
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Authors: Thomas Hellstrom <thellstrom-at-vmware-dot-com>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ttm_tt.c,v 1.19 2022/06/26 17:53:06 riastradh Exp $");

#define pr_fmt(fmt) "[TTM] " fmt

#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/shmem_fs.h>
#include <linux/file.h>
#include <drm/drm_cache.h>
#include <drm/drm_mem_util.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/ttm/ttm_page_alloc.h>
#include <drm/bus_dma_hacks.h>
#include <drm/ttm/ttm_set_memory.h>

/**
 * Allocates a ttm structure for the given BO.
 */
int ttm_tt_create(struct ttm_buffer_object *bo, bool zero_alloc)
{
	struct ttm_bo_device *bdev = bo->bdev;
	uint32_t page_flags = 0;

	dma_resv_assert_held(bo->base.resv);

	if (bdev->need_dma32)
		page_flags |= TTM_PAGE_FLAG_DMA32;

	if (bdev->no_retry)
		page_flags |= TTM_PAGE_FLAG_NO_RETRY;

	switch (bo->type) {
	case ttm_bo_type_device:
		if (zero_alloc)
			page_flags |= TTM_PAGE_FLAG_ZERO_ALLOC;
		break;
	case ttm_bo_type_kernel:
		break;
	case ttm_bo_type_sg:
		page_flags |= TTM_PAGE_FLAG_SG;
		break;
	default:
		bo->ttm = NULL;
		pr_err("Illegal buffer object type\n");
		return -EINVAL;
	}

	bo->ttm = bdev->driver->ttm_tt_create(bo, page_flags);
	if (unlikely(bo->ttm == NULL))
		return -ENOMEM;

	return 0;
}

/**
 * Allocates storage for pointers to the pages that back the ttm.
 */
static int ttm_tt_alloc_page_directory(struct ttm_tt *ttm)
{
	ttm->pages = kvmalloc_array(ttm->num_pages, sizeof(void*),
			GFP_KERNEL | __GFP_ZERO);
	if (!ttm->pages)
		return -ENOMEM;
	return 0;
}

static int ttm_sg_tt_alloc_page_directory(struct ttm_dma_tt *);

static int ttm_dma_tt_alloc_page_directory(struct ttm_dma_tt *ttm)
{
#ifdef __NetBSD__
	int r;

	/* Create array of pages at ttm->ttm.pages.  */
	r = ttm_tt_alloc_page_directory(&ttm->ttm);
	if (r)
		return r;

	/* Create bus DMA map at ttm->dma_address.  */
	r = ttm_sg_tt_alloc_page_directory(ttm);
	if (r) {
		kvfree(ttm->ttm.pages);
		ttm->ttm.pages = NULL;
		return r;
	}

	/* Success!  */
	return 0;
#else
	ttm->ttm.pages = kvmalloc_array(ttm->ttm.num_pages,
					  sizeof(*ttm->ttm.pages) +
					  sizeof(*ttm->dma_address),
					  GFP_KERNEL | __GFP_ZERO);
	if (!ttm->ttm.pages)
		return -ENOMEM;
	ttm->dma_address = (void *) (ttm->ttm.pages + ttm->ttm.num_pages);
	return 0;
#endif
}

static int ttm_sg_tt_alloc_page_directory(struct ttm_dma_tt *ttm)
{
#ifdef __NetBSD__
	ttm->dma_address = NULL;
	/* XXX errno NetBSD->Linux */
	return -bus_dmamap_create(ttm->ttm.bdev->dmat,
	    ttm->ttm.num_pages << PAGE_SHIFT, ttm->ttm.num_pages, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &ttm->dma_address);
#else
	ttm->dma_address = kvmalloc_array(ttm->ttm.num_pages,
					  sizeof(*ttm->dma_address),
					  GFP_KERNEL | __GFP_ZERO);
	if (!ttm->dma_address)
		return -ENOMEM;
	return 0;
#endif
}

static int ttm_tt_set_page_caching(struct page *p,
				   enum ttm_caching_state c_old,
				   enum ttm_caching_state c_new)
{
#ifdef __NetBSD__
	return 0;
#else
	int ret = 0;

	if (PageHighMem(p))
		return 0;

	if (c_old != tt_cached) {
		/* p isn't in the default caching state, set it to
		 * writeback first to free its current memtype. */

		ret = ttm_set_pages_wb(p, 1);
		if (ret)
			return ret;
	}

	if (c_new == tt_wc)
		ret = ttm_set_pages_wc(p, 1);
	else if (c_new == tt_uncached)
		ret = ttm_set_pages_uc(p, 1);

	return ret;
#endif
}

/*
 * Change caching policy for the linear kernel map
 * for range of pages in a ttm.
 */

static int ttm_tt_set_caching(struct ttm_tt *ttm,
			      enum ttm_caching_state c_state)
{
	int i, j;
	struct page *cur_page;
	int ret;

	if (ttm->caching_state == c_state)
		return 0;

	if (ttm->state == tt_unpopulated) {
		/* Change caching but don't populate */
		ttm->caching_state = c_state;
		return 0;
	}

	if (ttm->caching_state == tt_cached)
		drm_clflush_pages(ttm->pages, ttm->num_pages);

	for (i = 0; i < ttm->num_pages; ++i) {
		cur_page = ttm->pages[i];
		if (likely(cur_page != NULL)) {
			ret = ttm_tt_set_page_caching(cur_page,
						      ttm->caching_state,
						      c_state);
			if (unlikely(ret != 0))
				goto out_err;
		}
	}

	ttm->caching_state = c_state;

	return 0;

out_err:
	for (j = 0; j < i; ++j) {
		cur_page = ttm->pages[j];
		if (likely(cur_page != NULL)) {
			(void)ttm_tt_set_page_caching(cur_page, c_state,
						      ttm->caching_state);
		}
	}

	return ret;
}

int ttm_tt_set_placement_caching(struct ttm_tt *ttm, uint32_t placement)
{
	enum ttm_caching_state state;

	if (placement & TTM_PL_FLAG_WC)
		state = tt_wc;
	else if (placement & TTM_PL_FLAG_UNCACHED)
		state = tt_uncached;
	else
		state = tt_cached;

	return ttm_tt_set_caching(ttm, state);
}
EXPORT_SYMBOL(ttm_tt_set_placement_caching);

void ttm_tt_destroy(struct ttm_tt *ttm)
{
	if (ttm == NULL)
		return;

	ttm_tt_unbind(ttm);

	if (ttm->state == tt_unbound)
		ttm_tt_unpopulate(ttm);

#ifndef __NetBSD__
	if (!(ttm->page_flags & TTM_PAGE_FLAG_PERSISTENT_SWAP) &&
	    ttm->swap_storage)
		fput(ttm->swap_storage);

	ttm->swap_storage = NULL;
#endif
	ttm->func->destroy(ttm);
}

static void ttm_tt_init_fields(struct ttm_tt *ttm,
			       struct ttm_buffer_object *bo,
			       uint32_t page_flags)
{
	ttm->bdev = bo->bdev;
	ttm->num_pages = bo->num_pages;
	ttm->caching_state = tt_cached;
	ttm->page_flags = page_flags;
	ttm->state = tt_unpopulated;
#ifdef __NetBSD__
	WARN(bo->num_pages == 0,
	    "zero-size allocation in %s, please file a NetBSD PR",
	    __func__);	/* paranoia -- can't prove in five minutes */
	ttm->swap_storage = uao_create(PAGE_SIZE * MAX(1, bo->num_pages), 0);
	uao_set_pgfl(ttm->swap_storage, bus_dmamem_pgfl(ttm->bdev->dmat));
#else
	ttm->swap_storage = NULL;
#endif
	ttm->sg = bo->sg;
}

int ttm_tt_init(struct ttm_tt *ttm, struct ttm_buffer_object *bo,
		uint32_t page_flags)
{
	ttm_tt_init_fields(ttm, bo, page_flags);

	if (ttm_tt_alloc_page_directory(ttm)) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL(ttm_tt_init);

void ttm_tt_fini(struct ttm_tt *ttm)
{
	kvfree(ttm->pages);
	ttm->pages = NULL;
#ifdef __NetBSD__
	uao_detach(ttm->swap_storage);
	ttm->swap_storage = NULL;
#endif
}
EXPORT_SYMBOL(ttm_tt_fini);

int ttm_dma_tt_init(struct ttm_dma_tt *ttm_dma, struct ttm_buffer_object *bo,
		    uint32_t page_flags)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;

	ttm_tt_init_fields(ttm, bo, page_flags);

	INIT_LIST_HEAD(&ttm_dma->pages_list);
	if (ttm_dma_tt_alloc_page_directory(ttm_dma)) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL(ttm_dma_tt_init);

int ttm_sg_tt_init(struct ttm_dma_tt *ttm_dma, struct ttm_buffer_object *bo,
		   uint32_t page_flags)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;
	int ret;

	ttm_tt_init_fields(ttm, bo, page_flags);

	INIT_LIST_HEAD(&ttm_dma->pages_list);
	if (page_flags & TTM_PAGE_FLAG_SG)
		ret = ttm_sg_tt_alloc_page_directory(ttm_dma);
	else
		ret = ttm_dma_tt_alloc_page_directory(ttm_dma);
	if (ret) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL(ttm_sg_tt_init);

void ttm_dma_tt_fini(struct ttm_dma_tt *ttm_dma)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;

#ifdef __NetBSD__
	if (ttm_dma->dma_address) {
		bus_dmamap_destroy(ttm->bdev->dmat, ttm_dma->dma_address);
		ttm_dma->dma_address = NULL;
	}
	ttm_tt_fini(ttm);
#else
	if (ttm->pages)
		kvfree(ttm->pages);
	else
		kvfree(ttm_dma->dma_address);
	ttm->pages = NULL;
	ttm_dma->dma_address = NULL;
#endif
}
EXPORT_SYMBOL(ttm_dma_tt_fini);

void ttm_tt_unbind(struct ttm_tt *ttm)
{
	int ret __diagused;

	if (ttm->state == tt_bound) {
		ret = ttm->func->unbind(ttm);
		BUG_ON(ret);
		ttm->state = tt_unbound;
	}
}

int ttm_tt_bind(struct ttm_tt *ttm, struct ttm_mem_reg *bo_mem,
		struct ttm_operation_ctx *ctx)
{
	int ret = 0;

	if (!ttm)
		return -EINVAL;

	if (ttm->state == tt_bound)
		return 0;

	ret = ttm_tt_populate(ttm, ctx);
	if (ret)
		return ret;

	ret = ttm->func->bind(ttm, bo_mem);
	if (unlikely(ret != 0))
		return ret;

	ttm->state = tt_bound;

	return 0;
}
EXPORT_SYMBOL(ttm_tt_bind);

#ifdef __NetBSD__
/*
 * ttm_tt_wire(ttm)
 *
 *	Wire the uvm pages of ttm and fill the ttm page array.  ttm
 *	must be unpopulated, and must be marked swapped.  This does not
 *	change either state -- the caller is expected to include it
 *	among other operations for such a state transition.
 */
int
ttm_tt_wire(struct ttm_tt *ttm)
{
	struct uvm_object *uobj = ttm->swap_storage;
	struct vm_page *vm_page;
	unsigned i;
	int error;

	KASSERTMSG((ttm->state == tt_unpopulated),
	    "ttm_tt %p must be unpopulated for wiring, but state=%d",
	    ttm, (int)ttm->state);
	KASSERT(ISSET(ttm->page_flags, TTM_PAGE_FLAG_SWAPPED));
	KASSERT(uobj != NULL);

	error = uvm_obj_wirepages(uobj, 0, (ttm->num_pages << PAGE_SHIFT),
	    NULL);
	if (error)
		/* XXX errno NetBSD->Linux */
		return -error;

	rw_enter(uobj->vmobjlock, RW_READER);
	for (i = 0; i < ttm->num_pages; i++) {
		vm_page = uvm_pagelookup(uobj, ptoa(i));
		ttm->pages[i] = container_of(vm_page, struct page, p_vmp);
	}
	rw_exit(uobj->vmobjlock);

	/* Success!  */
	return 0;
}

/*
 * ttm_tt_unwire(ttm)
 *
 *	Nullify the ttm page array and unwire the uvm pages of ttm.
 *	ttm must be unbound and must be marked swapped.  This does not
 *	change either state -- the caller is expected to include it
 *	among other operations for such a state transition.
 */
void
ttm_tt_unwire(struct ttm_tt *ttm)
{
	struct uvm_object *uobj = ttm->swap_storage;
	unsigned i;

	KASSERTMSG((ttm->state == tt_unbound),
	    "ttm_tt %p must be unbound for unwiring, but state=%d",
	    ttm, (int)ttm->state);
	KASSERT(!ISSET(ttm->page_flags, TTM_PAGE_FLAG_SWAPPED));
	KASSERT(uobj != NULL);

	uvm_obj_unwirepages(uobj, 0, (ttm->num_pages << PAGE_SHIFT));
	for (i = 0; i < ttm->num_pages; i++)
		ttm->pages[i] = NULL;
}
#endif

#ifndef __NetBSD__
int ttm_tt_swapin(struct ttm_tt *ttm)
{
	struct address_space *swap_space;
	struct file *swap_storage;
	struct page *from_page;
	struct page *to_page;
	int i;
	int ret = -ENOMEM;

	swap_storage = ttm->swap_storage;
	BUG_ON(swap_storage == NULL);

	swap_space = swap_storage->f_mapping;

	for (i = 0; i < ttm->num_pages; ++i) {
		gfp_t gfp_mask = mapping_gfp_mask(swap_space);

		gfp_mask |= (ttm->page_flags & TTM_PAGE_FLAG_NO_RETRY ? __GFP_RETRY_MAYFAIL : 0);
		from_page = shmem_read_mapping_page_gfp(swap_space, i, gfp_mask);

		if (IS_ERR(from_page)) {
			ret = PTR_ERR(from_page);
			goto out_err;
		}
		to_page = ttm->pages[i];
		if (unlikely(to_page == NULL))
			goto out_err;

		copy_highpage(to_page, from_page);
		put_page(from_page);
	}

	if (!(ttm->page_flags & TTM_PAGE_FLAG_PERSISTENT_SWAP))
		fput(swap_storage);
	ttm->swap_storage = NULL;
	ttm->page_flags &= ~TTM_PAGE_FLAG_SWAPPED;

	return 0;
out_err:
	return ret;
}
#endif

int ttm_tt_swapout(struct ttm_tt *ttm, struct file *persistent_swap_storage)
{
#ifdef __NetBSD__

	KASSERTMSG((ttm->state == tt_unpopulated || ttm->state == tt_unbound),
	    "ttm_tt %p must be unpopulated or unbound for swapout,"
	    " but state=%d",
	    ttm, (int)ttm->state);
	KASSERTMSG((ttm->caching_state == tt_cached),
	    "ttm_tt %p must be cached for swapout, but caching_state=%d",
	    ttm, (int)ttm->caching_state);
	KASSERT(persistent_swap_storage == NULL);

	ttm->bdev->driver->ttm_tt_swapout(ttm);
	return 0;
#else
	struct address_space *swap_space;
	struct file *swap_storage;
	struct page *from_page;
	struct page *to_page;
	int i;
	int ret = -ENOMEM;

	BUG_ON(ttm->state != tt_unbound && ttm->state != tt_unpopulated);
	BUG_ON(ttm->caching_state != tt_cached);

	if (!persistent_swap_storage) {
		swap_storage = shmem_file_setup("ttm swap",
						ttm->num_pages << PAGE_SHIFT,
						0);
		if (IS_ERR(swap_storage)) {
			pr_err("Failed allocating swap storage\n");
			return PTR_ERR(swap_storage);
		}
	} else {
		swap_storage = persistent_swap_storage;
	}

	swap_space = swap_storage->f_mapping;

	for (i = 0; i < ttm->num_pages; ++i) {
		gfp_t gfp_mask = mapping_gfp_mask(swap_space);

		gfp_mask |= (ttm->page_flags & TTM_PAGE_FLAG_NO_RETRY ? __GFP_RETRY_MAYFAIL : 0);

		from_page = ttm->pages[i];
		if (unlikely(from_page == NULL))
			continue;

		to_page = shmem_read_mapping_page_gfp(swap_space, i, gfp_mask);
		if (IS_ERR(to_page)) {
			ret = PTR_ERR(to_page);
			goto out_err;
		}
		copy_highpage(to_page, from_page);
		set_page_dirty(to_page);
		mark_page_accessed(to_page);
		put_page(to_page);
	}

	ttm_tt_unpopulate(ttm);
	ttm->swap_storage = swap_storage;
	ttm->page_flags |= TTM_PAGE_FLAG_SWAPPED;
	if (persistent_swap_storage)
		ttm->page_flags |= TTM_PAGE_FLAG_PERSISTENT_SWAP;

	return 0;
out_err:
	if (!persistent_swap_storage)
		fput(swap_storage);

	return ret;
#endif
}

static void ttm_tt_add_mapping(struct ttm_tt *ttm)
{
#ifndef __NetBSD__
	pgoff_t i;

	if (ttm->page_flags & TTM_PAGE_FLAG_SG)
		return;

	for (i = 0; i < ttm->num_pages; ++i)
		ttm->pages[i]->mapping = ttm->bdev->dev_mapping;
#endif
}

int ttm_tt_populate(struct ttm_tt *ttm, struct ttm_operation_ctx *ctx)
{
	int ret;

	if (ttm->state != tt_unpopulated)
		return 0;

	if (ttm->bdev->driver->ttm_tt_populate)
		ret = ttm->bdev->driver->ttm_tt_populate(ttm, ctx);
	else
#ifdef __NetBSD__
		panic("no ttm population");
#else
		ret = ttm_pool_populate(ttm, ctx);
#endif
	if (!ret)
		ttm_tt_add_mapping(ttm);
	return ret;
}

static void ttm_tt_clear_mapping(struct ttm_tt *ttm)
{
#ifndef __NetBSD__
	pgoff_t i;
	struct page **page = ttm->pages;

	if (ttm->page_flags & TTM_PAGE_FLAG_SG)
		return;

	for (i = 0; i < ttm->num_pages; ++i) {
		(*page)->mapping = NULL;
		(*page++)->index = 0;
	}
#endif
}

void ttm_tt_unpopulate(struct ttm_tt *ttm)
{
	if (ttm->state == tt_unpopulated)
		return;

	ttm_tt_clear_mapping(ttm);
	if (ttm->bdev->driver->ttm_tt_unpopulate)
		ttm->bdev->driver->ttm_tt_unpopulate(ttm);
	else
#ifdef __NetBSD__
		panic("no ttm pool unpopulation");
#else
		ttm_pool_unpopulate(ttm);
#endif
}
