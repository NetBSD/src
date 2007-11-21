/* $NetBSD: drm_pci.c,v 1.4.20.1 2007/11/21 21:19:31 bouyer Exp $ */

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
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_pci.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"

/* What might happen in age of Aquarius:
 *
 *
 * Allocate a physically contiguous DMA-accessible consistent 
 * memory block.
 */

/* NetBSD reality:
 *
 * XXX We must fix this mess! This is a horrible misuse of bus_dma(9),
 *     and I am surprised it works at all.  			 
 */

drm_dma_handle_t *
drm_pci_alloc(drm_device_t *dev, size_t size, size_t align, dma_addr_t maxaddr)
{
	drm_dma_handle_t *dmah;
	int ret;
	int nsegs;

	/* Need power-of-two alignment, so fail the allocation if it isn't. */
	if ((align & (align - 1)) != 0) {
		DRM_ERROR("drm_pci_alloc with non-power-of-two alignment %d\n",
		    (int)align);
		return NULL;
	}

	dmah = malloc(sizeof(drm_dma_handle_t), M_DRM, M_ZERO | M_NOWAIT);
	if (dmah == NULL)
		return NULL;

	ret = bus_dmamem_alloc(dev->pa.pa_dmat, size, align, 0,
	    &dmah->seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (ret != 0) {
		aprint_error("%s: bus_dmamem_alloc(%zd, %zd) returned %d\n",
		    __func__, size, align, ret);
		free(dmah, M_DRM);
		return NULL;
	}
	if(nsegs != 1) {
		aprint_error("%s: bus_dmamem_alloc(%zd) returned %d segments\n",
		    __func__, size, nsegs);
		bus_dmamem_free(dev->pa.pa_dmat, &dmah->seg, nsegs);
		free(dmah, M_DRM);
		return NULL;
	}

	ret = bus_dmamem_map(dev->pa.pa_dmat, &dmah->seg, 1, size, &dmah->addr,
	    BUS_DMA_NOWAIT);
	if (ret != 0) {
		bus_dmamem_free(dev->pa.pa_dmat, &dmah->seg, 1);
		aprint_error("%s: bus_dmamem_map() failed %d\n", __func__,
		    ret);
		free(dmah, M_DRM);
		return NULL;
	}

	dmah->busaddr = dmah->seg.ds_addr;
	dmah->vaddr = dmah->addr;

	return dmah;
}

/*
 * 	Free a DMA-accessible consistent memory block.
 */

void
drm_pci_free(drm_device_t *dev, drm_dma_handle_t *dmah)
{
	if (dmah == NULL)
		return;

	bus_dmamem_free(dev->pa.pa_dmat, &dmah->seg, 1);

	free(dmah, M_DRM);
}
