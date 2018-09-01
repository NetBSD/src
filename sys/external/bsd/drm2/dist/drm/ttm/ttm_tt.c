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

#define pr_fmt(fmt) "[TTM] " fmt

#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/shmem_fs.h>
#include <linux/file.h>
#include <linux/swap.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <drm/drm_cache.h>
#include <drm/drm_mem_util.h>
#include <drm/ttm/ttm_module.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/ttm/ttm_placement.h>
#include <drm/ttm/ttm_page_alloc.h>
#include <drm/bus_dma_hacks.h>

/**
 * Allocates storage for pointers to the pages that back the ttm.
 */
static void ttm_tt_alloc_page_directory(struct ttm_tt *ttm)
{
	ttm->pages = drm_calloc_large(ttm->num_pages, sizeof(void*));
}

static void ttm_dma_tt_alloc_page_directory(struct ttm_dma_tt *ttm)
{
	ttm->ttm.pages = drm_calloc_large(ttm->ttm.num_pages, sizeof(void*));
#ifndef __NetBSD__
	ttm->dma_address = drm_calloc_large(ttm->ttm.num_pages,
					    sizeof(*ttm->dma_address));
#endif
}

#ifdef CONFIG_X86
static inline int ttm_tt_set_page_caching(struct page *p,
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

		ret = set_pages_wb(p, 1);
		if (ret)
			return ret;
	}

	if (c_new == tt_wc)
		ret = set_memory_wc((unsigned long) page_address(p), 1);
	else if (c_new == tt_uncached)
		ret = set_pages_uc(p, 1);

	return ret;
#endif
}
#else /* CONFIG_X86 */
static inline int ttm_tt_set_page_caching(struct page *p,
					  enum ttm_caching_state c_old,
					  enum ttm_caching_state c_new)
{
	return 0;
}
#endif /* CONFIG_X86 */

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
	if (unlikely(ttm == NULL))
		return;

	if (ttm->state == tt_bound) {
		ttm_tt_unbind(ttm);
	}

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

int ttm_tt_init(struct ttm_tt *ttm, struct ttm_bo_device *bdev,
		unsigned long size, uint32_t page_flags,
		struct page *dummy_read_page)
{
	ttm->bdev = bdev;
	ttm->glob = bdev->glob;
	ttm->num_pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	ttm->caching_state = tt_cached;
	ttm->page_flags = page_flags;
	ttm->dummy_read_page = dummy_read_page;
	ttm->state = tt_unpopulated;
#ifdef __NetBSD__
	WARN(size == 0, "zero-size allocation in %s, please file a NetBSD PR",
	    __func__);	/* paranoia -- can't prove in five minutes */
	size = MAX(size, 1);
	ttm->swap_storage = uao_create(roundup2(size, PAGE_SIZE), 0);
	uao_set_pgfl(ttm->swap_storage, bus_dmamem_pgfl(bdev->dmat));
#else
	ttm->swap_storage = NULL;
#endif
	TAILQ_INIT(&ttm->pglist);

	ttm_tt_alloc_page_directory(ttm);
	if (!ttm->pages) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL(ttm_tt_init);

void ttm_tt_fini(struct ttm_tt *ttm)
{
#ifdef __NetBSD__
	uao_detach(ttm->swap_storage);
	ttm->swap_storage = NULL;
#endif
	drm_free_large(ttm->pages);
	ttm->pages = NULL;
}
EXPORT_SYMBOL(ttm_tt_fini);

int ttm_dma_tt_init(struct ttm_dma_tt *ttm_dma, struct ttm_bo_device *bdev,
		unsigned long size, uint32_t page_flags,
		struct page *dummy_read_page)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;

	ttm->bdev = bdev;
	ttm->glob = bdev->glob;
	ttm->num_pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	ttm->caching_state = tt_cached;
	ttm->page_flags = page_flags;
	ttm->dummy_read_page = dummy_read_page;
	ttm->state = tt_unpopulated;
#ifdef __NetBSD__
	WARN(size == 0, "zero-size allocation in %s, please file a NetBSD PR",
	    __func__);	/* paranoia -- can't prove in five minutes */
	size = MAX(size, 1);
	ttm->swap_storage = uao_create(roundup2(size, PAGE_SIZE), 0);
	uao_set_pgfl(ttm->swap_storage, bus_dmamem_pgfl(bdev->dmat));
#else
	ttm->swap_storage = NULL;
#endif
	TAILQ_INIT(&ttm->pglist);

	INIT_LIST_HEAD(&ttm_dma->pages_list);
	ttm_dma_tt_alloc_page_directory(ttm_dma);
#ifdef __NetBSD__
    {
	int error;

	if (ttm->num_pages > (SIZE_MAX /
		MIN(sizeof(ttm_dma->dma_segs[0]), PAGE_SIZE))) {
		error = ENOMEM;
		goto fail0;
	}
	ttm_dma->dma_segs = kmem_alloc((ttm->num_pages *
		sizeof(ttm_dma->dma_segs[0])), KM_SLEEP);
	error = bus_dmamap_create(ttm->bdev->dmat,
	    (ttm->num_pages * PAGE_SIZE), ttm->num_pages, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &ttm_dma->dma_address);
	if (error)
		goto fail1;

	return 0;

fail2: __unused
	bus_dmamap_destroy(ttm->bdev->dmat, ttm_dma->dma_address);
fail1:	kmem_free(ttm_dma->dma_segs, (ttm->num_pages *
		sizeof(ttm_dma->dma_segs[0])));
fail0:	KASSERT(error);
	drm_free_large(ttm->pages);
	uao_detach(ttm->swap_storage);
	/* XXX errno NetBSD->Linux */
	return -error;
    }
#else
	if (!ttm->pages || !ttm_dma->dma_address) {
		ttm_tt_destroy(ttm);
		pr_err("Failed allocating page table\n");
		return -ENOMEM;
	}
	return 0;
#endif
}
EXPORT_SYMBOL(ttm_dma_tt_init);

void ttm_dma_tt_fini(struct ttm_dma_tt *ttm_dma)
{
	struct ttm_tt *ttm = &ttm_dma->ttm;

#ifdef __NetBSD__
	uao_detach(ttm->swap_storage);
	ttm->swap_storage = NULL;
#endif
	drm_free_large(ttm->pages);
	ttm->pages = NULL;
#ifdef __NetBSD__
	bus_dmamap_destroy(ttm->bdev->dmat, ttm_dma->dma_address);
	kmem_free(ttm_dma->dma_segs, (ttm->num_pages *
		sizeof(ttm_dma->dma_segs[0])));
#else
	drm_free_large(ttm_dma->dma_address);
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

int ttm_tt_bind(struct ttm_tt *ttm, struct ttm_mem_reg *bo_mem)
{
	int ret = 0;

	if (!ttm)
		return -EINVAL;

	if (ttm->state == tt_bound)
		return 0;

	ret = ttm->bdev->driver->ttm_tt_populate(ttm);
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
	struct vm_page *page;
	unsigned i;
	int error;

	KASSERTMSG((ttm->state == tt_unpopulated),
	    "ttm_tt %p must be unpopulated for wiring, but state=%d",
	    ttm, (int)ttm->state);
	KASSERT(ISSET(ttm->page_flags, TTM_PAGE_FLAG_SWAPPED));
	KASSERT(uobj != NULL);

	error = uvm_obj_wirepages(uobj, 0, (ttm->num_pages << PAGE_SHIFT),
	    &ttm->pglist);
	if (error)
		/* XXX errno NetBSD->Linux */
		return -error;

	i = 0;
	TAILQ_FOREACH(page, &ttm->pglist, pageq.queue) {
		KASSERT(i < ttm->num_pages);
		KASSERT(ttm->pages[i] == NULL);
		ttm->pages[i] = container_of(page, struct page, p_vmp);
		i++;
	}
	KASSERT(i == ttm->num_pages);

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

	swap_space = file_inode(swap_storage)->i_mapping;

	for (i = 0; i < ttm->num_pages; ++i) {
		from_page = shmem_read_mapping_page(swap_space, i);
		if (IS_ERR(from_page)) {
			ret = PTR_ERR(from_page);
			goto out_err;
		}
		to_page = ttm->pages[i];
		if (unlikely(to_page == NULL))
			goto out_err;

		copy_highpage(to_page, from_page);
		page_cache_release(from_page);
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
		if (unlikely(IS_ERR(swap_storage))) {
			pr_err("Failed allocating swap storage\n");
			return PTR_ERR(swap_storage);
		}
	} else
		swap_storage = persistent_swap_storage;

	swap_space = file_inode(swap_storage)->i_mapping;

	for (i = 0; i < ttm->num_pages; ++i) {
		from_page = ttm->pages[i];
		if (unlikely(from_page == NULL))
			continue;
		to_page = shmem_read_mapping_page(swap_space, i);
		if (unlikely(IS_ERR(to_page))) {
			ret = PTR_ERR(to_page);
			goto out_err;
		}
		copy_highpage(to_page, from_page);
		set_page_dirty(to_page);
		mark_page_accessed(to_page);
		page_cache_release(to_page);
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
	ttm->bdev->driver->ttm_tt_unpopulate(ttm);
}
