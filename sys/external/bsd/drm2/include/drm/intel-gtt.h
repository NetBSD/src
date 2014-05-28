/*	$NetBSD: intel-gtt.h,v 1.3 2014/05/28 16:13:02 riastradh Exp $	*/

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

#ifndef _DRM_INTEL_GTT_H_
#define _DRM_INTEL_GTT_H_

#include <sys/bus.h>

#include "drm/bus_dma_hacks.h"

#include <linux/pci.h>

#include <drm/drm_agp_netbsd.h>

struct intel_gtt {
	/*
	 * GMADR, graphics memory address, a.k.a. the `aperture'.
	 * Access to bus addresses in the region starting here are
	 * remapped to physical system memory addresses programmed into
	 * the GTT (or GPU-local memory, for i810 chipsets, depending
	 * on the GTT entries).  This corresponds to a prefix of the
	 * GPU's virtual address space.  The virtual address space may
	 * be larger: in that case, there will be more GTT entries than
	 * pages in the aperture.
	 */
	paddr_t			gma_bus_addr;

	/*
	 * Number of bytes of system memory stolen by the graphics
	 * device for frame buffer memory (but not for the GTT).  These
	 * pages in memory -- if you know where they are -- can't be
	 * used by the CPU, but they can be programmed into the GTT for
	 * access from the GPU.
	 */
	unsigned int		stolen_size;

	/*
	 * Total number of GTT entries, including entries for the GPU's
	 * virtual address space beyond the aperture.
	 */
	unsigned int		gtt_total_entries;

	/*
	 * Number of GTT entries for pages that we can actually map
	 * into the aperture.
	 */
	unsigned int		gtt_mappable_entries;

	/* Scratch page for unbound GTT entries.  */
	bus_dma_segment_t	gtt_scratch_seg;
	bus_dmamap_t		gtt_scratch_map;

	/* Bus space handle for the GTT itself.  */
	bus_space_handle_t	gtt_bsh;

	/* IOMMU-related quirk for certain chipsets.  */
	bool			do_idle_maps;
};

struct intel_gtt *
	intel_gtt_get(void);
int	intel_gmch_probe(struct pci_dev *, struct pci_dev *,
	    struct agp_bridge_data *);
void	intel_gmch_remove(void);
bool	intel_enable_gtt(void);
void	intel_gtt_chipset_flush(void);
#ifndef __NetBSD__
void	intel_gtt_insert_sg_entries(struct sg_table *, unsigned int,
	    unsigned int);
#endif
void	intel_gtt_clear_range(unsigned int, unsigned int);

#define	AGP_DCACHE_MEMORY	1
#define	AGP_PHYS_MEMORY		2

#define	AGP_USER_CACHED_MEMORY_GFDT	__BIT(3)

extern int	intel_iommu_gfx_mapped;

#endif  /* _DRM_INTEL_GTT_H_ */
