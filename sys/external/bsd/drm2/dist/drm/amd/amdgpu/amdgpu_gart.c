/*	$NetBSD: amdgpu_gart.c,v 1.3 2018/08/27 14:04:50 riastradh Exp $	*/

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdgpu_gart.c,v 1.3 2018/08/27 14:04:50 riastradh Exp $");

#include <drm/drmP.h>
#include <drm/amdgpu_drm.h>
#include "amdgpu.h"

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
 * amdgpu_gart_table_ram_alloc - allocate system ram for gart page table
 *
 * @adev: amdgpu_device pointer
 *
 * Allocate system memory for GART page table
 * (r1xx-r3xx, non-pcie r4xx, rs400).  These asics require the
 * gart table to be in system memory.
 * Returns 0 for success, -ENOMEM for failure.
 */
int amdgpu_gart_table_ram_alloc(struct amdgpu_device *adev)
{
#ifdef __NetBSD__
	int rsegs;
	int error;

	error = bus_dmamem_alloc(adev->ddev->dmat, adev->gart.table_size,
	    PAGE_SIZE, 0, &adev->gart.ag_table_seg, 1, &rsegs, BUS_DMA_WAITOK);
	if (error)
		goto fail0;
	KASSERT(rsegs == 1);
	error = bus_dmamap_create(adev->ddev->dmat, adev->gart.table_size, 1,
	    adev->gart.table_size, 0, BUS_DMA_WAITOK,
	    &adev->gart.ag_table_map);
	if (error)
		goto fail1;
	error = bus_dmamem_map(adev->ddev->dmat, &adev->gart.ag_table_seg, 1,
	    adev->gart.table_size, &adev->gart.ptr,
	    BUS_DMA_WAITOK|BUS_DMA_NOCACHE);
	if (error)
		goto fail2;
	error = bus_dmamap_load(adev->ddev->dmat, adev->gart.ag_table_map,
	    adev->gart.ptr, adev->gart.table_size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail3;

	/* Success!  */
	adev->gart.table_addr = adev->gart.ag_table_map->dm_segs[0].ds_addr;
	return 0;

fail4: __unused
	bus_dmamap_unload(adev->ddev->dmat, adev->gart.ag_table_map);
fail3:	bus_dmamem_unmap(adev->ddev->dmat, adev->gart.ptr,
	    adev->gart.table_size);
fail2:	bus_dmamap_destroy(adev->ddev->dmat, adev->gart.ag_table_map);
fail1:	bus_dmamem_free(adev->ddev->dmat, &adev->gart.ag_table_seg, 1);
fail0:	KASSERT(error);
	/* XXX errno NetBSD->Linux */
	return -error;
#else  /* __NetBSD__ */
	void *ptr;

	ptr = pci_alloc_consistent(adev->pdev, adev->gart.table_size,
				   &adev->gart.table_addr);
	if (ptr == NULL) {
		return -ENOMEM;
	}
#ifdef CONFIG_X86
	if (0) {
		set_memory_uc((unsigned long)ptr,
			      adev->gart.table_size >> PAGE_SHIFT);
	}
#endif
	adev->gart.ptr = ptr;
	memset((void *)adev->gart.ptr, 0, adev->gart.table_size);
	return 0;
#endif	/* __NetBSD__ */
}

/**
 * amdgpu_gart_table_ram_free - free system ram for gart page table
 *
 * @adev: amdgpu_device pointer
 *
 * Free system memory for GART page table
 * (r1xx-r3xx, non-pcie r4xx, rs400).  These asics require the
 * gart table to be in system memory.
 */
void amdgpu_gart_table_ram_free(struct amdgpu_device *adev)
{
	if (adev->gart.ptr == NULL) {
		return;
	}
#ifdef __NetBSD__
	bus_dmamap_unload(adev->ddev->dmat, adev->gart.ag_table_map);
	bus_dmamem_unmap(adev->ddev->dmat, adev->gart.ptr,
	    adev->gart.table_size);
	bus_dmamap_destroy(adev->ddev->dmat, adev->gart.ag_table_map);
	bus_dmamem_free(adev->ddev->dmat, &adev->gart.ag_table_seg, 1);
#else
#ifdef CONFIG_X86
	if (0) {
		set_memory_wb((unsigned long)adev->gart.ptr,
			      adev->gart.table_size >> PAGE_SHIFT);
	}
#endif
	pci_free_consistent(adev->pdev, adev->gart.table_size,
			    (void *)adev->gart.ptr,
			    adev->gart.table_addr);
	adev->gart.ptr = NULL;
	adev->gart.table_addr = 0;
#endif
}

/**
 * amdgpu_gart_table_vram_alloc - allocate vram for gart page table
 *
 * @adev: amdgpu_device pointer
 *
 * Allocate video memory for GART page table
 * (pcie r4xx, r5xx+).  These asics require the
 * gart table to be in video memory.
 * Returns 0 for success, error for failure.
 */
int amdgpu_gart_table_vram_alloc(struct amdgpu_device *adev)
{
	int r;

	if (adev->gart.robj == NULL) {
		r = amdgpu_bo_create(adev, adev->gart.table_size,
				     PAGE_SIZE, true, AMDGPU_GEM_DOMAIN_VRAM,
				     AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED,
				     NULL, NULL, &adev->gart.robj);
		if (r) {
			return r;
		}
	}
	return 0;
}

/**
 * amdgpu_gart_table_vram_pin - pin gart page table in vram
 *
 * @adev: amdgpu_device pointer
 *
 * Pin the GART page table in vram so it will not be moved
 * by the memory manager (pcie r4xx, r5xx+).  These asics require the
 * gart table to be in video memory.
 * Returns 0 for success, error for failure.
 */
int amdgpu_gart_table_vram_pin(struct amdgpu_device *adev)
{
	uint64_t gpu_addr;
	int r;

	r = amdgpu_bo_reserve(adev->gart.robj, false);
	if (unlikely(r != 0))
		return r;
	r = amdgpu_bo_pin(adev->gart.robj,
				AMDGPU_GEM_DOMAIN_VRAM, &gpu_addr);
	if (r) {
		amdgpu_bo_unreserve(adev->gart.robj);
		return r;
	}
	r = amdgpu_bo_kmap(adev->gart.robj, &adev->gart.ptr);
	if (r)
		amdgpu_bo_unpin(adev->gart.robj);
	amdgpu_bo_unreserve(adev->gart.robj);
	adev->gart.table_addr = gpu_addr;
	return r;
}

/**
 * amdgpu_gart_table_vram_unpin - unpin gart page table in vram
 *
 * @adev: amdgpu_device pointer
 *
 * Unpin the GART page table in vram (pcie r4xx, r5xx+).
 * These asics require the gart table to be in video memory.
 */
void amdgpu_gart_table_vram_unpin(struct amdgpu_device *adev)
{
	int r;

	if (adev->gart.robj == NULL) {
		return;
	}
	r = amdgpu_bo_reserve(adev->gart.robj, false);
	if (likely(r == 0)) {
		amdgpu_bo_kunmap(adev->gart.robj);
		amdgpu_bo_unpin(adev->gart.robj);
		amdgpu_bo_unreserve(adev->gart.robj);
		adev->gart.ptr = NULL;
	}
}

/**
 * amdgpu_gart_table_vram_free - free gart page table vram
 *
 * @adev: amdgpu_device pointer
 *
 * Free the video memory used for the GART page table
 * (pcie r4xx, r5xx+).  These asics require the gart table to
 * be in video memory.
 */
void amdgpu_gart_table_vram_free(struct amdgpu_device *adev)
{
	if (adev->gart.robj == NULL) {
		return;
	}
	amdgpu_bo_unref(&adev->gart.robj);
}

#ifdef __NetBSD__
static void
amdgpu_gart_pre_update(struct amdgpu_device *adev, unsigned gpu_pgstart,
    unsigned gpu_npages)
{

	if (adev->gart.ag_table_map != NULL) {
		const unsigned entsize =
		    adev->gart.table_size / adev->gart.num_gpu_pages;

		bus_dmamap_sync(adev->ddev->dmat, adev->gart.ag_table_map,
		    gpu_pgstart*entsize, gpu_npages*entsize,
		    BUS_DMASYNC_POSTWRITE);
	}
}

static void
amdgpu_gart_post_update(struct amdgpu_device *adev, unsigned gpu_pgstart,
    unsigned gpu_npages)
{

	if (adev->gart.ag_table_map != NULL) {
		const unsigned entsize =
		    adev->gart.table_size / adev->gart.num_gpu_pages;

		bus_dmamap_sync(adev->ddev->dmat, adev->gart.ag_table_map,
		    gpu_pgstart*entsize, gpu_npages*entsize,
		    BUS_DMASYNC_PREWRITE);
	}
	membar_sync();		/* XXX overkill */
	amdgpu_gart_flush_gpu_tlb(adev, 0);
}
#endif

/*
 * Common gart functions.
 */
#ifdef __NetBSD__
void
amdgpu_gart_unbind(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages)
{
	const unsigned gpu_per_cpu = (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE);
	const unsigned gpu_npages = (npages * gpu_per_cpu);
	const uint64_t gpu_pgstart = (gpu_start / AMDGPU_GPU_PAGE_SIZE);
	const uint64_t pgstart = (gpu_pgstart / gpu_per_cpu);
	uint64_t pgno, gpu_pgno;
	uint32_t flags = AMDGPU_PTE_SYSTEM;

	KASSERT(pgstart == (gpu_start / PAGE_SIZE));
	KASSERT(npages <= adev->gart.num_cpu_pages);
	KASSERT(gpu_npages <= adev->gart.num_cpu_pages);

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return;
	}

	amdgpu_gart_pre_update(adev, gpu_pgstart, gpu_npages);
	for (pgno = 0; pgno < npages; pgno++) {
		if (adev->gart.pages[pgstart + pgno] == NULL)
			continue;
		adev->gart.pages[pgstart + pgno] = NULL;
		adev->gart.pages_addr[pgstart + pgno] = adev->dummy_page.addr;

		if (adev->gart.ptr == NULL)
			continue;
		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++) {
			amdgpu_gart_set_pte_pde(adev, adev->gart.ptr,
			    gpu_pgstart + gpu_per_cpu*pgno + gpu_pgno,
			    adev->dummy_page.addr, flags);
		}
	}
	amdgpu_gart_post_update(adev, gpu_pgstart, gpu_npages);
}
#else  /* __NetBSD__ */
/**
 * amdgpu_gart_unbind - unbind pages from the gart page table
 *
 * @adev: amdgpu_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to unbind
 *
 * Unbinds the requested pages from the gart page table and
 * replaces them with the dummy page (all asics).
 */
void amdgpu_gart_unbind(struct amdgpu_device *adev, uint64_t offset,
			int pages)
{
	unsigned t;
	unsigned p;
	int i, j;
	u64 page_base;
	uint32_t flags = AMDGPU_PTE_SYSTEM;

	if (!adev->gart.ready) {
		WARN(1, "trying to unbind memory from uninitialized GART !\n");
		return;
	}

	t = offset / AMDGPU_GPU_PAGE_SIZE;
	p = t / (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE);
	for (i = 0; i < pages; i++, p++) {
		if (adev->gart.pages[p]) {
			adev->gart.pages[p] = NULL;
			adev->gart.pages_addr[p] = adev->dummy_page.addr;
			page_base = adev->gart.pages_addr[p];
			if (!adev->gart.ptr)
				continue;

			for (j = 0; j < (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE); j++, t++) {
				amdgpu_gart_set_pte_pde(adev, adev->gart.ptr,
							t, page_base, flags);
				page_base += AMDGPU_GPU_PAGE_SIZE;
			}
		}
	}
	mb();
	amdgpu_gart_flush_gpu_tlb(adev, 0);
}
#endif	/* __NetBSD__ */

#ifdef __NetBSD__
int
amdgpu_gart_bind(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages, struct page **pages, bus_dmamap_t dmamap, uint32_t flags)
{
	const unsigned gpu_per_cpu = (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE);
	const unsigned gpu_npages = (npages * gpu_per_cpu);
	const uint64_t gpu_pgstart = (gpu_start / AMDGPU_GPU_PAGE_SIZE);
	const uint64_t pgstart = (gpu_pgstart / gpu_per_cpu);
	uint64_t pgno, gpu_pgno;

	KASSERT(pgstart == (gpu_start / PAGE_SIZE));
	KASSERT(npages == dmamap->dm_nsegs);
	KASSERT(npages <= adev->gart.num_cpu_pages);
	KASSERT(gpu_npages <= adev->gart.num_cpu_pages);

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

	amdgpu_gart_pre_update(adev, gpu_pgstart, gpu_npages);
	for (pgno = 0; pgno < npages; pgno++) {
		const bus_addr_t addr = dmamap->dm_segs[pgno].ds_addr;

		KASSERT(dmamap->dm_segs[pgno].ds_len == PAGE_SIZE);
		adev->gart.pages[pgstart + pgno] = pages[pgno];
		adev->gart.pages_addr[pgstart + pgno] = addr;

		if (adev->gart.ptr == NULL)
			continue;

		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++) {
			amdgpu_gart_set_pte_pde(adev, adev->gart.ptr,
			    gpu_pgstart + gpu_per_cpu*pgno + gpu_pgno,
			    addr + gpu_pgno*AMDGPU_GPU_PAGE_SIZE, flags);
		}
	}
	amdgpu_gart_post_update(adev, gpu_pgstart, gpu_npages);

	return 0;
}
#else  /* __NetBSD__ */
/**
 * amdgpu_gart_bind - bind pages into the gart page table
 *
 * @adev: amdgpu_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to bind
 * @pagelist: pages to bind
 * @dma_addr: DMA addresses of pages
 *
 * Binds the requested pages to the gart page table
 * (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
int amdgpu_gart_bind(struct amdgpu_device *adev, uint64_t offset,
		     int pages, struct page **pagelist, dma_addr_t *dma_addr,
		     uint32_t flags)
{
	unsigned t;
	unsigned p;
	uint64_t page_base;
	int i, j;

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

	t = offset / AMDGPU_GPU_PAGE_SIZE;
	p = t / (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE);

	for (i = 0; i < pages; i++, p++) {
		adev->gart.pages_addr[p] = dma_addr[i];
		adev->gart.pages[p] = pagelist[i];
		if (adev->gart.ptr) {
			page_base = adev->gart.pages_addr[p];
			for (j = 0; j < (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE); j++, t++) {
				amdgpu_gart_set_pte_pde(adev, adev->gart.ptr, t, page_base, flags);
				page_base += AMDGPU_GPU_PAGE_SIZE;
			}
		}
	}
	mb();
	amdgpu_gart_flush_gpu_tlb(adev, 0);
	return 0;
}
#endif

/**
 * amdgpu_gart_init - init the driver info for managing the gart
 *
 * @adev: amdgpu_device pointer
 *
 * Allocate the dummy page and init the gart driver info (all asics).
 * Returns 0 for success, error for failure.
 */
int amdgpu_gart_init(struct amdgpu_device *adev)
{
	int r, i;

	if (adev->gart.pages) {
		return 0;
	}
	/* We need PAGE_SIZE >= AMDGPU_GPU_PAGE_SIZE */
	if (PAGE_SIZE < AMDGPU_GPU_PAGE_SIZE) {
		DRM_ERROR("Page size is smaller than GPU page size!\n");
		return -EINVAL;
	}
	r = amdgpu_dummy_page_init(adev);
	if (r)
		return r;
	/* Compute table size */
	adev->gart.num_cpu_pages = adev->mc.gtt_size / PAGE_SIZE;
	adev->gart.num_gpu_pages = adev->mc.gtt_size / AMDGPU_GPU_PAGE_SIZE;
	DRM_INFO("GART: num cpu pages %u, num gpu pages %u\n",
		 adev->gart.num_cpu_pages, adev->gart.num_gpu_pages);
	/* Allocate pages table */
	adev->gart.pages = vzalloc(sizeof(void *) * adev->gart.num_cpu_pages);
	if (adev->gart.pages == NULL) {
		amdgpu_gart_fini(adev);
		return -ENOMEM;
	}
	adev->gart.pages_addr = vzalloc(sizeof(dma_addr_t) *
					adev->gart.num_cpu_pages);
	if (adev->gart.pages_addr == NULL) {
		amdgpu_gart_fini(adev);
		return -ENOMEM;
	}
	/* set GART entry to point to the dummy page by default */
	for (i = 0; i < adev->gart.num_cpu_pages; i++) {
		adev->gart.pages_addr[i] = adev->dummy_page.addr;
	}
	return 0;
}

/**
 * amdgpu_gart_fini - tear down the driver info for managing the gart
 *
 * @adev: amdgpu_device pointer
 *
 * Tear down the gart driver info and free the dummy page (all asics).
 */
void amdgpu_gart_fini(struct amdgpu_device *adev)
{
	if (adev->gart.pages && adev->gart.pages_addr && adev->gart.ready) {
		/* unbind pages */
		amdgpu_gart_unbind(adev, 0, adev->gart.num_cpu_pages);
	}
	adev->gart.ready = false;
	vfree(adev->gart.pages);
	vfree(adev->gart.pages_addr);
	adev->gart.pages = NULL;
	adev->gart.pages_addr = NULL;

	amdgpu_dummy_page_fini(adev);
}
