/* $NetBSD: drm_memory.c,v 1.9 2008/05/06 01:26:14 bjs Exp $ */

/* drm_memory.h -- Memory management wrappers for DRM -*- linux-c -*-
 * Created: Thu Feb  4 14:00:34 1999 by faith@valinux.com
 */
/*-
 *Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_memory.c,v 1.9 2008/05/06 01:26:14 bjs Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_memory.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"

#ifdef DRM_NO_AGP
#define NAGP_I810 0
#else
#include "agp_i810.h"
#if NAGP_I810 > 0 /* XXX hack to borrow agp's register mapping */
#include <dev/pci/agpvar.h>
#endif
#endif

MALLOC_DEFINE(M_DRM, "drm", "DRM Data Structures");

void drm_mem_init(void)
{
}

void drm_mem_uninit(void)
{
}

inline void *drm_mem_alloc(size_t size)
{
	return kmem_alloc(size, KM_NOSLEEP);
}

inline void *drm_mem_zalloc(size_t size)
{
	return kmem_zalloc(size, KM_NOSLEEP);
}

inline void *drm_mem_calloc(size_t nmemb, size_t size)
{
	return kmem_zalloc(size * nmemb, KM_NOSLEEP);
}

inline void *drm_mem_realloc(void *oldpt, size_t oldsize, size_t size)
{
	void *pt;

	pt = drm_mem_alloc(size);
	if (pt == NULL)
		return NULL;
	if (oldpt && oldsize) {
		memcpy(pt, oldpt, oldsize);
		drm_mem_free(oldpt, oldsize);
	}
	return pt;
}

inline void drm_mem_free(void *pt, size_t size)
{
#if 0
	KASSERT(pt == NULL)
#else
	if (pt == NULL) {
		DRM_DEBUG("drm_mem_free: told to free a null pointer!");
		return;
	}
#endif

	kmem_free(pt, size);
}

void *drm_ioremap(drm_device_t *dev, drm_local_map_t *map)
{
	int i, reg, reason;
	for(i = 0; i<DRM_MAX_PCI_RESOURCE; i++) {
		reg = PCI_MAPREG_START + i*4;
		if ((dev->pci_map_data[i].maptype == PCI_MAPREG_TYPE_MEM ||
		     dev->pci_map_data[i].maptype ==
                      (PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT)) &&
		    dev->pci_map_data[i].base == map->offset             &&
		    dev->pci_map_data[i].size >= map->size)
		{
			map->bst = dev->pa.pa_memt;
			map->cnt = &(dev->pci_map_data[i].mapped);
			map->mapsize = dev->pci_map_data[i].size;
			dev->pci_map_data[i].mapped++;
			if (dev->pci_map_data[i].mapped > 1)
			{
				map->bsh = dev->pci_map_data[i].bsh;
				return dev->pci_map_data[i].vaddr;
			}
			if ((reason = bus_space_map(map->bst, map->offset,
					dev->pci_map_data[i].size,
					dev->pci_map_data[i].flags, &map->bsh)))
			{
				dev->pci_map_data[i].mapped--;
#if NAGP_I810 > 0 /* XXX horrible kludge: agp might have mapped it */
				if (agp_i810_borrow(map->offset, &map->bsh))
					return bus_space_vaddr(map->bst, map->bsh);
#endif
				DRM_DEBUG("ioremap: failed to map (%d)\n",
					  reason);
				return NULL;
			}
			dev->pci_map_data[i].bsh = map->bsh;
			dev->pci_map_data[i].vaddr =
				 	bus_space_vaddr(map->bst, map->bsh);
			DRM_DEBUG("ioremap mem found: %p\n",
				dev->pci_map_data[i].vaddr);
			return dev->pci_map_data[i].vaddr;
		}
	}
	/* failed to find a valid mapping; all hope isn't lost though */
	for(i = 0; i<DRM_MAX_PCI_RESOURCE; i++) {
		if (dev->agp_map_data[i].mapped > 0 &&
		    dev->agp_map_data[i].base == map->offset &&
		    dev->agp_map_data[i].size >= map->size) {
			map->bst = dev->pa.pa_memt;
			map->cnt = &(dev->agp_map_data[i].mapped);
			map->mapsize = dev->agp_map_data[i].size;
			dev->agp_map_data[i].mapped++;
			map->bsh = dev->agp_map_data[i].bsh;
			return dev->agp_map_data[i].vaddr;
		}
		if (dev->agp_map_data[i].mapped == 0) {
			int rv;

			map->bst = dev->pa.pa_memt;
			dev->agp_map_data[i].mapped++;
			dev->agp_map_data[i].size = map->size;
			dev->agp_map_data[i].flags = BUS_SPACE_MAP_LINEAR;
			dev->agp_map_data[i].maptype = PCI_MAPREG_TYPE_MEM;
			map->cnt = &(dev->agp_map_data[i].mapped);
			map->mapsize = dev->agp_map_data[i].size;

			rv = bus_space_map(map->bst, map->offset,
				dev->agp_map_data[i].size,
				dev->agp_map_data[i].flags, &map->bsh);
			if (rv) {
				dev->agp_map_data[i].mapped--;
				DRM_DEBUG("ioremap: failed to map (%d)\n", rv);
				return NULL;
			}
			dev->agp_map_data[i].bsh = map->bsh;
			dev->agp_map_data[i].vaddr =
			    bus_space_vaddr(map->bst, map->bsh);
			DRM_DEBUG("ioremap agp mem found: %p\n",
				dev->agp_map_data[i].vaddr);
			return dev->agp_map_data[i].vaddr;
		}
	}

	/* now we can give up... */
	DRM_DEBUG("drm_ioremap failed: offset=%lx size=%lu\n",
		  map->offset, map->size);
	return NULL;
}

void drm_ioremapfree(drm_local_map_t *map)
{
	if (map->cnt == NULL) {
		DRM_INFO("drm_ioremapfree called for unknown map\n");
		return;
	}
	if (*(map->cnt) > 0) {
		(*(map->cnt))--;
		if(*(map->cnt) == 0)
			bus_space_unmap(map->bst, map->bsh, map->mapsize);
	}
}

int
drm_mtrr_add(unsigned long offset, size_t size, int flags)
{
#ifndef DRM_NO_MTRR
	struct mtrr mtrrmap;
	int one = 1;

	DRM_DEBUG("offset=%lx size=%ld\n", (long)offset, (long)size);
	mtrrmap.base = offset;
	mtrrmap.len = size;
	mtrrmap.type = flags;
	mtrrmap.flags = MTRR_VALID;
	return mtrr_set(&mtrrmap, &one, NULL, MTRR_GETSET_KERNEL);
#else
	return 0;
#endif
}

int
drm_mtrr_del(int __unused handle, unsigned long offset, size_t size, int flags)
{
#ifndef DRM_NO_MTRR
	struct mtrr mtrrmap;
	int one = 1;

	DRM_DEBUG("offset=%lx size=%ld\n", (long)offset, (long)size);
	mtrrmap.base = offset;
	mtrrmap.len = size;
	mtrrmap.type = flags;
	mtrrmap.flags = 0;
	return mtrr_set(&mtrrmap, &one, NULL, MTRR_GETSET_KERNEL);
#else
	return 0;
#endif
}

inline struct drm_dmamem *
drm_dmamem_alloc(bus_dma_tag_t tag, size_t size, int wait)
{
	struct drm_dmamem *dma;
	int kmflag;

	kmflag = (wait & DRM_DMA_NOWAIT) != 0 ? KM_NOSLEEP : KM_SLEEP;

	dma = kmem_zalloc(sizeof(*dma), kmflag);
	if (dma == NULL)
		return NULL;

	dma->dm_size = size;
	dma->dm_tag = tag;

	if (bus_dmamap_create(dma->dm_tag, size, size / PAGE_SIZE + 1, size, 0,
	    wait|BUS_DMA_ALLOCNOW, &dma->dm_map) != 0)
		goto dmafree;

	if (bus_dmamem_alloc(dma->dm_tag, size, PAGE_SIZE, 0, dma->dm_segs,
	    1, &dma->dm_nsegs, wait) != 0)
		goto destroy;

	if (bus_dmamem_map(dma->dm_tag, dma->dm_segs, dma->dm_nsegs, size,
	    &dma->dm_kva, wait|BUS_DMA_COHERENT) != 0)
		goto free;

	if (bus_dmamap_load(dma->dm_tag, dma->dm_map, dma->dm_kva, size,
	    NULL, wait) != 0)
		goto unmap;

	return dma;

unmap:
	bus_dmamem_unmap(dma->dm_tag, dma->dm_kva, size);
free:
	bus_dmamem_free(dma->dm_tag, dma->dm_segs, dma->dm_nsegs);
destroy:
	bus_dmamap_destroy(dma->dm_tag, dma->dm_map);
dmafree:
	kmem_free(dma, sizeof(*dma));

	return NULL;
}

inline void
drm_dmamem_free(struct drm_dmamem *dma)
{
	if (dma != NULL) {
		bus_dmamap_unload(dma->dm_tag, dma->dm_map);
		bus_dmamem_unmap(dma->dm_tag, dma->dm_kva, dma->dm_size);
		bus_dmamem_free(dma->dm_tag, dma->dm_segs, dma->dm_nsegs);
		bus_dmamap_destroy(dma->dm_tag, dma->dm_map);
		kmem_free(dma, sizeof(*dma));
	}
}
