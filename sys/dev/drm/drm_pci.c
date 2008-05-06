/* $NetBSD: drm_pci.c,v 1.9 2008/05/06 01:26:14 bjs Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.9 2008/05/06 01:26:14 bjs Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_pci.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"

drm_dma_handle_t *
drm_pci_alloc(drm_device_t *dev, size_t size, size_t align, dma_addr_t maxaddr)
{
	drm_dma_handle_t *hdl;
	struct drm_dmamem *dmam;

	hdl = drm_mem_zalloc(sizeof(drm_dma_handle_t));
	if (hdl == NULL)
		goto err;

	dmam = drm_dmamem_alloc(dev->pa.pa_dmat, size, DRM_DMA_NOWAIT);
	if (dmam == NULL)
		goto free;

	hdl->busaddr = DRM_NETBSD_DMA_ADDR(dmam);
	hdl->vaddr = DRM_NETBSD_DMA_VADDR(dmam);
	hdl->size = size;
	return hdl;

free:
	drm_mem_free(hdl, sizeof(*hdl));
err:
	return NULL;
	
}

void
drm_pci_free(drm_device_t *dev, drm_dma_handle_t *hdl)
{
	if (hdl != NULL) {
		if (hdl->dmam != NULL)
			drm_dmamem_free(hdl->dmam);
	drm_mem_free(hdl, sizeof(*hdl));
	}
}
