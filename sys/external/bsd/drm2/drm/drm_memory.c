/*	$NetBSD: drm_memory.c,v 1.1.2.1 2013/07/24 02:23:06 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_memory.c,v 1.1.2.1 2013/07/24 02:23:06 riastradh Exp $");

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
				if (drm_bus_borrow(map->offset, &map->bsh)) {
					map->bus_map = NULL;
					goto win;
				}
				return NULL;
			}
		}

		/* Mark it used and make a subregion just for the request.  */
		bm->bm_mapped++;
		error = bus_space_subregion(bst, bm->bm_bsh,
		    map->offset - bm->bm_base, map->size, &map->bsh);
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
		map->bus_map = bm;
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

			map->bsh = bm->bm_bsh;
			map->bus_map = bm;
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

			map->bsh = bm->bm_bsh;
			map->bus_map = bm;
			goto win;
		}
	}

	return NULL;

win:
	return bus_space_vaddr(bst, map->bsh);
}

void
drm_iounmap(struct drm_device *dev, struct drm_local_map *map)
{
	const bus_space_tag_t bst = dev->bst;
	struct drm_bus_map *const bm = map->bus_map;

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
