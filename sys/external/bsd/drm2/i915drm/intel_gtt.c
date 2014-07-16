/*	$NetBSD: intel_gtt.c,v 1.4 2014/07/16 20:56:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

/* Intel GTT stubs */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_gtt.c,v 1.4 2014/07/16 20:56:25 riastradh Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <machine/vmparam.h>

#include <dev/pci/pcivar.h>		/* XXX agpvar.h needs...  */
#include <dev/pci/agpvar.h>
#include <dev/pci/agp_i810var.h>

#include "drm/intel-gtt.h"

/* Access to this should be single-threaded.  */
static struct {
	bus_dma_segment_t	scratch_seg;
	bus_dmamap_t		scratch_map;
} intel_gtt;

void
intel_gtt_get(size_t *va_size, size_t *stolen_size, bus_addr_t *aper_base,
    unsigned long *aper_size)
{
	struct agp_softc *const sc = agp_i810_sc;

	if (sc == NULL) {
		*va_size = 0;
		*stolen_size = 0;
		*aper_base = 0;
		*aper_size = 0;
		return;
	}

	struct agp_i810_softc *const isc = sc->as_chipc;
	*va_size = ((size_t)(isc->gtt_size/sizeof(uint32_t)) << PAGE_SHIFT);
	*stolen_size = ((size_t)isc->stolen << PAGE_SHIFT);
	*aper_base = sc->as_apaddr;
	*aper_size = sc->as_apsize;
}

int
intel_gmch_probe(struct pci_dev *bridge_pci __unused,
    struct pci_dev *gpu __unused, struct agp_bridge_data *bridge_agp __unused)
{
	struct agp_softc *const sc = agp_i810_sc;
	int nsegs;
	int error;

	if (sc == NULL)
		return 0;

	error = bus_dmamem_alloc(sc->as_dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &intel_gtt.scratch_seg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		goto fail0;
	KASSERT(nsegs == 1);

	error = bus_dmamap_create(sc->as_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &intel_gtt.scratch_map);
	if (error)
		goto fail1;

	error = bus_dmamap_load_raw(sc->as_dmat, intel_gtt.scratch_map,
	    &intel_gtt.scratch_seg, 1, PAGE_SIZE, BUS_DMA_WAITOK);
	if (error)
		goto fail2;

	/* Success!  */
	return 1;

fail3: __unused
	bus_dmamap_unload(sc->as_dmat, intel_gtt.scratch_map);
fail2:	bus_dmamap_destroy(sc->as_dmat, intel_gtt.scratch_map);
fail1:	bus_dmamem_free(sc->as_dmat, &intel_gtt.scratch_seg, 1);
fail0:	KASSERT(error);
	return 0;
}

void
intel_gmch_remove(void)
{
	struct agp_softc *const sc = agp_i810_sc;

	bus_dmamap_unload(sc->as_dmat, intel_gtt.scratch_map);
	bus_dmamap_destroy(sc->as_dmat, intel_gtt.scratch_map);
	bus_dmamem_free(sc->as_dmat, &intel_gtt.scratch_seg, 1);
}

bool
intel_enable_gtt(void)
{

	return (agp_i810_sc != NULL);
}

void
intel_gtt_chipset_flush(void)
{

	KASSERT(agp_i810_sc != NULL);
	agp_i810_chipset_flush(agp_i810_sc->as_chipc);
}

void
intel_gtt_insert_entries(bus_dmamap_t dmamap, unsigned va_page, unsigned flags)
{
	struct agp_i810_softc *const isc = agp_i810_sc->as_chipc;
	off_t va = (va_page << PAGE_SHIFT);
	unsigned seg;
	int error;

	KASSERT(0 <= va);
	KASSERT((va >> PAGE_SHIFT) == va_page);
	KASSERT(0 < dmamap->dm_nsegs);

	for (seg = 0; seg < dmamap->dm_nsegs; seg++) {
		const bus_addr_t addr = dmamap->dm_segs[seg].ds_addr;

		KASSERT(dmamap->dm_segs[seg].ds_len == PAGE_SIZE);

		/* XXX Respect flags.  */
		error = agp_i810_write_gtt_entry(isc, va, (addr | 1));
		if (error)
			device_printf(agp_i810_sc->as_dev,
			    "write gtt entry"
			    " %"PRIxMAX" -> %"PRIxMAX" failed: %d\n",
			    (uintmax_t)va, (uintmax_t)(addr | 1), error);
		va += PAGE_SIZE;
	}
	agp_i810_post_gtt_entry(isc, (va - PAGE_SIZE));
}

void
intel_gtt_clear_range(unsigned va_page, unsigned npages)
{
	struct agp_i810_softc *const isc = agp_i810_sc->as_chipc;
	const bus_addr_t addr = intel_gtt.scratch_map->dm_segs[0].ds_addr;
	off_t va = (va_page << PAGE_SHIFT);

	KASSERT(0 <= va);
	KASSERT((va >> PAGE_SHIFT) == va_page);
	KASSERT(0 < npages);

	while (npages--) {
		agp_i810_write_gtt_entry(isc, va, (addr | 1));
		va += PAGE_SIZE;
	}
	agp_i810_post_gtt_entry(isc, va - PAGE_SIZE);
}
