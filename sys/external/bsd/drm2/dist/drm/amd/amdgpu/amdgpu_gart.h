/*	$NetBSD: amdgpu_gart.h,v 1.4 2021/12/19 12:21:29 riastradh Exp $	*/

/*
 * Copyright 2017 Advanced Micro Devices, Inc.
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
 */

#ifndef __AMDGPU_GART_H__
#define __AMDGPU_GART_H__

#include <linux/types.h>

/*
 * GART structures, functions & helpers
 */
struct amdgpu_device;
struct amdgpu_bo;

#define AMDGPU_GPU_PAGE_SIZE 4096
#define AMDGPU_GPU_PAGE_MASK (AMDGPU_GPU_PAGE_SIZE - 1)
#define AMDGPU_GPU_PAGE_SHIFT 12
#define AMDGPU_GPU_PAGE_ALIGN(a) (((a) + AMDGPU_GPU_PAGE_MASK) & ~AMDGPU_GPU_PAGE_MASK)

#define AMDGPU_GPU_PAGES_IN_CPU_PAGE (PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE)

struct amdgpu_gart {
#ifdef __NetBSD__
	bus_dma_segment_t		ag_table_seg;
	bus_dmamap_t			ag_table_map;
#endif
	struct amdgpu_bo		*bo;
	/* CPU kmapped address of gart table */
	void				*ptr;
	unsigned			num_gpu_pages;
	unsigned			num_cpu_pages;
	unsigned			table_size;
#ifdef CONFIG_DRM_AMDGPU_GART_DEBUGFS
	struct page			**pages;
#endif
	bool				ready;

	/* Asic default pte flags */
	uint64_t			gart_pte_flags;
};

int amdgpu_gart_table_vram_alloc(struct amdgpu_device *adev);
void amdgpu_gart_table_vram_free(struct amdgpu_device *adev);
int amdgpu_gart_table_vram_pin(struct amdgpu_device *adev);
void amdgpu_gart_table_vram_unpin(struct amdgpu_device *adev);
int amdgpu_gart_init(struct amdgpu_device *adev);
void amdgpu_gart_fini(struct amdgpu_device *adev);
#ifdef __NetBSD__
int amdgpu_gart_unbind(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages);
int amdgpu_gart_map(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages, bus_size_t map_start, bus_dmamap_t map, uint32_t flags,
    void *dst);
int amdgpu_gart_bind(struct amdgpu_device *adev, uint64_t gpu_start,
    unsigned npages, struct page **pagelist, bus_dmamap_t dmamap,
    uint32_t flags);
#else
int amdgpu_gart_unbind(struct amdgpu_device *adev, uint64_t offset,
		       int pages);
int amdgpu_gart_map(struct amdgpu_device *adev, uint64_t offset,
		    int pages, dma_addr_t *dma_addr, uint64_t flags,
		    void *dst);
int amdgpu_gart_bind(struct amdgpu_device *adev, uint64_t offset,
		     int pages, struct page **pagelist,
		     dma_addr_t *dma_addr, uint64_t flags);
#endif

#endif
