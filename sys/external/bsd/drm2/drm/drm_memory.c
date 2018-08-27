/*	$NetBSD: drm_memory.c,v 1.12 2018/08/27 06:58:20 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_memory.c,v 1.12 2018/08/27 06:58:20 riastradh Exp $");

#if defined(__i386__) || defined(__x86_64__)

# ifdef _KERNEL_OPT
#  include "agp.h"
#  if NAGP > 0
#   include "agp_i810.h"
#  else
#   define NAGP_I810	0
#  endif
#  include "genfb.h"
# else
#  define NAGP_I810	1
#  define NGENFB	0
# endif

#else

# ifdef _KERNEL_OPT
#  define NAGP_I810	0
#  include "genfb.h"
# else
#  define NAGP_I810	0
#  define NGENFB	0
# endif

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
#include <drm/drm_legacy.h>

/*
 * XXX drm_bus_borrow is a horrible kludge!
 */
static bool
drm_bus_borrow(bus_addr_t base, bus_size_t size, bus_space_handle_t *handlep)
{

#if NAGP_I810 > 0
	if (agp_i810_borrow(base, size, handlep))
		return true;
#endif

#if NGENFB > 0
	if (genfb_borrow(base, handlep))
		return true;
#endif

	return false;
}

void
drm_legacy_ioremap(struct drm_local_map *map, struct drm_device *dev)
{
	const bus_space_tag_t bst = dev->bst;
	unsigned int unit;

	/*
	 * Search dev's bus maps for a match.
	 */
	for (unit = 0; unit < dev->bus_nmaps; unit++) {
		struct drm_bus_map *const bm = &dev->bus_maps[unit];
		int flags = bm->bm_flags;

		/* Reject maps starting after the request.  */
		if (map->offset < bm->bm_base)
			continue;

		/* Reject maps smaller than the request.  */
		if (bm->bm_size < map->size)
			continue;

		/* Reject maps that the request doesn't fit in.  */
		if ((bm->bm_size - map->size) <
		    (map->offset - bm->bm_base))
			continue;

		/* Ensure we can map the space into virtual memory.  */
		if (!ISSET(flags, BUS_SPACE_MAP_LINEAR))
			continue;

		/* Reflect requested flags in the bus_space map.  */
		if (ISSET(map->flags, _DRM_WRITE_COMBINING))
			flags |= BUS_SPACE_MAP_PREFETCHABLE;

		/* Map it.  */
		if (bus_space_map(bst, map->offset, map->size, flags,
			&map->lm_data.bus_space.bsh))
			break;

		map->lm_data.bus_space.bus_map = bm;
		goto win;
	}

	/* Couldn't map it.  Try borrowing from someone else.  */
	if (drm_bus_borrow(map->offset, map->size,
		&map->lm_data.bus_space.bsh)) {
		map->lm_data.bus_space.bus_map = NULL;
		goto win;
	}

	/* Failure!  */
	return;

win:	map->lm_data.bus_space.bst = bst;
	map->handle = bus_space_vaddr(bst, map->lm_data.bus_space.bsh);
}

void
drm_legacy_ioremapfree(struct drm_local_map *map, struct drm_device *dev)
{
	if (map->lm_data.bus_space.bus_map != NULL) {
		bus_space_unmap(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, map->size);
		map->lm_data.bus_space.bus_map = NULL;
		map->handle = NULL;
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

/*
 * Make sure the DMA-safe memory allocated for dev lies between
 * min_addr and max_addr.  Can be used multiple times to restrict the
 * bounds further, but never to expand the bounds again.
 *
 * XXX Caller must guarantee nobody has used the tag yet,
 * i.e. allocated any DMA memory.
 */
int
drm_limit_dma_space(struct drm_device *dev, resource_size_t min_addr,
    resource_size_t max_addr)
{
	int ret;

	KASSERT(min_addr <= max_addr);

	/*
	 * Limit it further if we have already limited it, and destroy
	 * the old subregion DMA tag.
	 */
	if (dev->dmat_subregion_p) {
		min_addr = MAX(min_addr, dev->dmat_subregion_min);
		max_addr = MIN(max_addr, dev->dmat_subregion_max);
		bus_dmatag_destroy(dev->dmat);
	}

	/*
	 * Create a DMA tag for a subregion from the bus's DMA tag.  If
	 * that fails, restore dev->dmat to the whole region so that we
	 * need not worry about dev->dmat being uninitialized (not that
	 * the caller should try to allocate DMA-safe memory on failure
	 * anyway, but...paranoia).
	 */
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmatag_subregion(dev->bus_dmat, min_addr, max_addr,
	    &dev->dmat, BUS_DMA_WAITOK);
	if (ret) {
		dev->dmat = dev->bus_dmat;
		dev->dmat_subregion_p = false;
		return ret;
	}

	/*
	 * Remember that we have a subregion tag so that we know to
	 * destroy it later, and record the bounds in case we need to
	 * limit them again.
	 */
	dev->dmat_subregion_p = true;
	dev->dmat_subregion_min = min_addr;
	dev->dmat_subregion_max = max_addr;

	/* Success!  */
	return 0;
}
