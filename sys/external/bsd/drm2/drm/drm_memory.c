/*	$NetBSD: drm_memory.c,v 1.1.2.4 2013/07/24 02:47:50 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_memory.c,v 1.1.2.4 2013/07/24 02:47:50 riastradh Exp $");

/* XXX Cargo-culted from the old drm_memory.c.  */

#ifdef _KERNEL_OPT
#include "agp_i810.h"
#include "genfb.h"
#else
#define	NAGP_I810	1	/* XXX WTF?  */
#define	NGENFB		0	/* XXX WTF?  */
#endif

#include <sys/bus.h>

#if NAGP_I810 > 0
/* XXX include order botch -- shouldn't need to include pcivar.h */
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>
#endif

#if NGENFB > 0
#include <dev/wsfb/genfbvar.h>
#endif

#include <drm/drmP.h>

/*
 * XXX drm_bus_borrow is a horrible kludge!
 */
static bool
drm_bus_borrow(bus_addr_t base, bus_space_handle_t *handlep)
{

#if NAGP_I810 > 0
	if (agp_i810_borrow(base, handlep))
		return true;
#endif

#if NGENFB > 0
	if (genfb_borrow(base, handlep))
		return true;
#endif

	return false;
}

void *
drm_ioremap(struct drm_device *dev, struct drm_local_map *map)
{
	const bus_space_tag_t bst = dev->bst;
	unsigned int unit;
	int error;

	/*
	 * Search dev's bus maps for a match.
	 */
	for (unit = 0; unit < dev->bus_nmaps; unit++) {
		struct drm_bus_map *const bm = &dev->bus_maps[unit];

		/* Reject maps starting after the request.  */
		if (map->offset < bm->bm_base)
			continue;

		/* Reject maps smaller than the request.  */
		if (bm->bm_size < map->size)
			continue;

		/*
		 * Reject maps that the request doesn't fit in.  (Make
		 * sure to avoid integer overflow.)
		 */
		if ((bm->bm_size - map->size) <
		    (map->offset - bm->bm_base))
			continue;

		/* Has it been mapped yet?  If not, map it.  */
		if (bm->bm_mapped == 0) {
			KASSERT(ISSET(bm->bm_flags, BUS_SPACE_MAP_LINEAR));
			error = bus_space_map(bst, bm->bm_base,
			    bm->bm_size, bm->bm_flags, &bm->bm_bsh);
			if (error) {
				if (drm_bus_borrow(map->offset,
					&map->lm_data.bus_space.bsh)) {
					map->lm_data.bus_space.bus_map = NULL;
					goto win;
				}
				return NULL;
			}
		}

		/* Mark it used and make a subregion just for the request.  */
		bm->bm_mapped++;
		error = bus_space_subregion(bst, bm->bm_bsh,
		    map->offset - bm->bm_base, map->size,
		    &map->lm_data.bus_space.bsh);
		if (error) {
			/*
			 * Back out: unmark it and, if nobody else was
			 * using it, unmap it.
			 */
			if (--bm->bm_mapped == 0)
				bus_space_unmap(bst, bm->bm_bsh,
				    bm->bm_size);
			return NULL;
		}

		/* Got it!  */
		map->lm_data.bus_space.bus_map = bm;
		goto win;
	}

	/*
	 * No dice.  Try mapping it directly ourselves.
	 *
	 * XXX Is this sensible?  What prevents us from clobbering some
	 * existing map?  And what does this have to do with agp?
	 */
	for (unit = 0; unit < dev->agp_nmaps; unit++) {
		struct drm_bus_map *const bm = &dev->agp_maps[unit];

		/* Is this one allocated? */
		if (bm->bm_mapped > 0) {
			/*
			 * Make sure it has the same base.
			 *
			 * XXX Why must it be the same base?  Can't we
			 * subregion here too?
			 */
			if (bm->bm_base != map->offset)
				continue;

			/* Make sure it's big enough.  */
			if (bm->bm_size < map->size)
				continue;

			/* Mark it used and return it.  */
			bm->bm_mapped++;

			/* XXX size is an input/output parameter too...?  */
			map->size = bm->bm_size;

			map->lm_data.bus_space.bsh = bm->bm_bsh;
			map->lm_data.bus_space.bus_map = bm;
			goto win;
		} else {
			const int flags = BUS_SPACE_MAP_PREFETCHABLE |
			    BUS_SPACE_MAP_LINEAR;

			/* Try mapping the request.  */
			error = bus_space_map(bst, map->offset, map->size,
			    flags, &bm->bm_bsh);
			if (error)
				return NULL; /* XXX Why not continue?  */

			/* Got it.  Allocate this bus map.  */
			bm->bm_mapped++;
			bm->bm_base = map->offset;
			bm->bm_size = map->size;
			bm->bm_flags = flags; /* XXX What for?  */

			map->lm_data.bus_space.bsh = bm->bm_bsh;
			map->lm_data.bus_space.bus_map = bm;
			goto win;
		}
	}

	return NULL;

win:
	map->lm_data.bus_space.bst = bst;
	return bus_space_vaddr(bst, map->lm_data.bus_space.bsh);
}

void
drm_iounmap(struct drm_device *dev, struct drm_local_map *map)
{
	const bus_space_tag_t bst = dev->bst;
	struct drm_bus_map *const bm = map->lm_data.bus_space.bus_map;

	/*
	 * bm may be null if we have committed the horrible deed of
	 * borrowing from agp_i810 or genfb.
	 */
	if (bm != NULL) {
		KASSERT(bm->bm_mapped > 0);
		if (--bm->bm_mapped)
			bus_space_unmap(bst, bm->bm_bsh, bm->bm_size);
	}
}

/*
 * Allocate a drm dma handle, allocate memory fit for DMA, and map it.
 *
 * XXX This is called drm_pci_alloc for hysterical raisins; it is not
 * specific to PCI.
 *
 * XXX For now, we use non-blocking allocations because this is called
 * by ioctls with the drm global mutex held.
 *
 * XXX Error information is lost because this returns NULL on failure,
 * not even an error embedded in a pointer.
 */
struct drm_dma_handle *
drm_pci_alloc(struct drm_device *dev, size_t size, size_t align)
{
	int nsegs;
	int error;

	/*
	 * Allocate a drm_dma_handle record.
	 */
	struct drm_dma_handle *const dmah = kmem_alloc(sizeof(*dmah),
	    KM_NOSLEEP);
	if (dmah == NULL) {
		error = -ENOMEM;
		goto out;
	}
	dmah->dmah_tag = dev->dmat;

	/*
	 * Allocate the requested amount of DMA-safe memory.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamem_alloc(dmah->dmah_tag, size, align, 0,
	    &dmah->dmah_seg, 1, &nsegs, BUS_DMA_NOWAIT);
	if (error)
		goto fail0;
	KASSERT(nsegs == 1);

	/*
	 * XXX Old drm passed BUS_DMA_NOWAIT below but BUS_DMA_WAITOK
	 * above.  WTF?
	 */

	/*
	 * Map the DMA-safe memory into kernel virtual address space.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamem_map(dmah->dmah_tag, &dmah->dmah_seg, 1, size,
	    &dmah->vaddr,
	    (BUS_DMA_NOWAIT | BUS_DMA_COHERENT | BUS_DMA_NOCACHE));
	if (error)
		goto fail1;
	dmah->size = size;

	/*
	 * Create a map for DMA transfers.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamap_create(dmah->dmah_tag, size, 1, size, 0,
	    BUS_DMA_NOWAIT, &dmah->dmah_map);
	if (error)
		goto fail2;

	/*
	 * Load the kva buffer into the map for DMA transfers.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamap_load(dmah->dmah_tag, dmah->dmah_map, dmah->vaddr,
	    size, NULL, (BUS_DMA_NOWAIT | BUS_DMA_NOCACHE));
	if (error)
		goto fail3;

	/* Record the bus address for convenient reference.  */
	dmah->busaddr = dmah->dmah_map->dm_segs[0].ds_addr;

	/* Zero the DMA buffer.  XXX Yikes!  Is this necessary?  */
	memset(dmah->vaddr, 0, size);

	/* Success!  */
	return dmah;

fail3:	bus_dmamap_destroy(dmah->dmah_tag, dmah->dmah_map);
fail2:	bus_dmamem_unmap(dmah->dmah_tag, dmah->vaddr, dmah->size);
fail1:	bus_dmamem_free(dmah->dmah_tag, &dmah->dmah_seg, 1);
fail0:	dmah->dmah_tag = NULL;	/* XXX paranoia */
	kmem_free(dmah, sizeof(*dmah));
out:	DRM_DEBUG("drm_pci_alloc failed: %d\n", error);
	return NULL;
}

/*
 * Release the bus DMA mappings and memory in dmah, and deallocate it.
 */
void
drm_pci_free(struct drm_device *dev, struct drm_dma_handle *dmah)
{

	bus_dmamap_unload(dmah->dmah_tag, dmah->dmah_map);
	bus_dmamap_destroy(dmah->dmah_tag, dmah->dmah_map);
	bus_dmamem_unmap(dmah->dmah_tag, dmah->vaddr, dmah->size);
	bus_dmamem_free(dmah->dmah_tag, &dmah->dmah_seg, 1);
	dmah->dmah_tag = NULL;	/* XXX paranoia */
	kmem_free(dmah, sizeof(*dmah));
}
