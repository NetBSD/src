/*	$NetBSD: drm_iomap_netbsd.h,v 1.1 2018/08/27 05:34:49 riastradh Exp $	*/

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

#ifndef	_DRM_DRM_IOMAP_NETBSD_H_
#define	_DRM_DRM_IOMAP_NETBSD_H_

#include <sys/bus.h>
#include <sys/endian.h>

#include <drm/drm_legacy.h>

static inline bool
DRM_IS_BUS_SPACE(struct drm_local_map *map)
{
	switch (map->type) {
	case _DRM_FRAME_BUFFER:
		panic("I don't know how to access drm frame buffer memory!");

	case _DRM_REGISTERS:
		return true;

	case _DRM_SHM:
		panic("I don't know how to access drm shared memory!");

	case _DRM_AGP:
		panic("I don't know how to access drm agp memory!");

	case _DRM_SCATTER_GATHER:
		panic("I don't know how to access drm scatter-gather memory!");

	case _DRM_CONSISTENT:
		/*
		 * XXX Old drm uses bus space access for this, but
		 * consistent maps don't have bus space handles!  They
		 * do, however, have kernel virtual addresses in the
		 * map->handle, so maybe that's right.
		 */
#if 0
		return false;
#endif
		panic("I don't know how to access drm consistent memory!");

	default:
		panic("I don't know what kind of memory you mean!");
	}
}

static inline uint8_t
DRM_READ8(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map))
		return bus_space_read_1(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
	else
		return *(volatile uint8_t *)((vaddr_t)map->handle + offset);
}

static inline uint16_t
DRM_READ16(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map))
		return bus_space_read_2(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
	else
		return *(volatile uint16_t *)((vaddr_t)map->handle + offset);
}

static inline uint32_t
DRM_READ32(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map))
		return bus_space_read_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
	else
		return *(volatile uint32_t *)((vaddr_t)map->handle + offset);
}

static inline uint64_t
DRM_READ64(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map)) {
#if _LP64			/* XXX How to detect bus_space_read_8?  */
		return bus_space_read_8(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
#elif _BYTE_ORDER == _LITTLE_ENDIAN
		/* XXX Yes, this is sketchy.  */
		return bus_space_read_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset) |
		    ((uint64_t)bus_space_read_4(map->lm_data.bus_space.bst,
			map->lm_data.bus_space.bsh, (offset + 4)) << 32);
#else
		/* XXX Yes, this is sketchy.  */
		return bus_space_read_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, (offset + 4)) |
		    ((uint64_t)bus_space_read_4(map->lm_data.bus_space.bst,
			map->lm_data.bus_space.bsh, offset) << 32);
#endif
	} else {
		return *(volatile uint64_t *)((vaddr_t)map->handle + offset);
	}
}

static inline void
DRM_WRITE8(struct drm_local_map *map, bus_size_t offset, uint8_t value)
{
	if (DRM_IS_BUS_SPACE(map))
		bus_space_write_1(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
	else
		*(volatile uint8_t *)((vaddr_t)map->handle + offset) = value;
}

static inline void
DRM_WRITE16(struct drm_local_map *map, bus_size_t offset, uint16_t value)
{
	if (DRM_IS_BUS_SPACE(map))
		bus_space_write_2(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
	else
		*(volatile uint16_t *)((vaddr_t)map->handle + offset) = value;
}

static inline void
DRM_WRITE32(struct drm_local_map *map, bus_size_t offset, uint32_t value)
{
	if (DRM_IS_BUS_SPACE(map))
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
	else
		*(volatile uint32_t *)((vaddr_t)map->handle + offset) = value;
}

static inline void
DRM_WRITE64(struct drm_local_map *map, bus_size_t offset, uint64_t value)
{
	if (DRM_IS_BUS_SPACE(map)) {
#if _LP64			/* XXX How to detect bus_space_write_8?  */
		bus_space_write_8(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
#elif _BYTE_ORDER == _LITTLE_ENDIAN
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, (value & 0xffffffffU));
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, (offset + 4), (value >> 32));
#else
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, (value >> 32));
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, (offset + 4),
		    (value & 0xffffffffU));
#endif
	} else {
		*(volatile uint64_t *)((vaddr_t)map->handle + offset) = value;
	}
}

#endif	/* _DRM_DRM_IOMAP_NETBSD_H_ */
