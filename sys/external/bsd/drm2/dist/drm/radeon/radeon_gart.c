/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */
#include <drm/drmP.h>
#include <drm/radeon_drm.h>
#include "radeon.h"

/*
 * GART
 * The GART (Graphics Aperture Remapping Table) is an aperture
 * in the GPU's address space.  System pages can be mapped into
 * the aperture and look like contiguous pages from the GPU's
 * perspective.  A page table maps the pages in the aperture
 * to the actual backing pages in system memory.
 *
 * Radeon GPUs support both an internal GART, as described above,
 * and AGP.  AGP works similarly, but the GART table is configured
 * and maintained by the northbridge rather than the driver.
 * Radeon hw has a separate AGP aperture that is programmed to
 * point to the AGP aperture provided by the northbridge and the
 * requests are passed through to the northbridge aperture.
 * Both AGP and internal GART can be used at the same time, however
 * that is not currently supported by the driver.
 *
 * This file handles the common internal GART management.
 */

/*
 * Common GART table functions.
 */
/**
 * radeon_gart_table_ram_alloc - allocate system ram for gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Allocate system memory for GART page table
 * (r1xx-r3xx, non-pcie r4xx, rs400).  These asics require the
 * gart table to be in system memory.
 * Returns 0 for success, -ENOMEM for failure.
 */
int radeon_gart_table_ram_alloc(struct radeon_device *rdev)
{
#ifdef __NetBSD__
	int rsegs;
	int error;

	error = bus_dmamem_alloc(rdev->ddev->dmat, rdev->gart.table_size,
	    PAGE_SIZE, 0, &rdev->gart.rg_table_seg, 1, &rsegs, BUS_DMA_WAITOK);
	if (error)
		goto fail0;
	KASSERT(rsegs == 1);
	error = bus_dmamap_create(rdev->ddev->dmat, rdev->gart.table_size, 1,
	    rdev->gart.table_size, 0, BUS_DMA_WAITOK,
	    &rdev->gart.rg_table_map);
	if (error)
		goto fail1;
	error = bus_dmamem_map(rdev->ddev->dmat, &rdev->gart.rg_table_seg, 1,
	    rdev->gart.table_size, &rdev->gart.ptr,
	    BUS_DMA_WAITOK|BUS_DMA_NOCACHE);
	if (error)
		goto fail2;
	error = bus_dmamap_load(rdev->ddev->dmat, rdev->gart.rg_table_map,
	    rdev->gart.ptr, rdev->gart.table_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail3;

	/* Success!  */
	rdev->gart.table_addr = rdev->gart.rg_table_map->dm_segs[0].ds_addr;
	return 0;

fail4: __unused
	bus_dmamap_unload(rdev->ddev->dmat, rdev->gart.rg_table_map);
fail3:	bus_dmamem_unmap(rdev->ddev->dmat, rdev->gart.ptr,
	    rdev->gart.table_size);
fail2:	bus_dmamap_destroy(rdev->ddev->dmat, rdev->gart.rg_table_map);
fail1:	bus_dmamem_free(rdev->ddev->dmat, &rdev->gart.rg_table_seg, 1);
fail0:	KASSERT(error);
	/* XXX errno NetBSD->Linux */
	return -error;
#else
	void *ptr;

	ptr = pci_alloc_consistent(rdev->pdev, rdev->gart.table_size,
				   &rdev->gart.table_addr);
	if (ptr == NULL) {
		return -ENOMEM;
	}
#ifdef CONFIG_X86
	if (rdev->family == CHIP_RS400 || rdev->family == CHIP_RS480 ||
	    rdev->family == CHIP_RS690 || rdev->family == CHIP_RS740) {
		set_memory_uc((unsigned long)ptr,
			      rdev->gart.table_size >> PAGE_SHIFT);
	}
#endif
	rdev->gart.ptr = ptr;
	memset((void *)rdev->gart.ptr, 0, rdev->gart.table_size);
	return 0;
#endif
}

/**
 * radeon_gart_table_ram_free - free system ram for gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Free system memory for GART page table
 * (r1xx-r3xx, non-pcie r4xx, rs400).  These asics require the
 * gart table to be in system memory.
 */
void radeon_gart_table_ram_free(struct radeon_device *rdev)
{
	if (rdev->gart.ptr == NULL) {
		return;
	}
#ifdef __NetBSD__
	bus_dmamap_unload(rdev->ddev->dmat, rdev->gart.rg_table_map);
	bus_dmamem_unmap(rdev->ddev->dmat, rdev->gart.ptr,
	    rdev->gart.table_size);
	bus_dmamap_destroy(rdev->ddev->dmat, rdev->gart.rg_table_map);
	bus_dmamem_free(rdev->ddev->dmat, &rdev->gart.rg_table_seg, 1);
#else
#ifdef CONFIG_X86
	if (rdev->family == CHIP_RS400 || rdev->family == CHIP_RS480 ||
	    rdev->family == CHIP_RS690 || rdev->family == CHIP_RS740) {
		set_memory_wb((unsigned long)rdev->gart.ptr,
			      rdev->gart.table_size >> PAGE_SHIFT);
	}
#endif
	pci_free_consistent(rdev->pdev, rdev->gart.table_size,
			    (void *)rdev->gart.ptr,
			    rdev->gart.table_addr);
	rdev->gart.ptr = NULL;
	rdev->gart.table_addr = 0;
#endif
}

/**
 * radeon_gart_table_vram_alloc - allocate vram for gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Allocate video memory for GART page table
 * (pcie r4xx, r5xx+).  These asics require the
 * gart table to be in video memory.
 * Returns 0 for success, error for failure.
 */
int radeon_gart_table_vram_alloc(struct radeon_device *rdev)
{
	int r;

	if (rdev->gart.robj == NULL) {
		r = radeon_bo_create(rdev, rdev->gart.table_size,
				     PAGE_SIZE, true, RADEON_GEM_DOMAIN_VRAM,
				     NULL, &rdev->gart.robj);
		if (r) {
			return r;
		}
	}
	return 0;
}

/**
 * radeon_gart_table_vram_pin - pin gart page table in vram
 *
 * @rdev: radeon_device pointer
 *
 * Pin the GART page table in vram so it will not be moved
 * by the memory manager (pcie r4xx, r5xx+).  These asics require the
 * gart table to be in video memory.
 * Returns 0 for success, error for failure.
 */
int radeon_gart_table_vram_pin(struct radeon_device *rdev)
{
	uint64_t gpu_addr;
	int r;

	r = radeon_bo_reserve(rdev->gart.robj, false);
	if (unlikely(r != 0))
		return r;
	r = radeon_bo_pin(rdev->gart.robj,
				RADEON_GEM_DOMAIN_VRAM, &gpu_addr);
	if (r) {
		radeon_bo_unreserve(rdev->gart.robj);
		return r;
	}
	r = radeon_bo_kmap(rdev->gart.robj, &rdev->gart.ptr);
	if (r)
		radeon_bo_unpin(rdev->gart.robj);
	radeon_bo_unreserve(rdev->gart.robj);
	rdev->gart.table_addr = gpu_addr;
	return r;
}

/**
 * radeon_gart_table_vram_unpin - unpin gart page table in vram
 *
 * @rdev: radeon_device pointer
 *
 * Unpin the GART page table in vram (pcie r4xx, r5xx+).
 * These asics require the gart table to be in video memory.
 */
void radeon_gart_table_vram_unpin(struct radeon_device *rdev)
{
	int r;

	if (rdev->gart.robj == NULL) {
		return;
	}
	r = radeon_bo_reserve(rdev->gart.robj, false);
	if (likely(r == 0)) {
		radeon_bo_kunmap(rdev->gart.robj);
		radeon_bo_unpin(rdev->gart.robj);
		radeon_bo_unreserve(rdev->gart.robj);
		rdev->gart.ptr = NULL;
	}
}

/**
 * radeon_gart_table_vram_free - free gart page table vram
 *
 * @rdev: radeon_device pointer
 *
 * Free the video memory used for the GART page table
 * (pcie r4xx, r5xx+).  These asics require the gart table to
 * be in video memory.
 */
void radeon_gart_table_vram_free(struct radeon_device *rdev)
{
	if (rdev->gart.robj == NULL) {
		return;
	}
	radeon_bo_unref(&rdev->gart.robj);
}

#ifdef __NetBSD__
static void
radeon_gart_pre_update(struct radeon_device *rdev, unsigned gpu_pgstart,
    unsigned gpu_npages)
{

	if (rdev->gart.rg_table_map != NULL) {
		const unsigned entsize =
		    rdev->gart.table_size / rdev->gart.num_gpu_pages;

		bus_dmamap_sync(rdev->ddev->dmat, rdev->gart.rg_table_map,
		    gpu_pgstart*entsize, gpu_npages*entsize,
		    BUS_DMASYNC_PREWRITE);
	}
}

static void
radeon_gart_post_update(struct radeon_device *rdev, unsigned gpu_pgstart,
    unsigned gpu_npages)
{

	membar_sync();		/* XXX overkill */
	if (rdev->gart.rg_table_map != NULL) {
		const unsigned entsize =
		    rdev->gart.table_size / rdev->gart.num_gpu_pages;

		bus_dmamap_sync(rdev->ddev->dmat, rdev->gart.rg_table_map,
		    gpu_pgstart*entsize, gpu_npages*entsize,
		    BUS_DMASYNC_POSTWRITE);
	}
	radeon_gart_tlb_flush(rdev);
}
#endif

/*
 * Common gart functions.
 */
#ifdef __NetBSD__
void
radeon_gart_unbind(struct radeon_device *rdev, unsigned gpu_start,
    unsigned npages)
{
	const unsigned gpu_per_cpu = (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);
	const unsigned gpu_npages = (npages * gpu_per_cpu);
	const unsigned gpu_pgstart = (gpu_start / RADEON_GPU_PAGE_SIZE);
	const unsigned pgstart = (gpu_pgstart / gpu_per_cpu);
	unsigned pgno, gpu_pgno;

	KASSERT(pgstart == (gpu_start / PAGE_SIZE));
	KASSERT(npages <= rdev->gart.num_cpu_pages);
	KASSERT(gpu_npages <= rdev->gart.num_cpu_pages);

	if (!rdev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return;
	}

	radeon_gart_pre_update(rdev, gpu_pgstart, gpu_npages);
	for (pgno = 0; pgno < npages; pgno++) {
		if (rdev->gart.pages[pgstart + pgno] == NULL)
			continue;
		rdev->gart.pages[pgstart + pgno] = NULL;
		rdev->gart.pages_addr[pgstart + pgno] = rdev->dummy_page.addr;
		if (rdev->gart.ptr == NULL)
			continue;
		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++)
			radeon_gart_set_page(rdev,
			    (gpu_pgstart + gpu_per_cpu*pgno + gpu_pgno),
			    (rdev->dummy_page.addr +
				gpu_pgno*RADEON_GPU_PAGE_SIZE));
	}
	radeon_gart_post_update(rdev, gpu_pgstart, gpu_npages);
}
#else
/**
 * radeon_gart_unbind - unbind pages from the gart page table
 *
 * @rdev: radeon_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to unbind
 *
 * Unbinds the requested pages from the gart page table and
 * replaces them with the dummy page (all asics).
 */
void radeon_gart_unbind(struct radeon_device *rdev, unsigned offset,
			int pages)
{
	unsigned t;
	unsigned p;
	int i, j;
	u64 page_base;

	if (!rdev->gart.ready) {
		WARN(1, "trying to unbind memory from uninitialized GART !\n");
		return;
	}
	t = offset / RADEON_GPU_PAGE_SIZE;
	p = t / (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);
	for (i = 0; i < pages; i++, p++) {
		if (rdev->gart.pages[p]) {
			rdev->gart.pages[p] = NULL;
			rdev->gart.pages_addr[p] = rdev->dummy_page.addr;
			page_base = rdev->gart.pages_addr[p];
			for (j = 0; j < (PAGE_SIZE / RADEON_GPU_PAGE_SIZE); j++, t++) {
				if (rdev->gart.ptr) {
					radeon_gart_set_page(rdev, t, page_base);
				}
				page_base += RADEON_GPU_PAGE_SIZE;
			}
		}
	}
	mb();
	radeon_gart_tlb_flush(rdev);
}
#endif

#ifdef __NetBSD__
int
radeon_gart_bind(struct radeon_device *rdev, unsigned gpu_start,
    unsigned npages, struct page **pages, bus_dmamap_t dmamap)
{
	const unsigned gpu_per_cpu = (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);
	const unsigned gpu_npages = (npages * gpu_per_cpu);
	const unsigned gpu_pgstart = (gpu_start / RADEON_GPU_PAGE_SIZE);
	const unsigned pgstart = (gpu_pgstart / gpu_per_cpu);
	unsigned pgno, gpu_pgno;

	KASSERT(pgstart == (gpu_start / PAGE_SIZE));
	KASSERT(npages == dmamap->dm_nsegs);
	KASSERT(npages <= rdev->gart.num_cpu_pages);
	KASSERT(gpu_npages <= rdev->gart.num_cpu_pages);

	if (!rdev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

	radeon_gart_pre_update(rdev, gpu_pgstart, gpu_npages);
	for (pgno = 0; pgno < npages; pgno++) {
		const bus_addr_t addr = dmamap->dm_segs[pgno].ds_addr;

		KASSERT(dmamap->dm_segs[pgno].ds_len == PAGE_SIZE);
		rdev->gart.pages[pgstart + pgno] = pages[pgno];
		rdev->gart.pages_addr[pgstart + pgno] = addr;
		if (rdev->gart.ptr == NULL)
			continue;
		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++)
			radeon_gart_set_page(rdev,
			    (gpu_pgstart + gpu_per_cpu*pgno + gpu_pgno),
			    (addr + gpu_pgno*RADEON_GPU_PAGE_SIZE));
	}
	radeon_gart_post_update(rdev, gpu_pgstart, gpu_npages);

	return 0;
}
#else
/**
 * radeon_gart_bind - bind pages into the gart page table
 *
 * @rdev: radeon_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to bind
 * @pagelist: pages to bind
 * @dma_addr: DMA addresses of pages
 *
 * Binds the requested pages to the gart page table
 * (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
int radeon_gart_bind(struct radeon_device *rdev, unsigned offset,
		     int pages, struct page **pagelist, dma_addr_t *dma_addr)
{
	unsigned t;
	unsigned p;
	uint64_t page_base;
	int i, j;

	if (!rdev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}
	t = offset / RADEON_GPU_PAGE_SIZE;
	p = t / (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);

	for (i = 0; i < pages; i++, p++) {
		rdev->gart.pages_addr[p] = dma_addr[i];
		rdev->gart.pages[p] = pagelist[i];
		if (rdev->gart.ptr) {
			page_base = rdev->gart.pages_addr[p];
			for (j = 0; j < (PAGE_SIZE / RADEON_GPU_PAGE_SIZE); j++, t++) {
				radeon_gart_set_page(rdev, t, page_base);
				page_base += RADEON_GPU_PAGE_SIZE;
			}
		}
	}
	mb();
	radeon_gart_tlb_flush(rdev);
	return 0;
}
#endif

/**
 * radeon_gart_restore - bind all pages in the gart page table
 *
 * @rdev: radeon_device pointer
 *
 * Binds all pages in the gart page table (all asics).
 * Used to rebuild the gart table on device startup or resume.
 */
void radeon_gart_restore(struct radeon_device *rdev)
{
#ifdef __NetBSD__
	const unsigned gpu_per_cpu = (PAGE_SIZE / RADEON_GPU_PAGE_SIZE);
	unsigned pgno, gpu_pgno;

	if (rdev->gart.ptr == NULL)
		return;

	radeon_gart_pre_update(rdev, 0, rdev->gart.num_gpu_pages);
	for (pgno = 0; pgno < rdev->gart.num_cpu_pages; pgno++) {
		const bus_addr_t addr = rdev->gart.pages_addr[pgno];
		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++)
			radeon_gart_set_page(rdev,
			    (gpu_per_cpu*pgno + gpu_pgno),
			    (addr + gpu_pgno*RADEON_GPU_PAGE_SIZE));
	}
	radeon_gart_post_update(rdev, 0, rdev->gart.num_gpu_pages);
#else
	int i, j, t;
	u64 page_base;

	if (!rdev->gart.ptr) {
		return;
	}
	for (i = 0, t = 0; i < rdev->gart.num_cpu_pages; i++) {
		page_base = rdev->gart.pages_addr[i];
		for (j = 0; j < (PAGE_SIZE / RADEON_GPU_PAGE_SIZE); j++, t++) {
			radeon_gart_set_page(rdev, t, page_base);
			page_base += RADEON_GPU_PAGE_SIZE;
		}
	}
	mb();
	radeon_gart_tlb_flush(rdev);
#endif
}

/**
 * radeon_gart_init - init the driver info for managing the gart
 *
 * @rdev: radeon_device pointer
 *
 * Allocate the dummy page and init the gart driver info (all asics).
 * Returns 0 for success, error for failure.
 */
int radeon_gart_init(struct radeon_device *rdev)
{
	int r, i;

	if (rdev->gart.pages) {
		return 0;
	}
	/* We need PAGE_SIZE >= RADEON_GPU_PAGE_SIZE */
	if (PAGE_SIZE < RADEON_GPU_PAGE_SIZE) {
		DRM_ERROR("Page size is smaller than GPU page size!\n");
		return -EINVAL;
	}
	r = radeon_dummy_page_init(rdev);
	if (r)
		return r;
	/* Compute table size */
	rdev->gart.num_cpu_pages = rdev->mc.gtt_size / PAGE_SIZE;
	rdev->gart.num_gpu_pages = rdev->mc.gtt_size / RADEON_GPU_PAGE_SIZE;
	DRM_INFO("GART: num cpu pages %u, num gpu pages %u\n",
		 rdev->gart.num_cpu_pages, rdev->gart.num_gpu_pages);
	/* Allocate pages table */
	rdev->gart.pages = vzalloc(sizeof(void *) * rdev->gart.num_cpu_pages);
	if (rdev->gart.pages == NULL) {
		radeon_gart_fini(rdev);
		return -ENOMEM;
	}
	rdev->gart.pages_addr = vzalloc(sizeof(dma_addr_t) *
					rdev->gart.num_cpu_pages);
	if (rdev->gart.pages_addr == NULL) {
		radeon_gart_fini(rdev);
		return -ENOMEM;
	}
	/* set GART entry to point to the dummy page by default */
	for (i = 0; i < rdev->gart.num_cpu_pages; i++) {
		rdev->gart.pages_addr[i] = rdev->dummy_page.addr;
	}
	return 0;
}

/**
 * radeon_gart_fini - tear down the driver info for managing the gart
 *
 * @rdev: radeon_device pointer
 *
 * Tear down the gart driver info and free the dummy page (all asics).
 */
void radeon_gart_fini(struct radeon_device *rdev)
{
	if (rdev->gart.pages && rdev->gart.pages_addr && rdev->gart.ready) {
		/* unbind pages */
		radeon_gart_unbind(rdev, 0, rdev->gart.num_cpu_pages);
	}
	rdev->gart.ready = false;
	vfree(rdev->gart.pages);
	vfree(rdev->gart.pages_addr);
	rdev->gart.pages = NULL;
	rdev->gart.pages_addr = NULL;

	radeon_dummy_page_fini(rdev);
}
