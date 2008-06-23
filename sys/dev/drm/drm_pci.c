/* $NetBSD: drm_pci.c,v 1.10.2.1 2008/06/23 04:31:01 wrstuden Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.10.2.1 2008/06/23 04:31:01 wrstuden Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_pci.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"

drm_dma_handle_t *
drm_pci_alloc(drm_device_t *dev, size_t size, size_t align, dma_addr_t maxaddr)
{
	drm_dma_handle_t *h;

	h = malloc(sizeof(drm_dma_handle_t), M_DRM, M_ZERO | M_NOWAIT);

	if (h == NULL)
		return NULL;

	h->mem = drm_dmamem_pgalloc(dev, round_page(size) >> PAGE_SHIFT);
	if (h->mem == NULL) {
	free(h, M_DRM);
	return NULL;
	}

	h->busaddr = DRM_DMAMEM_BUSADDR(h->mem);
	h->vaddr = DRM_DMAMEM_KERNADDR(h->mem);
	h->size = h->mem->dd_dmam->dm_mapsize;

	return h;
	
}

/*
 * 	Free a DMA-accessible consistent memory block.
 */

void
drm_pci_free(drm_device_t *dev, drm_dma_handle_t *h)
{
	if (h == NULL)
		return;

	drm_dmamem_free(h->mem);
	free(h, M_DRM);
}
