/*-
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Eric Anholt <anholt@FreeBSD.org>
 *
 */

/** @file drm_scatter.c
 * Allocation of memory for scatter-gather mappings by the graphics chip.
 *
 * The memory allocated here is then made into an aperture in the card
 * by drm_ati_pcigart_init().
 */

#include "drmP.h"

#if defined(__FreeBSD__)
static void drm_sg_alloc_cb(void *arg, bus_dma_segment_t *segs,
			    int nsegs, int error);
#endif

int
drm_sg_alloc(struct drm_device *dev, struct drm_scatter_gather *request)
{
	struct drm_sg_mem *entry;
	struct drm_dma_handle *dmah;
	unsigned long pages;
	int ret;
#if defined(__NetBSD__)
	int nsegs, i, npage;
#endif

	if (dev->sg)
		return EINVAL;

	entry = malloc(sizeof(*entry), DRM_MEM_SGLISTS, M_WAITOK | M_ZERO);
	if (!entry)
		return ENOMEM;

	pages = round_page(request->size) / PAGE_SIZE;
	DRM_DEBUG("sg size=%ld pages=%ld\n", request->size, pages);

	entry->pages = pages;

	entry->busaddr = malloc(pages * sizeof(*entry->busaddr), DRM_MEM_PAGES,
	    M_WAITOK | M_ZERO);
	if (!entry->busaddr) {
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}

#if defined(__FreeBSD__)
	dmah = malloc(sizeof(struct drm_dma_handle), DRM_MEM_DMA,
	    M_ZERO | M_NOWAIT);
	if (dmah == NULL) {
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}

	ret = bus_dma_tag_create(NULL, PAGE_SIZE, 0, /* tag, align, boundary */
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, /* lowaddr, highaddr */
	    NULL, NULL, /* filtfunc, filtfuncargs */
	    request->size, pages, /* maxsize, nsegs */
	    PAGE_SIZE, 0, /* maxsegsize, flags */
	    NULL, NULL, /* lockfunc, lockfuncargs */
	    &dmah->tag);
	if (ret != 0) {
		free(dmah, DRM_MEM_DMA);
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}

	ret = bus_dmamem_alloc(dmah->tag, &dmah->vaddr,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO, &dmah->map);
	if (ret != 0) {
		bus_dma_tag_destroy(dmah->tag);
		free(dmah, DRM_MEM_DMA);
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}

	ret = bus_dmamap_load(dmah->tag, dmah->map, dmah->vaddr,
	    request->size, drm_sg_alloc_cb, entry,
	    BUS_DMA_NOWAIT | BUS_DMA_NOCACHE);
	if (ret != 0) {
		bus_dmamem_free(dmah->tag, dmah->vaddr, dmah->map);
		bus_dma_tag_destroy(dmah->tag);
		free(dmah, DRM_MEM_DMA);
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}
#elif   defined(__NetBSD__)
	dmah = malloc(sizeof(struct drm_dma_handle) +
		      (pages - 1) * sizeof(bus_dma_segment_t),
		      DRM_MEM_DMA, M_ZERO | M_NOWAIT);
	if (dmah == NULL) {
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}

	dmah->tag = dev->pa.pa_dmat;

	if ((ret = bus_dmamem_alloc(dmah->tag, request->size, PAGE_SIZE, 0,
				    dmah->segs, pages, &nsegs,
				    BUS_DMA_WAITOK)) != 0) {
		printf("drm: Unable to allocate %lu bytes of DMA, error %d\n",
		    request->size, ret);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}
	DRM_DEBUG("nsegs = %d\n", nsegs);
	dmah->nsegs = nsegs;
	if ((ret = bus_dmamem_map(dmah->tag, dmah->segs, nsegs, request->size,
				  &dmah->vaddr, BUS_DMA_NOWAIT |
				  BUS_DMA_NOCACHE | BUS_DMA_COHERENT)) != 0) {
		printf("drm: Unable to map DMA, error %d\n", ret);
		bus_dmamem_free(dmah->tag, dmah->segs, dmah->nsegs);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}
	if ((ret = bus_dmamap_create(dmah->tag, request->size, nsegs,
                                     request->size, 0, BUS_DMA_NOWAIT,
				     &dmah->map)) != 0) {
		printf("drm: Unable to create DMA map, error %d\n", ret);
		bus_dmamem_unmap(dmah->tag, dmah->vaddr, request->size);
		bus_dmamem_free(dmah->tag, dmah->segs, nsegs);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		free(entry->busaddr, DRM_MEM_PAGES);
		free(entry, DRM_MEM_SGLISTS);
		return ENOMEM;
	}
	if ((ret = bus_dmamap_load(dmah->tag, dmah->map, dmah->vaddr,
				   request->size, NULL,
				   BUS_DMA_NOWAIT | BUS_DMA_NOCACHE)) != 0) {
		printf("drm: Unable to load DMA map, error %d\n", ret);
		bus_dmamap_destroy(dmah->tag, dmah->map);
		bus_dmamem_unmap(dmah->tag, dmah->vaddr, request->size);
		bus_dmamem_free(dmah->tag, dmah->segs, dmah->nsegs);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}
	/*
	 * We are expected to return address for each page here.
	 * If bus_dmamem_alloc did not return each page in own segment
	 * (likely not), split them as if they were separate segments.
	 */
	for (i = 0, npage = 0 ; (i < nsegs) && (npage < pages) ; i++) {
		bus_addr_t base = dmah->map->dm_segs[i].ds_addr;
		bus_size_t offs;

		for (offs = 0;
		     (offs + PAGE_SIZE <= dmah->map->dm_segs[i].ds_len) &&
		     (npage < pages);
		     offs += PAGE_SIZE)
			entry->busaddr[npage++] = base + offs;
	}
	KASSERT(i == nsegs);
	KASSERT(npage == pages);
	dmah->size = request->size;
	memset(dmah->vaddr, 0, request->size);
#endif

	entry->sg_dmah = dmah;
	entry->handle = (unsigned long)dmah->vaddr;
	
	DRM_DEBUG("sg alloc handle  = %08lx\n", entry->handle);

	entry->virtual = (void *)entry->handle;
	request->handle = entry->handle;

	DRM_LOCK();
	if (dev->sg) {
		DRM_UNLOCK();
		drm_sg_cleanup(entry);
		return EINVAL;
	}
	dev->sg = entry;
	DRM_UNLOCK();

	return 0;
}

#if defined(__FreeBSD__)
static void
drm_sg_alloc_cb(void *arg, bus_dma_segment_t *segs, int nsegs, int error)
{
	struct drm_sg_mem *entry = arg;
	int i;

	if (error != 0)
	    return;

	for(i = 0 ; i < nsegs ; i++) {
		entry->busaddr[i] = segs[i].ds_addr;
	}
}
#endif

int
drm_sg_alloc_ioctl(struct drm_device *dev, void *data,
		   struct drm_file *file_priv)
{
	struct drm_scatter_gather *request = data;

	DRM_DEBUG("\n");

	return drm_sg_alloc(dev, request);
}

void
drm_sg_cleanup(struct drm_sg_mem *entry)
{
	struct drm_dma_handle *dmah = entry->sg_dmah;

#if defined(__FreeBSD__)
	bus_dmamap_unload(dmah->tag, dmah->map);
	bus_dmamem_free(dmah->tag, dmah->vaddr, dmah->map);
	bus_dma_tag_destroy(dmah->tag);
#elif   defined(__NetBSD__)
	bus_dmamap_unload(dmah->tag, dmah->map);
	bus_dmamap_destroy(dmah->tag, dmah->map);
	bus_dmamem_unmap(dmah->tag, dmah->vaddr, dmah->size);
	bus_dmamem_free(dmah->tag, dmah->segs, dmah->nsegs);
	dmah->tag = NULL;
#endif
	free(dmah, DRM_MEM_DMA);
	free(entry->busaddr, DRM_MEM_PAGES);
	free(entry, DRM_MEM_SGLISTS);
}

int
drm_sg_free(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_scatter_gather *request = data;
	struct drm_sg_mem *entry;

	DRM_LOCK();
	entry = dev->sg;
	dev->sg = NULL;
	DRM_UNLOCK();

	if (!entry || entry->handle != request->handle)
		return EINVAL;

	DRM_DEBUG("sg free virtual = 0x%lx\n", entry->handle);

	drm_sg_cleanup(entry);

	return 0;
}
