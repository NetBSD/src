/* $NetBSD: drm_scatter.c,v 1.7.10.1 2009/05/13 17:19:17 jym Exp $ */

/* drm_scatter.h -- IOCTLs to manage scatter/gather memory -*- linux-c -*-
 * Created: Mon Dec 18 23:20:54 2000 by gareth@valinux.com */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_scatter.c,v 1.7.10.1 2009/05/13 17:19:17 jym Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_scatter.c,v 1.3 2006/05/17 06:29:36 anholt Exp $");
*/

/** @file drm_scatter.c
 * Allocation of memory for scatter-gather mappings by the graphics chip.
 *
 * The memory allocated here is then made into an aperture in the card
 * by drm_ati_pcigart_init().
 */

#include "drmP.h"

int
drm_sg_alloc(struct drm_device *dev, struct drm_scatter_gather *request)
{
	drm_sg_mem_t *entry;
	struct drm_dma_handle *dmah;
	unsigned long pages;
	int nsegs, ret, i;

	entry = kmem_zalloc(sizeof(*entry), KM_SLEEP);
	if ( !entry )
		return ENOMEM;

	pages = round_page(request->size) / PAGE_SIZE;
	DRM_DEBUG( "sg size=%ld pages=%ld\n", request->size, pages );

	entry->pages = pages;

	entry->busaddr = kmem_zalloc(pages * sizeof(*entry->busaddr), KM_SLEEP);
	if ( !entry->busaddr ) {
		kmem_free(entry, sizeof(*entry));
		return ENOMEM;
	}

	dmah = kmem_zalloc(sizeof(struct drm_dma_handle), KM_SLEEP);
	if (dmah == NULL) {
		kmem_free(entry->busaddr, pages * sizeof(*entry->busaddr));
		kmem_free(entry, sizeof(*entry));
		return ENOMEM;
	}

	dmah->dmat = dev->pa.pa_dmat;

	ret = bus_dmamem_alloc(dmah->dmat, request->size, PAGE_SIZE, 0,
	    dmah->segs, 1, &nsegs, 0);
	if (ret != 0) {
		kmem_free(dmah, sizeof(struct drm_dma_handle));
		kmem_free(entry->busaddr, pages * sizeof(*entry->busaddr));
		kmem_free(entry, sizeof(*entry));
		return ENOMEM;
	}

	ret = bus_dmamem_map(dmah->dmat, dmah->segs, nsegs, request->size,
	     &dmah->addr, BUS_DMA_COHERENT);
	if (ret != 0) {
		bus_dmamem_free(dmah->dmat, dmah->segs, nsegs);
		kmem_free(dmah, sizeof(struct drm_dma_handle));
		kmem_free(entry->busaddr, pages * sizeof(*entry->busaddr));
		kmem_free(entry, sizeof(*entry));
		return ENOMEM;
	}

	ret = bus_dmamap_create(dmah->dmat, request->size, pages, PAGE_SIZE,
		0, 0, &dmah->map);

	if (ret != 0) {
		bus_dmamem_unmap(dmah->dmat, dmah->addr, request->size);
		bus_dmamem_free(dmah->dmat, dmah->segs, nsegs);
		kmem_free(dmah, sizeof(struct drm_dma_handle));
		kmem_free(entry->busaddr, pages * sizeof(*entry->busaddr));
		kmem_free(entry, sizeof(*entry));
		return ENOMEM;
	}

	ret = bus_dmamap_load(dmah->dmat, dmah->map, dmah->addr, request->size,
	    NULL, 0);
	if (ret != 0) {
		bus_dmamem_unmap(dmah->dmat, dmah->addr, request->size);
		bus_dmamem_free(dmah->dmat, dmah->segs, nsegs);
		kmem_free(dmah, sizeof(struct drm_dma_handle));
		kmem_free(entry->busaddr, pages * sizeof(*entry->busaddr));
		kmem_free(entry, sizeof(*entry));
		return ENOMEM;
	}

	DRM_DEBUG( "sg alloc nsegs = %d\n", dmah->map->dm_nsegs);

        for (i = 0; i < pages; i++) {
            entry->busaddr[i] = dmah->map->dm_segs[i].ds_addr;
	}

	entry->handle = (unsigned long)dmah->addr;
	entry->sg_dmah = dmah;
	
	DRM_DEBUG( "sg alloc handle  = %08lx\n", entry->handle );

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

int
drm_sg_alloc_ioctl(DRM_IOCTL_ARGS)
{
	DRM_DEVICE;
	drm_scatter_gather_t request;
	int ret;

	if ( dev->sg )
		return EINVAL;

	DRM_DEBUG( "%s\n", __FUNCTION__ );

	DRM_COPY_FROM_USER_IOCTL(request, (drm_scatter_gather_t *)data,
			     sizeof(request) );

	ret = drm_sg_alloc(dev, &request);
	if (ret != 0)
		return ret;

	DRM_COPY_TO_USER_IOCTL( (drm_scatter_gather_t *)data,
			   request,
			   sizeof(request) );

	return 0;
}

void
drm_sg_cleanup(drm_sg_mem_t *entry)
{
	struct drm_dma_handle *dmah = entry->sg_dmah;

	bus_dmamap_unload(dmah->dmat, dmah->map);
	bus_dmamem_unmap(dmah->dmat, dmah->addr, entry->pages << PAGE_SHIFT);
	bus_dmamem_free(dmah->dmat, dmah->segs, 1);
	kmem_free(dmah, sizeof(struct drm_dma_handle));
	kmem_free(entry->busaddr, entry->pages * sizeof(*entry->busaddr));
	kmem_free(entry, sizeof(*entry));
}

int
drm_sg_free_ioctl(DRM_IOCTL_ARGS)
{
	DRM_DEVICE;
	drm_scatter_gather_t request;
	drm_sg_mem_t *entry;

	DRM_COPY_FROM_USER_IOCTL( request, (drm_scatter_gather_t *)data,
			     sizeof(request) );

	DRM_LOCK();
	entry = dev->sg;
	if ( !entry || entry->handle != request.handle ) {
		DRM_UNLOCK();
		DRM_DEBUG( "sg free: error: %s handle\n", entry ? "invalid" : "no allocated");
		return EINVAL;
	}
	dev->sg = NULL;
	DRM_UNLOCK();

	DRM_DEBUG( "sg free virtual  = 0x%lx\n", entry->handle );

	drm_sg_cleanup(entry);

	return 0;
}
