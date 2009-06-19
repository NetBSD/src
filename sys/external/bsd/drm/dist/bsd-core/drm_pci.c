/*-
 * Copyright 2003 Eric Anholt.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file drm_pci.h
 * \brief PCI consistent, DMA-accessible memory allocation.
 *
 * \author Eric Anholt <anholt@FreeBSD.org>
 */

#include "drmP.h"

/**********************************************************************/
/** \name PCI memory */
/*@{*/

#if defined(__FreeBSD__)
static void
drm_pci_busdma_callback(void *arg, bus_dma_segment_t *segs, int nsegs, int error)
{
	drm_dma_handle_t *dmah = arg;

	if (error != 0)
		return;

	KASSERT(nsegs == 1, ("drm_pci_busdma_callback: bad dma segment count"));
	dmah->busaddr = segs[0].ds_addr;
}
#endif

/**
 * \brief Allocate a physically contiguous DMA-accessible consistent 
 * memory block.
 */
drm_dma_handle_t *
drm_pci_alloc(struct drm_device *dev, size_t size,
	      size_t align, dma_addr_t maxaddr)
{
	drm_dma_handle_t *dmah;
	int ret;
#if defined (__NetBSD__)
    int nsegs;
#endif

	/* Need power-of-two alignment, so fail the allocation if it isn't. */
	if ((align & (align - 1)) != 0) {
		DRM_ERROR("drm_pci_alloc with non-power-of-two alignment %d\n",
		    (int)align);
		return NULL;
	}

	dmah = malloc(sizeof(drm_dma_handle_t), DRM_MEM_DMA, M_ZERO | M_NOWAIT);
	if (dmah == NULL)
		return NULL;

#if defined__FreeBSD__
	/* Make sure we aren't holding locks here */
	mtx_assert(&dev->dev_lock, MA_NOTOWNED);
	if (mtx_owned(&dev->dev_lock))
	    DRM_ERROR("called while holding dev_lock\n");
	mtx_assert(&dev->dma_lock, MA_NOTOWNED);
	if (mtx_owned(&dev->dma_lock))
	    DRM_ERROR("called while holding dma_lock\n");

	ret = bus_dma_tag_create(NULL, align, 0, /* tag, align, boundary */
	    maxaddr, BUS_SPACE_MAXADDR, /* lowaddr, highaddr */
	    NULL, NULL, /* filtfunc, filtfuncargs */
	    size, 1, size, /* maxsize, nsegs, maxsegsize */
	    0, NULL, NULL, /* flags, lockfunc, lockfuncargs */
	    &dmah->tag);
	if (ret != 0) {
		free(dmah, DRM_MEM_DMA);
		return NULL;
	}

	ret = bus_dmamem_alloc(dmah->tag, &dmah->vaddr,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO, &dmah->map);
	if (ret != 0) {
		bus_dma_tag_destroy(dmah->tag);
		free(dmah, DRM_MEM_DMA);
		return NULL;
	}

	ret = bus_dmamap_load(dmah->tag, dmah->map, dmah->vaddr, size,
	    drm_pci_busdma_callback, dmah, BUS_DMA_NOWAIT | BUS_DMA_NOCACHE);
	if (ret != 0) {
		bus_dmamem_free(dmah->tag, dmah->vaddr, dmah->map);
		bus_dma_tag_destroy(dmah->tag);
		free(dmah, DRM_MEM_DMA);
		return NULL;
	}
#elif   defined(__NetBSD__)
	dmah->tag = dev->pa.pa_dmat;

	if ((ret = bus_dmamem_alloc(dmah->tag, size, align, maxaddr,
	    dmah->segs, 1, &nsegs, BUS_DMA_WAITOK)) != 0) {
		printf("drm: Unable to allocate %zu bytes of DMA, error %d\n",
		    size, ret);
        dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
        return NULL;
	}
	/* XXX is there a better way to deal with this? */
	if (nsegs != 1) {
		printf("drm: bad segment count from bus_dmamem_alloc\n");
        bus_dmamem_free(dmah->tag, dmah->segs, 1);
        dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
        return NULL;
	}
	if ((ret = bus_dmamem_map(dmah->tag, dmah->segs, nsegs, size, 
	     &dmah->vaddr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT | BUS_DMA_NOCACHE)) != 0) {
		printf("drm: Unable to map DMA, error %d\n", ret);
        bus_dmamem_free(dmah->tag, dmah->segs, 1);
        dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
        return NULL;
	}
	if ((ret = bus_dmamap_create(dmah->tag, size, 1, size, maxaddr,
	     BUS_DMA_NOWAIT, &dmah->map)) != 0) {
		printf("drm: Unable to create DMA map, error %d\n", ret);
        bus_dmamem_unmap(dmah->tag, dmah->vaddr, size);
        bus_dmamem_free(dmah->tag, dmah->segs, 1);
        dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
        return NULL;
	}
	if ((ret = bus_dmamap_load(dmah->tag, dmah->map, dmah->vaddr, 
	     size, NULL, BUS_DMA_NOWAIT | BUS_DMA_NOCACHE)) != 0) {
		printf("drm: Unable to load DMA map, error %d\n", ret);
        bus_dmamap_destroy(dmah->tag, dmah->map);
        bus_dmamem_unmap(dmah->tag, dmah->vaddr, size);
        bus_dmamem_free(dmah->tag, dmah->segs, 1);
        dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
        return NULL;
	}
	dmah->busaddr = dmah->map->dm_segs[0].ds_addr;
	dmah->size = size;
    dmah->nsegs = 1;
    memset(dmah->vaddr, 0, size);
#endif

	return dmah;
}

/**
 * \brief Free a DMA-accessible consistent memory block.
 */
void
drm_pci_free(struct drm_device *dev, drm_dma_handle_t *dmah)
{
	if (dmah == NULL)
		return;

#if defined(__FreeBSD__)
	bus_dmamem_free(dmah->tag, dmah->vaddr, dmah->map);
	bus_dma_tag_destroy(dmah->tag);
#elif   defined(__NetBSD__)
	bus_dmamap_unload(dmah->tag, dmah->map);
	bus_dmamap_destroy(dmah->tag, dmah->map);
	bus_dmamem_unmap(dmah->tag, dmah->vaddr, dmah->size);
	bus_dmamem_free(dmah->tag, dmah->segs, 1);
	dmah->tag = NULL;
#endif

	free(dmah, DRM_MEM_DMA);
}

/*@}*/
