/* $NetBSD: drm_pci.c,v 1.5.2.2 2007/12/26 21:39:24 ad Exp $ */

/*
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.5.2.2 2007/12/26 21:39:24 ad Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_pci.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"

drm_dma_handle_t *
drm_pci_alloc(drm_device_t *dev, size_t size, size_t align, dma_addr_t maxaddr)
{
	drm_dma_handle_t *h;
	int error, rseg;


	/* Need power-of-two alignment, so fail the allocation if it isn't. */
	if ((align & (align - 1)) != 0) {
		DRM_ERROR("drm_pci_alloc with non-power-of-two alignment %d\n",
		    (int)align);
		return NULL;
	}

	h = malloc(sizeof(drm_dma_handle_t), M_DRM, M_ZERO | M_NOWAIT);
	if (!h)
		return NULL;
	h->size = size;
	if ((error = bus_dmamem_alloc(dev->pa.pa_dmat, size, align, 0,
	    &h->seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("drm: Unable to allocate DMA, error %d\n", error);
		goto fail;
	}
	if ((error = bus_dmamem_map(dev->pa.pa_dmat, &h->seg, rseg, size, 
	     &h->addr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		printf("drm: Unable to map DMA, error %d\n", error);
	     	goto free;
	}
	if ((error = bus_dmamap_create(dev->pa.pa_dmat, size, 1, size, 0,
	     BUS_DMA_NOWAIT, &h->map)) != 0) {
		printf("drm: Unable to create DMA map, error %d\n", error);
		goto unmap;
	}
	if ((error = bus_dmamap_load(dev->pa.pa_dmat, h->map, h->addr, size,
	     NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("drm: Unable to load DMA map, error %d\n", error);
		goto destroy;
	}
	h->busaddr = h->seg.ds_addr;
	h->vaddr = h->addr;

	return h;

destroy:
	bus_dmamap_destroy(dev->pa.pa_dmat, h->map);
unmap:
	bus_dmamem_unmap(dev->pa.pa_dmat, h->addr, size);
free:
	bus_dmamem_free(dev->pa.pa_dmat, h->addr, 1);
fail:
	free(h, M_DRM);
	return NULL;
	
}

/*
 * 	Free a DMA-accessible consistent memory block.
 */

void
drm_pci_free(drm_device_t *dev, drm_dma_handle_t *h)
{
	if (!h)
		return;
	bus_dmamap_unload(dev->pa.pa_dmat, h->map);
	bus_dmamap_destroy(dev->pa.pa_dmat, h->map);
	bus_dmamem_unmap(dev->pa.pa_dmat, h->addr, h->size);
	bus_dmamem_free(dev->pa.pa_dmat, &h->seg, 1);

	free(h, M_DRM);
}
