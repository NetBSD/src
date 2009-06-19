/*-
 * Copyright 2003 Eric Anholt
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
 * ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** @file drm_vm.c
 * Support code for mmaping of DRM maps.
 */

#include "drmP.h"
#include "drm.h"

#if defined(__FreeBSD__)
int drm_mmap(struct cdev *kdev, vm_offset_t offset, vm_paddr_t *paddr,
    int prot)
#elif   defined(__NetBSD__)
paddr_t drm_mmap(dev_t kdev, off_t offset, int prot)
#endif
{
	struct drm_device *dev = drm_get_device_from_kdev(kdev);
	struct drm_file *file_priv = NULL;
	drm_local_map_t *map;
	enum drm_map_type type;
#if defined(__FreeBSD__)
	vm_paddr_t phys;
	int error;
#elif   defined(__NetBSD__)
    paddr_t phys;
	unsigned long map_offs;
#endif

#if defined(__FreeBSD__)
	/* d_mmap gets called twice, we can only reference file_priv during
	 * the first call.  We need to assume that if error is EBADF the
	 * call was succesful and the client is authenticated.
	 */
	error = devfs_get_cdevpriv((void **)&file_priv);
	if (error == ENOENT) {
		DRM_ERROR("Could not find authenticator!\n");
		return EINVAL;
	}
#elif   defined(__NetBSD__)
    DRM_LOCK();
    file_priv = drm_find_file_by_proc(dev, DRM_CURPROC);
    DRM_UNLOCK();
    if (file_priv == NULL) {
		DRM_ERROR("Could not find authenticator!\n");
		return -1;
    }
#endif

	if (file_priv && !file_priv->authenticated)
#if defined(__NetBSD__)
		return -1;
#else
		return EACCES;
#endif

	if (dev->dma && offset >= 0 && offset < ptoa(dev->dma->page_count)) {
		drm_device_dma_t *dma = dev->dma;

		DRM_SPINLOCK(&dev->dma_lock);

		if (dma->pagelist != NULL) {
			unsigned long page = offset >> PAGE_SHIFT;
			unsigned long physaddr = dma->pagelist[page];

			DRM_SPINUNLOCK(&dev->dma_lock);
#if defined(__FreeBSD__)
			*paddr = physaddr;
			return 0;
#elif   defined(__NetBSD__)
            return atop(physaddr);
#endif
		} else {
			DRM_SPINUNLOCK(&dev->dma_lock);
			return -1;
		}
	}

				/* A sequential search of a linked list is
				   fine here because: 1) there will only be
				   about 5-10 entries in the list and, 2) a
				   DRI client only has to do this mapping
				   once, so it doesn't have to be optimized
				   for performance, even if the list was a
				   bit longer. */
	DRM_LOCK();
	TAILQ_FOREACH(map, &dev->maplist, link) {
		if (offset >= map->offset && offset < map->offset + map->size)
			break;
	}

	if (map == NULL) {
		DRM_DEBUG("Can't find map, requested offset = %" PRIx64 "\n",
		    offset);
		TAILQ_FOREACH(map, &dev->maplist, link) {
			DRM_DEBUG("map offset = %016lx, handle = %016lx\n",
			map->offset, (unsigned long)map->handle);
		}
		DRM_UNLOCK();
		return -1;
	}
	if (((map->flags&_DRM_RESTRICTED) && !DRM_SUSER(DRM_CURPROC))) {
		DRM_UNLOCK();
		DRM_DEBUG("restricted map\n");
		return -1;
	}
	type = map->type;
#if	defined(__NetBSD__)
	map_offs = map->offset;
#endif
	DRM_UNLOCK();

	switch (type) {
	case _DRM_FRAME_BUFFER:
	case _DRM_REGISTERS:
	case _DRM_AGP:
#if	defined(__NetBSD__)
		phys = bus_space_mmap(dev->pa.pa_memt, map->offset,
				offset - map->offset, prot, BUS_SPACE_MAP_LINEAR);
		if (phys == -1) {
			DRM_ERROR("bus_space_mmap for %" PRIx64 " failed\n", offset);
			return -1;	
		}
		return phys;
#else
		phys = offset;
#endif
		break;
	case _DRM_CONSISTENT:
		phys = vtophys((vaddr_t)((char *)map->handle + (offset - map->offset)));
		break;
	case _DRM_SCATTER_GATHER:
	case _DRM_SHM:
		phys = vtophys(offset);
		break;
	default:
		DRM_ERROR("bad map type %d\n", type);
		return -1;	/* This should never happen. */
	}

#if defined(__FreeBSD__)
	*paddr = phys;
	return 0;
#elif   defined(__NetBSD__)
    return atop(phys);
#endif
}

