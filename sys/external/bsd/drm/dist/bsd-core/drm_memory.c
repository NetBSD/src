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

/** @file drm_memory.c
 * Wrappers for kernel memory allocation routines, and MTRR management support.
 *
 * This file previously implemented a memory consumption tracking system using
 * the "area" argument for various different types of allocations, but that
 * has been stripped out for now.
 */

#include "drmP.h"

#if defined(__NetBSD__)
# ifdef DRM_NO_AGP
#  define NAGP_I810 0
# else
#  if defined(_KERNEL_OPT)
#    include "agp_i810.h"
#  else
#   define NAGP_I810 1
#  endif
# endif
# if NAGP_I810 > 0
#  include <dev/pci/agpvar.h>
# endif
#endif

MALLOC_DEFINE(DRM_MEM_DMA, "drm_dma", "DRM DMA Data Structures");
MALLOC_DEFINE(DRM_MEM_SAREA, "drm_sarea", "DRM SAREA Data Structures");
MALLOC_DEFINE(DRM_MEM_DRIVER, "drm_driver", "DRM DRIVER Data Structures");
MALLOC_DEFINE(DRM_MEM_MAGIC, "drm_magic", "DRM MAGIC Data Structures");
MALLOC_DEFINE(DRM_MEM_IOCTLS, "drm_ioctls", "DRM IOCTL Data Structures");
MALLOC_DEFINE(DRM_MEM_MAPS, "drm_maps", "DRM MAP Data Structures");
MALLOC_DEFINE(DRM_MEM_BUFS, "drm_bufs", "DRM BUFFER Data Structures");
MALLOC_DEFINE(DRM_MEM_SEGS, "drm_segs", "DRM SEGMENTS Data Structures");
MALLOC_DEFINE(DRM_MEM_PAGES, "drm_pages", "DRM PAGES Data Structures");
MALLOC_DEFINE(DRM_MEM_FILES, "drm_files", "DRM FILE Data Structures");
MALLOC_DEFINE(DRM_MEM_QUEUES, "drm_queues", "DRM QUEUE Data Structures");
MALLOC_DEFINE(DRM_MEM_CMDS, "drm_cmds", "DRM COMMAND Data Structures");
MALLOC_DEFINE(DRM_MEM_MAPPINGS, "drm_mapping", "DRM MAPPING Data Structures");
MALLOC_DEFINE(DRM_MEM_BUFLISTS, "drm_buflists", "DRM BUFLISTS Data Structures");
MALLOC_DEFINE(DRM_MEM_AGPLISTS, "drm_agplists", "DRM AGPLISTS Data Structures");
MALLOC_DEFINE(DRM_MEM_CTXBITMAP, "drm_ctxbitmap",
    "DRM CTXBITMAP Data Structures");
MALLOC_DEFINE(DRM_MEM_SGLISTS, "drm_sglists", "DRM SGLISTS Data Structures");
MALLOC_DEFINE(DRM_MEM_DRAWABLE, "drm_drawable", "DRM DRAWABLE Data Structures");

void drm_mem_init(void)
{
}

void drm_mem_uninit(void)
{
}

#if defined(__NetBSD__)
static void *
drm_netbsd_ioremap(struct drm_device *dev, drm_local_map_t *map, int wc)
{
	bus_space_handle_t h;
	int i, reg, reason;
	for(i = 0; i<DRM_MAX_PCI_RESOURCE; i++) {
		reg = PCI_MAPREG_START + i*4;

		/* Does the requested mapping lie within this resource? */
		if ((dev->pci_map_data[i].maptype == PCI_MAPREG_TYPE_MEM ||
		     dev->pci_map_data[i].maptype ==
                      (PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT)) &&
		    map->offset >= dev->pci_map_data[i].base             &&
		    map->offset + map->size <= dev->pci_map_data[i].base +
                                              dev->pci_map_data[i].size)
		{
			map->bst = dev->pa.pa_memt;
			map->fullmap = &(dev->pci_map_data[i]);
			map->mapsize = map->size;
			dev->pci_map_data[i].mapped++;

			/* If we've already mapped this resource in, handle
			 * submapping if needed, give caller a bus_space handle
			 * and pointer for the offest they asked for */
			if (dev->pci_map_data[i].mapped > 1)
			{
				if ((reason = bus_space_subregion(
						dev->pa.pa_memt,
						dev->pci_map_data[i].bsh,
						map->offset - dev->pci_map_data[i].base,
						map->size, &h)) != 0)  {
					DRM_DEBUG("ioremap failed to "
						"bus_space_subregion: %d\n",
						reason);
					return NULL;
				}
				map->bsh = h;
				map->handle = bus_space_vaddr(dev->pa.pa_memt,
									h);
				return map->handle;
			}

			/* Map in entirety of resource - full size and handle
			 * go in pci_map_data, specific mapping in callers
			 * drm_local_map_t */
			DRM_DEBUG("ioremap%s: flags %d\n", wc ? "_wc" : "",
				  dev->pci_map_data[i].flags);
			if ((reason = bus_space_map(map->bst,
					dev->pci_map_data[i].base,
					dev->pci_map_data[i].size,
					dev->pci_map_data[i].flags,
					&dev->pci_map_data[i].bsh)))
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

			dev->pci_map_data[i].vaddr = bus_space_vaddr(map->bst,
						dev->pci_map_data[i].bsh);

			/* Caller might have requested a submapping of that */
			if ((reason = bus_space_subregion(
					dev->pa.pa_memt,
					dev->pci_map_data[i].bsh,
					map->offset - dev->pci_map_data[i].base,
					map->size, &h)) != 0)  {
				DRM_DEBUG("ioremap failed to "
					"bus_space_subregion: %d\n",
					reason);
				return NULL;
			}

			DRM_DEBUG("ioremap mem found for %lx, %lx: %p\n",
				map->offset, map->size,
				dev->agp_map_data[i].vaddr);

			map->bsh = h;
			map->handle = bus_space_vaddr(dev->pa.pa_memt, h);
			return map->handle;
		}
	}
	/* failed to find a valid mapping; all hope isn't lost though */
	for(i = 0; i<DRM_MAX_PCI_RESOURCE; i++) {
		if (dev->agp_map_data[i].mapped > 0 &&
		    dev->agp_map_data[i].base == map->offset &&
		    dev->agp_map_data[i].size >= map->size) {
			map->bst = dev->pa.pa_memt;
			map->fullmap = &(dev->agp_map_data[i]);
			map->mapsize = dev->agp_map_data[i].size;
			dev->agp_map_data[i].mapped++;
			map->bsh = dev->agp_map_data[i].bsh;
			return dev->agp_map_data[i].vaddr;
		}
		if (dev->agp_map_data[i].mapped == 0) {
			int rv;

			map->bst = dev->pa.pa_memt;
			dev->agp_map_data[i].mapped++;
			dev->agp_map_data[i].base = map->offset;
			dev->agp_map_data[i].size = map->size;
			dev->agp_map_data[i].flags = BUS_SPACE_MAP_LINEAR;
			dev->agp_map_data[i].maptype = PCI_MAPREG_TYPE_MEM;
			map->fullmap = &(dev->agp_map_data[i]);
			map->mapsize = dev->agp_map_data[i].size;

			DRM_DEBUG("ioremap%s: flags %d\n", wc ? "_wc" : "",
				  dev->agp_map_data[i].flags);
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
			DRM_DEBUG("ioremap agp mem found for %lx, %lx: %p\n",
				map->offset, map->size,
				dev->agp_map_data[i].vaddr);
			return dev->agp_map_data[i].vaddr;
		}
	}

	/* now we can give up... */
	DRM_DEBUG("drm_ioremap failed: offset=%lx size=%lu\n",
		  map->offset, map->size);
	return NULL;
}
#endif

void *drm_ioremap_wc(struct drm_device *dev, drm_local_map_t *map)
{
#if defined(__FreeBSD__)
	return pmap_mapdev_attr(map->offset, map->size, PAT_WRITE_COMBINING);
#elif   defined(__NetBSD__)
	return drm_netbsd_ioremap(dev, map, 1);
#endif
}

void *drm_ioremap(struct drm_device *dev, drm_local_map_t *map)
{
#if defined(__FreeBSD__)
	return pmap_mapdev(map->offset, map->size);
#elif   defined(__NetBSD__)
	return drm_netbsd_ioremap(dev, map, 0);
#endif
}

void drm_ioremapfree(drm_local_map_t *map)
{
#if defined(__FreeBSD__)
	pmap_unmapdev((vm_offset_t) map->handle, map->size);
#elif   defined(__NetBSD__)
	if (map->fullmap == NULL) {
		DRM_INFO("drm_ioremapfree called for unknown map\n");
		return;
	}

	if (map->fullmap->mapped > 0) {
		map->fullmap->mapped--;
		if(map->fullmap->mapped == 0)
			bus_space_unmap(map->bst, map->fullmap->bsh,
					map->fullmap->size);
	}
#endif
}

#if defined(__FreeBSD__)
int
drm_mtrr_add(unsigned long offset, size_t size, int flags)
{
	int act;
	struct mem_range_desc mrdesc;

	mrdesc.mr_base = offset;
	mrdesc.mr_len = size;
	mrdesc.mr_flags = flags;
	act = MEMRANGE_SET_UPDATE;
	strlcpy(mrdesc.mr_owner, "drm", sizeof(mrdesc.mr_owner));
	return mem_range_attr_set(&mrdesc, &act);
}

int
drm_mtrr_del(int __unused handle, unsigned long offset, size_t size, int flags)
{
	int act;
	struct mem_range_desc mrdesc;

	mrdesc.mr_base = offset;
	mrdesc.mr_len = size;
	mrdesc.mr_flags = flags;
	act = MEMRANGE_SET_REMOVE;
	strlcpy(mrdesc.mr_owner, "drm", sizeof(mrdesc.mr_owner));
	return mem_range_attr_set(&mrdesc, &act);
}
#elif   defined(__NetBSD__)
int
drm_mtrr_add(unsigned long offset, size_t size, int flags)
{
	struct mtrr mtrrmap;
	int one = 1;

	mtrrmap.base = offset;
	mtrrmap.len = size;
	mtrrmap.type = flags;
	mtrrmap.flags = MTRR_VALID;
	return mtrr_set(&mtrrmap, &one, NULL, MTRR_GETSET_KERNEL);
}

int
drm_mtrr_del(unsigned long offset, size_t size, int flags)
{
	struct mtrr mtrrmap;
	int one = 1;

	mtrrmap.base = offset;
	mtrrmap.len = size;
	mtrrmap.type = flags;
	mtrrmap.flags = 0;
	return mtrr_set(&mtrrmap, &one, NULL, MTRR_GETSET_KERNEL);
}
#endif
