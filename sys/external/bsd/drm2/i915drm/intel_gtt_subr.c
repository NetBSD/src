/*	$NetBSD: intel_gtt_subr.c,v 1.3 2021/12/19 12:37:36 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intel_gtt_subr.c,v 1.3 2021/12/19 12:37:36 riastradh Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <machine/vmparam.h>

#include <dev/pci/pcivar.h>		/* XXX agpvar.h needs...  */
#include <dev/pci/agpvar.h>
#include <dev/pci/agp_i810var.h>

#include <linux/pci.h>
#include <linux/scatterlist.h>

#include "drm/i915_drm.h"
#include "drm/intel-gtt.h"

static uint8_t
pci_conf_read8(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t reg)
{
	uint32_t v;

	v = pci_conf_read(pc, tag, reg & ~3);

	return 0xff & (v >> (8 * (reg & 3)));
}

static uint8_t
pci_read8(pci_chipset_tag_t pc, int bus, int dev, int func, bus_size_t reg)
{
	pcitag_t tag = pci_make_tag(pc, bus, dev, func);

	return pci_conf_read8(pc, tag, reg);
}

static uint16_t
pci_conf_read16(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t reg)
{
	uint32_t v;

	KASSERT((reg & 1) == 0);

	v = pci_conf_read(pc, tag, reg & ~2);

	return 0xffff & (v >> (8 * (reg & 2)));
}

static uint16_t
pci_read16(pci_chipset_tag_t pc, int bus, int dev, int func, bus_size_t reg)
{
	pcitag_t tag = pci_make_tag(pc, bus, dev, func);

	return pci_conf_read16(pc, tag, reg);
}

/* Access to this should be single-threaded.  */
static struct {
	bus_dma_segment_t	scratch_seg;
	bus_dmamap_t		scratch_map;
} intel_gtt;

/* XXX This logic should be merged with agp_i810.c.  */
struct resource intel_graphics_stolen_res;

static bus_size_t
i830_tseg_size(pci_chipset_tag_t pc)
{
	uint8_t esmramc = pci_read8(pc, 0, 0, 0, I830_ESMRAMC);

	if ((esmramc & TSEG_ENABLE) == 0)
		return 0;

	return (esmramc & I830_TSEG_SIZE_1M) ? 1024*1024 : 512*1024;
}

static bus_size_t
i845_tseg_size(pci_chipset_tag_t pc)
{
	uint8_t esmramc = pci_read8(pc, 0, 0, 0, I845_ESMRAMC);

	if ((esmramc & TSEG_ENABLE) == 0)
		return 0;

	switch (esmramc & I845_TSEG_SIZE_MASK) {
	case I845_TSEG_SIZE_512K:
		return 512*1024;
	case I845_TSEG_SIZE_1M:
		return 1024*1024;
	default:
		return 0;
	}
}

static bus_size_t
i85x_tseg_size(pci_chipset_tag_t pc)
{
	uint8_t esmramc = pci_read8(pc, 0, 0, 0, I85X_ESMRAMC);

	if ((esmramc & TSEG_ENABLE) == 0)
		return 0;

	return 1024*1024;
}

static bus_size_t
i830_tom(pci_chipset_tag_t pc)
{
	uint8_t drb3 = pci_read8(pc, 0, 0, 0, I830_DRB3);

	return (bus_size_t)32*1024*1024 * drb3;
}

static bus_size_t
i85x_tom(pci_chipset_tag_t pc)
{
	uint8_t drb3 = pci_read8(pc, 0, 0, 1, I85X_DRB3);

	return (bus_size_t)32*1024*1024 * drb3;
}

static bus_size_t
i830_stolen_size(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint16_t gmch_ctrl = pci_read16(pc, 0, 0, 0, I830_GMCH_CTRL);

	switch (gmch_ctrl & I830_GMCH_GMS_MASK) {
	case I830_GMCH_GMS_STOLEN_512:
		return 512*1024;
	case I830_GMCH_GMS_STOLEN_1024:
		return 1024*1024;
	case I830_GMCH_GMS_STOLEN_8192:
		return 8*1024*1024;
	case I830_GMCH_GMS_LOCAL:
	default:
		aprint_error("%s: invalid gmch_ctrl 0x%04x\n", __func__,
		    gmch_ctrl);
		return 0;
	}
}

static bus_size_t
gen3_stolen_size(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint16_t gmch_ctrl = pci_read16(pc, 0, 0, 0, I830_GMCH_CTRL);

	switch (gmch_ctrl & I855_GMCH_GMS_MASK) {
	case I855_GMCH_GMS_STOLEN_1M:
		return 1024*1024;
	case I855_GMCH_GMS_STOLEN_4M:
		return 4*1024*1024;
	case I855_GMCH_GMS_STOLEN_8M:
		return 8*1024*1024;
	case I855_GMCH_GMS_STOLEN_16M:
		return 16*1024*1024;
	case I855_GMCH_GMS_STOLEN_32M:
		return 32*1024*1024;
	case I915_GMCH_GMS_STOLEN_48M:
		return 48*1024*1024;
	case I915_GMCH_GMS_STOLEN_64M:
		return 64*1024*1024;
	case G33_GMCH_GMS_STOLEN_128M:
		return 128*1024*1024;
	case G33_GMCH_GMS_STOLEN_256M:
		return 256*1024*1024;
	case INTEL_GMCH_GMS_STOLEN_96M:
		return 96*1024*1024;
	case INTEL_GMCH_GMS_STOLEN_160M:
		return 160*1024*1024;
	case INTEL_GMCH_GMS_STOLEN_224M:
		return 224*1024*1024;
	case INTEL_GMCH_GMS_STOLEN_352M:
		return 352*1024*1024;
	default:
		aprint_error("%s: invalid gmch_ctrl 0x%04x\n", __func__,
		    gmch_ctrl);
		return 0;
	}
}

static bus_size_t
gen6_stolen_size(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint16_t gmch_ctrl = pci_conf_read16(pc, tag, SNB_GMCH_CTRL);
	uint16_t gms = (gmch_ctrl >> SNB_GMCH_GMS_SHIFT) & SNB_GMCH_GMS_MASK;

	return (bus_size_t)32*1024*1024 * gms;
}

static bus_size_t
gen8_stolen_size(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint16_t gmch_ctrl = pci_conf_read16(pc, tag, SNB_GMCH_CTRL);
	uint16_t gms = (gmch_ctrl >> BDW_GMCH_GMS_SHIFT) & BDW_GMCH_GMS_MASK;

	return (bus_size_t)32*1024*1024 * gms;
}

static bus_size_t
chv_stolen_size(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint16_t gmch_ctrl = pci_conf_read16(pc, tag, SNB_GMCH_CTRL);
	uint16_t gms = (gmch_ctrl >> SNB_GMCH_GMS_SHIFT) & SNB_GMCH_GMS_MASK;

	if (gms <= 0x10)
		return (bus_size_t)32*1024*1024 * gms;
	else if (gms <= 0x16)
		return (bus_size_t)(8 + 4*(gms - 0x11))*1024*1024;
	else
		return (bus_size_t)(36 + 4*(gms - 0x17))*1024*1024;
}

static bus_size_t
gen9_stolen_size(pci_chipset_tag_t pc, pcitag_t tag)
{
	uint16_t gmch_ctrl = pci_conf_read16(pc, tag, SNB_GMCH_CTRL);
	uint16_t gms = (gmch_ctrl >> BDW_GMCH_GMS_SHIFT) & BDW_GMCH_GMS_MASK;

	if (gms <= 0xef)
		return (bus_size_t)32*1024*1024 * gms;
	else
		return (bus_size_t)(4 + 4*(gms - 0xf0))*1024*1024;
}

static bus_addr_t
i830_stolen_base(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t stolen_size)
{

	return i830_tom(pc) - i830_tseg_size(pc) - stolen_size;
}

static bus_addr_t
i845_stolen_base(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t stolen_size)
{

	return i830_tom(pc) - i845_tseg_size(pc) - stolen_size;
}

static bus_addr_t
i85x_stolen_base(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t stolen_size)
{

	return i85x_tom(pc) - i85x_tseg_size(pc) - stolen_size;
}

static bus_addr_t
i865_stolen_base(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t stolen_size)
{
	uint16_t toud = pci_read16(pc, 0, 0, 0, I865_TOUD);

	return i845_tseg_size(pc) + (64*1024 * toud);
}

static bus_addr_t
gen3_stolen_base(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t stolen_size)
{
	uint32_t bsm = pci_conf_read(pc, tag, INTEL_BSM);

	return bsm & INTEL_BSM_MASK;
}

static bus_addr_t
gen11_stolen_base(pci_chipset_tag_t pc, pcitag_t tag, bus_size_t stolen_size)
{
	uint32_t bsm;

	bsm = pci_conf_read(pc, tag, INTEL_GEN11_BSM_DW0) & INTEL_BSM_MASK;
	bsm |= (uint64_t)pci_conf_read(pc, tag, INTEL_GEN11_BSM_DW1) << 32;

	return bsm;
}

struct intel_stolen_ops {
	bus_size_t	(*size)(pci_chipset_tag_t, pcitag_t);
	bus_addr_t	(*base)(pci_chipset_tag_t, pcitag_t, bus_size_t);
};

static const struct intel_stolen_ops i830_stolen_ops = {
	.size = i830_stolen_size,
	.base = i830_stolen_base,
};

static const struct intel_stolen_ops i845_stolen_ops = {
	.size = i830_stolen_size,
	.base = i845_stolen_base,
};

static const struct intel_stolen_ops i85x_stolen_ops = {
	.size = gen3_stolen_size,
	.base = i85x_stolen_base,
};

static const struct intel_stolen_ops i865_stolen_ops = {
	.size = gen3_stolen_size,
	.base = i865_stolen_base,
};

static const struct intel_stolen_ops gen3_stolen_ops = {
	.size = gen3_stolen_size,
	.base = gen3_stolen_base,
};

static const struct intel_stolen_ops gen6_stolen_ops = {
	.size = gen6_stolen_size,
	.base = gen3_stolen_base,
};

static const struct intel_stolen_ops gen8_stolen_ops = {
	.size = gen8_stolen_size,
	.base = gen3_stolen_base,
};

static const struct intel_stolen_ops gen9_stolen_ops = {
	.size = gen9_stolen_size,
	.base = gen3_stolen_base,
};

static const struct intel_stolen_ops chv_stolen_ops = {
	.size = chv_stolen_size,
	.base = gen3_stolen_base,
};

static const struct intel_stolen_ops gen11_stolen_ops = {
	.size = gen9_stolen_size,
	.base = gen11_stolen_base,
};

static const struct pci_device_id intel_stolen_ids[] = {
	INTEL_I830_IDS(&i830_stolen_ops),
	INTEL_I845G_IDS(&i845_stolen_ops),
	INTEL_I85X_IDS(&i85x_stolen_ops),
	INTEL_I865G_IDS(&i865_stolen_ops),
	INTEL_I915G_IDS(&gen3_stolen_ops),
	INTEL_I915GM_IDS(&gen3_stolen_ops),
	INTEL_I945G_IDS(&gen3_stolen_ops),
	INTEL_I945GM_IDS(&gen3_stolen_ops),
	INTEL_VLV_IDS(&gen6_stolen_ops),
	INTEL_PINEVIEW_G_IDS(&gen3_stolen_ops),
	INTEL_PINEVIEW_M_IDS(&gen3_stolen_ops),
	INTEL_I965G_IDS(&gen3_stolen_ops),
	INTEL_G33_IDS(&gen3_stolen_ops),
	INTEL_I965GM_IDS(&gen3_stolen_ops),
	INTEL_GM45_IDS(&gen3_stolen_ops),
	INTEL_G45_IDS(&gen3_stolen_ops),
	INTEL_IRONLAKE_D_IDS(&gen3_stolen_ops),
	INTEL_IRONLAKE_M_IDS(&gen3_stolen_ops),
	INTEL_SNB_D_IDS(&gen6_stolen_ops),
	INTEL_SNB_M_IDS(&gen6_stolen_ops),
	INTEL_IVB_M_IDS(&gen6_stolen_ops),
	INTEL_IVB_D_IDS(&gen6_stolen_ops),
	INTEL_HSW_IDS(&gen6_stolen_ops),
	INTEL_BDW_IDS(&gen8_stolen_ops),
	INTEL_CHV_IDS(&chv_stolen_ops),
	INTEL_SKL_IDS(&gen9_stolen_ops),
	INTEL_BXT_IDS(&gen9_stolen_ops),
	INTEL_KBL_IDS(&gen9_stolen_ops),
	INTEL_CFL_IDS(&gen9_stolen_ops),
	INTEL_GLK_IDS(&gen9_stolen_ops),
	INTEL_CNL_IDS(&gen9_stolen_ops),
	INTEL_ICL_11_IDS(&gen11_stolen_ops),
	INTEL_EHL_IDS(&gen11_stolen_ops),
	INTEL_TGL_12_IDS(&gen11_stolen_ops),
};

void
intel_gtt_get(uint64_t *va_size, bus_addr_t *aper_base,
    resource_size_t *aper_size)
{
	struct agp_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;
	struct agp_i810_softc *isc;
	const struct intel_stolen_ops *ops;
	bus_addr_t stolen_base;
	bus_size_t stolen_size;
	unsigned i;

	if ((sc = agp_i810_sc) == NULL) {
		*va_size = 0;
		*aper_base = 0;
		*aper_size = 0;
		return;
	}

	pc = sc->as_pc;
	tag = sc->as_tag;

	isc = sc->as_chipc;
	*va_size = ((size_t)(isc->gtt_size/sizeof(uint32_t)) << PAGE_SHIFT);
	*aper_base = sc->as_apaddr;
	*aper_size = sc->as_apsize;

	for (i = 0; i < __arraycount(intel_stolen_ids); i++) {
		if (intel_stolen_ids[i].device == PCI_PRODUCT(sc->as_id)) {
			ops = (const struct intel_stolen_ops *)
			    intel_stolen_ids[i].driver_data;
			stolen_size = (*ops->size)(pc, tag);
			stolen_base = (*ops->base)(pc, tag, stolen_size);
			intel_graphics_stolen_res.start = stolen_base;
			intel_graphics_stolen_res.end =
			    stolen_base + stolen_size - 1;
			break;
		}
	}
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
	struct agp_softc *sc = agp_i810_sc;
	struct agp_i810_softc *isc;

	if (sc == NULL)
		return false;
	isc = sc->as_chipc;
	agp_i810_reset(isc);
	return true;
}

void
intel_gtt_chipset_flush(void)
{

	KASSERT(agp_i810_sc != NULL);
	agp_i810_chipset_flush(agp_i810_sc->as_chipc);
}

static int
intel_gtt_flags(unsigned flags)
{
	int gtt_flags = AGP_I810_GTT_VALID;

	switch (flags) {
	case AGP_USER_MEMORY:
		break;
	case AGP_USER_CACHED_MEMORY:
		gtt_flags |= AGP_I810_GTT_CACHED;
		break;
	default:
		panic("invalid intel gtt flags: %x", flags);
	}

	return gtt_flags;
}

void
intel_gtt_insert_page(bus_addr_t addr, unsigned va_page, unsigned flags)
{
	struct agp_i810_softc *const isc = agp_i810_sc->as_chipc;
	off_t va = (off_t)va_page << PAGE_SHIFT;
	int gtt_flags = intel_gtt_flags(flags);
	int error;

	error = agp_i810_write_gtt_entry(isc, va, addr, gtt_flags);
	if (error)
		device_printf(agp_i810_sc->as_dev,
		    "write gtt entry"
		    " %"PRIxMAX" -> %"PRIxMAX" (flags=%x) failed: %d\n",
		    (uintmax_t)va, (uintmax_t)addr, flags,
		    error);
	agp_i810_post_gtt_entry(isc, va);
	intel_gtt_chipset_flush();
}

void
intel_gtt_insert_sg_entries(struct sg_table *sg, unsigned va_page,
    unsigned flags)
{
	bus_dmamap_t dmamap = sg->sgl[0].sg_dmamap;
	struct agp_i810_softc *const isc = agp_i810_sc->as_chipc;
	off_t va = (off_t)va_page << PAGE_SHIFT;
	unsigned seg;
	int gtt_flags = intel_gtt_flags(flags);
	int error;

	KASSERT(0 <= va);
	KASSERT((va >> PAGE_SHIFT) == va_page);
	KASSERT(0 < dmamap->dm_nsegs);

	for (seg = 0; seg < dmamap->dm_nsegs; seg++) {
		const bus_addr_t addr = dmamap->dm_segs[seg].ds_addr;
		bus_size_t len;

		for (len = dmamap->dm_segs[seg].ds_len;
		     len >= PAGE_SIZE;
		     len -= PAGE_SIZE, va += PAGE_SIZE) {
			error = agp_i810_write_gtt_entry(isc, va, addr,
			    gtt_flags);
			if (error)
				device_printf(agp_i810_sc->as_dev,
				    "write gtt entry"
				    " %"PRIxMAX" -> %"PRIxMAX" (flags=%x)"
				    " failed: %d\n",
				    (uintmax_t)va, (uintmax_t)addr, flags,
				    error);
		}
		KASSERTMSG(len == 0,
		    "segment length not divisible by PAGE_SIZE: %jx",
		    (uintmax_t)dmamap->dm_segs[seg].ds_len);
	}
	agp_i810_post_gtt_entry(isc, (va - PAGE_SIZE));
	intel_gtt_chipset_flush();
}

void
intel_gtt_clear_range(unsigned va_page, unsigned npages)
{
	struct agp_i810_softc *const isc = agp_i810_sc->as_chipc;
	const bus_addr_t addr = intel_gtt.scratch_map->dm_segs[0].ds_addr;
	const int gtt_flags = AGP_I810_GTT_VALID;
	off_t va = (va_page << PAGE_SHIFT);

	KASSERT(0 <= va);
	KASSERT((va >> PAGE_SHIFT) == va_page);
	KASSERT(0 < npages);

	while (npages--) {
		agp_i810_write_gtt_entry(isc, va, addr, gtt_flags);
		va += PAGE_SIZE;
	}
	agp_i810_post_gtt_entry(isc, va - PAGE_SIZE);
}
