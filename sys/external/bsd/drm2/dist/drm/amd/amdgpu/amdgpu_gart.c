/*	$NetBSD: amdgpu_gart.c,v 1.10.4.2 2024/10/04 11:40:50 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: amdgpu_gart.c,v 1.10.4.2 2024/10/04 11:40:50 martin Exp $");

#include <linux/pci.h>
#include <linux/vmalloc.h>

#include <drm/amdgpu_drm.h>
#ifdef CONFIG_X86
#include <asm/set_memory.h>
#endif
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
 * amdgpu_dummy_page_init - init dummy page used by the driver
 *
 * @adev: amdgpu_device pointer
 *
 * Allocate the dummy page used by the driver (all asics).
 * This dummy page is used by the driver as a filler for gart entries
 * when pages are taken out of the GART
 * Returns 0 on sucess, -ENOMEM on failure.
 */
static int amdgpu_gart_dummy_page_init(struct amdgpu_device *adev)
{
#ifdef __NetBSD__
	int rsegs;
	void *p;
	int error;

	/* XXX Can this be called more than once??  */
	if (adev->dummy_page_map != NULL)
		return 0;

	error = bus_dmamem_alloc(adev->ddev->dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &adev->dummy_page_seg, 1, &rsegs, BUS_DMA_WAITOK);
	if (error)
		goto fail0;
	KASSERT(rsegs == 1);
	error = bus_dmamem_map(adev->ddev->dmat, &adev->dummy_page_seg, 1,
	    PAGE_SIZE, &p, BUS_DMA_WAITOK);
	if (error)
		goto fail1;
	memset(p, 0, PAGE_SIZE);
	bus_dmamem_unmap(adev->ddev->dmat, p, PAGE_SIZE);
	error = bus_dmamap_create(adev->ddev->dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &adev->dummy_page_map);
	if (error)
		goto fail1;
	error = bus_dmamap_load_raw(adev->ddev->dmat, adev->dummy_page_map,
	    &adev->dummy_page_seg, 1, PAGE_SIZE, BUS_DMA_WAITOK);
	if (error)
		goto fail2;

	bus_dmamap_sync(adev->ddev->dmat, adev->dummy_page_map, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* Success!  */
	adev->dummy_page_addr = adev->dummy_page_map->dm_segs[0].ds_addr;
	return 0;

fail3: __unused
	bus_dmamap_unload(adev->ddev->dmat, adev->dummy_page_map);
fail2:	bus_dmamap_destroy(adev->ddev->dmat, adev->dummy_page_map);
fail1:	bus_dmamem_free(adev->ddev->dmat, &adev->dummy_page_seg, 1);
fail0:	KASSERT(error);
	adev->dummy_page_map = NULL;
	adev->dummy_page_addr = 0; /* paranoia */
	/* XXX errno NetBSD->Linux */
	return -error;
#else  /* __NetBSD__ */
	struct page *dummy_page = ttm_bo_glob.dummy_read_page;

	if (adev->dummy_page_addr)
		return 0;
	adev->dummy_page_addr = pci_map_page(adev->pdev, dummy_page, 0,
					     PAGE_SIZE, PCI_DMA_BIDIRECTIONAL);
	if (pci_dma_mapping_error(adev->pdev, adev->dummy_page_addr)) {
		dev_err(&adev->pdev->dev, "Failed to DMA MAP the dummy page\n");
		adev->dummy_page_addr = 0;
		return -ENOMEM;
	}
	return 0;
#endif	/* __NetBSD__ */
}

/**
 * amdgpu_dummy_page_fini - free dummy page used by the driver
 *
 * @adev: amdgpu_device pointer
 *
 * Frees the dummy page used by the driver (all asics).
 */
static void amdgpu_gart_dummy_page_fini(struct amdgpu_device *adev)
{
	if (!adev->dummy_page_addr)
		return;
#ifdef __NetBSD__
	bus_dmamap_unload(adev->ddev->dmat, adev->dummy_page_map);
	bus_dmamap_destroy(adev->ddev->dmat, adev->dummy_page_map);
	bus_dmamem_free(adev->ddev->dmat, &adev->dummy_page_seg, 1);
	adev->dummy_page_map = NULL;
#else
	pci_unmap_page(adev->pdev, adev->dummy_page_addr,
		       PAGE_SIZE, PCI_DMA_BIDIRECTIONAL);
#endif
	adev->dummy_page_addr = 0;
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

	if (adev->gart.bo == NULL) {
		struct amdgpu_bo_param bp;

		memset(&bp, 0, sizeof(bp));
		bp.size = adev->gart.table_size;
		bp.byte_align = PAGE_SIZE;
		bp.domain = AMDGPU_GEM_DOMAIN_VRAM;
		bp.flags = AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
			AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS;
		bp.type = ttm_bo_type_kernel;
		bp.resv = NULL;
		r = amdgpu_bo_create(adev, &bp, &adev->gart.bo);
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
	int r;

	r = amdgpu_bo_reserve(adev->gart.bo, false);
	if (unlikely(r != 0))
		return r;
	r = amdgpu_bo_pin(adev->gart.bo, AMDGPU_GEM_DOMAIN_VRAM);
	if (r) {
		amdgpu_bo_unreserve(adev->gart.bo);
		return r;
	}
	r = amdgpu_bo_kmap(adev->gart.bo, &adev->gart.ptr);
	if (r)
		amdgpu_bo_unpin(adev->gart.bo);
	amdgpu_bo_unreserve(adev->gart.bo);
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

	if (adev->gart.bo == NULL) {
		return;
	}
	r = amdgpu_bo_reserve(adev->gart.bo, true);
	if (likely(r == 0)) {
		amdgpu_bo_kunmap(adev->gart.bo);
		amdgpu_bo_unpin(adev->gart.bo);
		amdgpu_bo_unreserve(adev->gart.bo);
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
	if (adev->gart.bo == NULL) {
		return;
	}
	amdgpu_bo_unref(&adev->gart.bo);
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
	unsigned i;

	if (adev->gart.ag_table_map != NULL) {
		const unsigned entsize =
		    adev->gart.table_size / adev->gart.num_gpu_pages;

		bus_dmamap_sync(adev->ddev->dmat, adev->gart.ag_table_map,
		    gpu_pgstart*entsize, gpu_npages*entsize,
		    BUS_DMASYNC_PREWRITE);
	}
	mb();			/* XXX why is bus_dmamap_sync not enough? */
	amdgpu_asic_flush_hdp(adev, NULL);
	for (i = 0; i < adev->num_vmhubs; i++)
		amdgpu_gmc_flush_gpu_tlb(adev, 0, i, 0);
}
#endif

/*
 * Common gart functions.
 */
#ifdef __NetBSD__
int
amdgpu_gart_unbind(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages)
{
	const unsigned gpu_per_cpu = AMDGPU_GPU_PAGES_IN_CPU_PAGE;
	const unsigned gpu_npages = (npages * gpu_per_cpu);
	const uint64_t gpu_pgstart = (gpu_start / AMDGPU_GPU_PAGE_SIZE);
	const uint64_t pgstart __diagused = (gpu_pgstart / gpu_per_cpu);
	uint64_t pgno, gpu_pgno;
	uint32_t flags = AMDGPU_PTE_SYSTEM;

	KASSERT(pgstart == (gpu_start / PAGE_SIZE));
	KASSERT(npages <= adev->gart.num_cpu_pages);
	KASSERT(gpu_npages <= adev->gart.num_cpu_pages);

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

	amdgpu_gart_pre_update(adev, gpu_pgstart, gpu_npages);
	for (pgno = 0; pgno < npages; pgno++) {
#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
		adev->gart.pages[pgstart + pgno] = NULL;
#endif

		if (adev->gart.ptr == NULL)
			continue;
		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++) {
			amdgpu_gmc_set_pte_pde(adev, adev->gart.ptr,
			    gpu_pgstart + gpu_per_cpu*pgno + gpu_pgno,
			    adev->dummy_page_addr, flags);
		}
	}
	amdgpu_gart_post_update(adev, gpu_pgstart, gpu_npages);

	return 0;
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
 * Returns 0 for success, -EINVAL for failure.
 */
int amdgpu_gart_unbind(struct amdgpu_device *adev, uint64_t offset,
			int pages)
{
	unsigned t;
	unsigned p;
	int i, j;
	u64 page_base;
	/* Starting from VEGA10, system bit must be 0 to mean invalid. */
	uint64_t flags = 0;

	if (!adev->gart.ready) {
		WARN(1, "trying to unbind memory from uninitialized GART !\n");
		return -EINVAL;
	}

	t = offset / AMDGPU_GPU_PAGE_SIZE;
	p = t / AMDGPU_GPU_PAGES_IN_CPU_PAGE;
	for (i = 0; i < pages; i++, p++) {
#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
		adev->gart.pages[p] = NULL;
#endif
		page_base = adev->dummy_page_addr;
		if (!adev->gart.ptr)
			continue;

		for (j = 0; j < AMDGPU_GPU_PAGES_IN_CPU_PAGE; j++, t++) {
			amdgpu_gmc_set_pte_pde(adev, adev->gart.ptr,
					       t, page_base, flags);
			page_base += AMDGPU_GPU_PAGE_SIZE;
		}
	}
	mb();
	amdgpu_asic_flush_hdp(adev, NULL);
	for (i = 0; i < adev->num_vmhubs; i++)
		amdgpu_gmc_flush_gpu_tlb(adev, 0, i, 0);

	return 0;
}
#endif	/* __NetBSD__ */

/**
 * amdgpu_gart_map - map dma_addresses into GART entries
 *
 * @adev: amdgpu_device pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to bind
 * @dma_addr: DMA addresses of pages
 * @flags: page table entry flags
 * @dst: CPU address of the gart table
 *
 * Map the dma_addresses into GART entries (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
#ifdef __NetBSD__
int amdgpu_gart_map(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages, bus_size_t map_start, bus_dmamap_t dmamap, uint32_t flags,
    void *dst)
{
	bus_size_t seg_off = 0;
	unsigned i, j, t;

	CTASSERT(AMDGPU_GPU_PAGE_SIZE <= PAGE_SIZE);
	CTASSERT((PAGE_SIZE % AMDGPU_GPU_PAGE_SIZE) == 0);

	KASSERT((gpu_start & (PAGE_SIZE - 1)) == 0);

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

	for (i = 0; i < dmamap->dm_nsegs; i++) {
		KASSERT((dmamap->dm_segs[i].ds_len & (PAGE_SIZE - 1)) == 0);
		if (map_start == 0)
			break;
		if (map_start < dmamap->dm_segs[i].ds_len) {
			seg_off = map_start;
			break;
		}
		map_start -= dmamap->dm_segs[i].ds_len;
	}
	KASSERT(i < dmamap->dm_nsegs);

	t = gpu_start / AMDGPU_GPU_PAGE_SIZE;

	for (i = 0; npages --> 0;) {
		for (j = 0; j < AMDGPU_GPU_PAGES_IN_CPU_PAGE; j++, t++) {
			KASSERT(i < dmamap->dm_nsegs);
			KASSERT(seg_off < dmamap->dm_segs[i].ds_len);
			amdgpu_gmc_set_pte_pde(adev, dst, t,
			    dmamap->dm_segs[i].ds_addr + seg_off, flags);
			seg_off += AMDGPU_GPU_PAGE_SIZE;
			if (seg_off == dmamap->dm_segs[i].ds_len) {
				i++;
				seg_off = 0;
			}
		}
	}

	return 0;
}
#else
int amdgpu_gart_map(struct amdgpu_device *adev, uint64_t offset,
		    int pages, dma_addr_t *dma_addr, uint64_t flags,
		    void *dst)
{
	uint64_t page_base;
	unsigned i, j, t;

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

	t = offset / AMDGPU_GPU_PAGE_SIZE;

	for (i = 0; i < pages; i++) {
		page_base = dma_addr[i];
		for (j = 0; j < AMDGPU_GPU_PAGES_IN_CPU_PAGE; j++, t++) {
			amdgpu_gmc_set_pte_pde(adev, dst, t, page_base, flags);
			page_base += AMDGPU_GPU_PAGE_SIZE;
		}
	}
	return 0;
}
#endif	/* __NetBSD__ */

#ifdef __NetBSD__
int
amdgpu_gart_bind(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages, struct page **pages, bus_dmamap_t dmamap, uint32_t flags)
{
	const unsigned gpu_per_cpu = AMDGPU_GPU_PAGES_IN_CPU_PAGE;
	const unsigned gpu_npages = (npages * gpu_per_cpu);
	const uint64_t gpu_pgstart = (gpu_start / AMDGPU_GPU_PAGE_SIZE);
	const uint64_t pgstart __diagused = (gpu_pgstart / gpu_per_cpu);
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
#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
		adev->gart.pages[pgstart + pgno] = NULL;
#endif

		if (adev->gart.ptr == NULL)
			continue;

		for (gpu_pgno = 0; gpu_pgno < gpu_per_cpu; gpu_pgno++) {
			amdgpu_gmc_set_pte_pde(adev, adev->gart.ptr,
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
 * @flags: page table entry flags
 *
 * Binds the requested pages to the gart page table
 * (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
int amdgpu_gart_bind(struct amdgpu_device *adev, uint64_t offset,
		     int pages, struct page **pagelist, dma_addr_t *dma_addr,
		     uint64_t flags)
{
#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
	unsigned t,p;
#endif
	int r, i;

	if (!adev->gart.ready) {
		WARN(1, "trying to bind memory to uninitialized GART !\n");
		return -EINVAL;
	}

#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
	t = offset / AMDGPU_GPU_PAGE_SIZE;
	p = t / AMDGPU_GPU_PAGES_IN_CPU_PAGE;
	for (i = 0; i < pages; i++, p++)
		adev->gart.pages[p] = pagelist ? pagelist[i] : NULL;
#endif

	if (!adev->gart.ptr)
		return 0;

	r = amdgpu_gart_map(adev, offset, pages, dma_addr, flags,
		    adev->gart.ptr);
	if (r)
		return r;

	mb();
	amdgpu_asic_flush_hdp(adev, NULL);
	for (i = 0; i < adev->num_vmhubs; i++)
		amdgpu_gmc_flush_gpu_tlb(adev, 0, i, 0);
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
	int r;

	if (adev->dummy_page_addr)
		return 0;

	/* We need PAGE_SIZE >= AMDGPU_GPU_PAGE_SIZE */
	if (PAGE_SIZE < AMDGPU_GPU_PAGE_SIZE) {
		DRM_ERROR("Page size is smaller than GPU page size!\n");
		return -EINVAL;
	}
	r = amdgpu_gart_dummy_page_init(adev);
	if (r)
		return r;
	/* Compute table size */
	adev->gart.num_cpu_pages = adev->gmc.gart_size / PAGE_SIZE;
	adev->gart.num_gpu_pages = adev->gmc.gart_size / AMDGPU_GPU_PAGE_SIZE;
	DRM_INFO("GART: num cpu pages %u, num gpu pages %u\n",
		 adev->gart.num_cpu_pages, adev->gart.num_gpu_pages);

#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
	/* Allocate pages table */
	adev->gart.pages = vzalloc(array_size(sizeof(void *),
					      adev->gart.num_cpu_pages));
	if (adev->gart.pages == NULL)
		return -ENOMEM;
#endif

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
#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
	vfree(adev->gart.pages);
	adev->gart.pages = NULL;
#endif
	amdgpu_gart_dummy_page_fini(adev);
}
