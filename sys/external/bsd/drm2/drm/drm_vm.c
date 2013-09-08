/*	$NetBSD: drm_vm.c,v 1.1.2.2 2013/09/08 15:44:14 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_vm.c,v 1.1.2.2 2013/09/08 15:44:14 riastradh Exp $");

#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <drm/drmP.h>

static paddr_t	drm_mmap_dma_paddr(struct drm_device *, off_t, int);
static paddr_t	drm_mmap_map_paddr(struct drm_device *, struct drm_local_map *,
		    off_t, int);

paddr_t
drm_mmap_paddr(struct drm_device *dev, off_t byte_offset, int prot)
{
	const off_t page_offset = (byte_offset >> PAGE_SHIFT);
	struct drm_hash_item *hash;
	paddr_t paddr = (paddr_t)-1;
	int error;

	if (byte_offset != (byte_offset & ~(PAGE_SIZE-1)))
		return (paddr_t)-1;

	mutex_lock(&dev->struct_mutex);

	if ((dev->dma != NULL) &&
	    (0 <= byte_offset) &&
	    (byte_offset <= (dev->dma->page_count << PAGE_SHIFT))) {
		paddr = drm_mmap_dma_paddr(dev, byte_offset, prot);
		goto out;
	}

	if (drm_ht_find_item(&dev->map_hash, page_offset, &hash))
		goto out;

	struct drm_local_map *const map = drm_hash_entry(hash,
	    struct drm_map_list, hash)->map;
	if (map == NULL)
		goto out;

	/*
	 * XXX FreeBSD drops the mutex at this point, which would be
	 * nice, to allow sleeping in bus_dma cruft, but I don't know
	 * what guarantees the map will continue to exist.
	 */

	if (ISSET(map->flags, _DRM_RESTRICTED) && !DRM_SUSER())
		goto out;

	if (byte_offset < map->offset)
		goto out;

	paddr = drm_mmap_map_paddr(dev, map, (map->offset - byte_offset),
	    prot);

out:	mutex_unlock(&dev->struct_mutex);
	return error;
}

static paddr_t
drm_mmap_dma_paddr(struct drm_device *dev, off_t byte_offset, int prot)
{
	const off_t page_offset = (byte_offset >> PAGE_SHIFT);

	/* XXX Is this the right mutex?  */
	KASSERT(mutex_is_locked(&dev->struct_mutex));

	if (dev->dma->pagelist == NULL)
		return (paddr_t)-1;

	if (page_offset >= dev->dma->page_count)
		return (paddr_t)-1;

	return dev->dma->pagelist[page_offset];
}

static paddr_t
drm_mmap_map_paddr(struct drm_device *dev, struct drm_local_map *map,
    off_t byte_offset, int prot)
{
	int flags = 0;

	switch (map->type) {
	case _DRM_FRAME_BUFFER:
	case _DRM_AGP:
		flags |= BUS_SPACE_MAP_PREFETCHABLE;
		/* Fall through.  */

	case _DRM_REGISTERS:
		flags |= BUS_SPACE_MAP_LINEAR; /* XXX Why?  */

		return bus_space_mmap(dev->bst, map->offset, byte_offset, prot,
		    flags);

	case _DRM_CONSISTENT: {
		struct drm_dma_handle *const dmah = map->lm_data.dmah;

		return bus_dmamem_mmap(dev->dmat, &dmah->dmah_seg, 1,
		    byte_offset, prot,
		    /* XXX BUS_DMA_WAITOK?  We're holding a mutex...  */
		    /* XXX What else?  BUS_DMA_COHERENT?  */
		    (BUS_DMA_WAITOK | BUS_DMA_NOCACHE));
	}

	case _DRM_SCATTER_GATHER: {
		struct drm_sg_mem *const sg = dev->sg;

#if 0				/* XXX */
		KASSERT(sg == map->lm_data.sg);
#endif

		return bus_dmamem_mmap(dev->dmat, sg->sg_segs, sg->sg_nsegs,
		    byte_offset, prot,
		    /* XXX BUS_DMA_WAITOK?  We're holding a mutex...  */
		    /* XXX What else?  BUS_DMA_COHERENT?  */
		    (BUS_DMA_WAITOK | BUS_DMA_NOCACHE));
	}

	case _DRM_SHM:
	case _DRM_GEM:
	default:
		return (paddr_t)-1;
	}
}
