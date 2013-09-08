/*	$NetBSD: i915_gem_gtt.c,v 1.1.2.3 2013/09/08 16:00:22 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_gem_gtt.c,v 1.1.2.3 2013/09/08 16:00:22 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <drm/drmP.h>

#include "i915_drv.h"

static void	i915_gtt_color_adjust(struct drm_mm_node *, unsigned long,
		    unsigned long *, unsigned long *);
static void	i915_ggtt_clear_range(struct drm_device *, unsigned, unsigned);
static void	gen6_ggtt_bind_object(struct drm_i915_gem_object *,
		    enum i915_cache_level);
static void	gen6_ggtt_clear_range(struct drm_device *, unsigned, unsigned);

#define	SNB_GMCH_GGMS	(SNB_GMCH_GGMS_MASK << SNB_GMCH_GGMS_SHIFT)
#define	SNB_GMCH_GMS	(SNB_GMCH_GMS_MASK << SNB_GMCH_GMS_SHIFT)
#define	IVB_GMCH_GMS	(IVB_GMCH_GMS_MASK << IVB_GMCH_GMS_SHIFT)

typedef uint32_t gtt_pte_t;

#define	GEN6_PTE_VALID		__BIT(0)
#define	GEN6_PTE_UNCACHED	__BIT(1)
#define	HSW_PTE_UNCACHED	(0)
#define	GEN6_PTE_CACHE_LLC	__BIT(2)
#define	GEN6_PTE_CACHE_LLC_MLC	__BIT(3)

static uint32_t
gen6_pte_addr_encode(bus_addr_t addr)
{
	/* XXX KASSERT bounds?  Must be at most 36-bit, it seems.  */
	return (addr | ((addr >> 28) & 0xff0));
}

static gtt_pte_t
pte_encode(struct drm_device *dev, bus_addr_t addr,
    enum i915_cache_level level)
{
	uint32_t flags = GEN6_PTE_VALID;

	switch (level) {
	case I915_CACHE_LLC_MLC:
		flags |= (IS_HASWELL(dev)? GEN6_PTE_CACHE_LLC
		    : GEN6_PTE_CACHE_LLC_MLC);
		break;

	case I915_CACHE_LLC:
		flags |= GEN6_PTE_CACHE_LLC;
		break;

	case I915_CACHE_NONE:
		flags |= (IS_HASWELL(dev)? HSW_PTE_UNCACHED
		    : GEN6_PTE_UNCACHED);
		break;

	default:
		panic("invalid i915 GTT cache level: %d", (int)level);
		break;
	}

	return (gen6_pte_addr_encode(addr) | flags);
}

int
i915_gem_gtt_init(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct pci_attach_args *const pa = &dev->pdev->pd_pa;
	struct intel_gtt *gtt;
	uint16_t snb_gmch_ctl, ggms, gms;
	int nsegs;
	int ret;

	if (INTEL_INFO(dev)->gen < 6) {
		/* XXX gen<6 */
		DRM_ERROR("device is too old for drm2 for now!\n");
		return -ENODEV;
	}

	gtt = kmem_zalloc(sizeof(*gtt), KM_NOSLEEP);

	/* XXX pci_set_dma_mask?  pci_set_consistent_dma_mask?  */
	drm_limit_dma_space(dev, 0, 0x0000000fffffffffULL);

	gtt->gma_bus_addr = dev->bus_maps[2].bm_base;

	snb_gmch_ctl = pci_conf_read(pa->pa_pc, pa->pa_tag, SNB_GMCH_CTRL);

	/* GMS: Graphics Mode Select.  */
	if (INTEL_INFO(dev)->gen < 7) {
		gms = __SHIFTOUT(snb_gmch_ctl, SNB_GMCH_GMS);
		gtt->stolen_size = (gms << 25);
	} else {
		gms = __SHIFTOUT(snb_gmch_ctl, IVB_GMCH_GMS);
		static const unsigned sizes[] = {
			0, 0, 0, 0, 0, 32, 48, 64, 128, 256, 96, 160, 224, 352
		};
		gtt->stolen_size = sizes[gms] << 20;
	}

	/* GGMS: GTT Graphics Memory Size.  */
	ggms = __SHIFTOUT(snb_gmch_ctl, SNB_GMCH_GGMS);
	gtt->gtt_total_entries = (ggms << 20) / sizeof(gtt_pte_t);

	gtt->gtt_mappable_entries = (dev->bus_maps[2].bm_size >> PAGE_SHIFT);
	if (((gtt->gtt_mappable_entries >> 8) < 64) ||
	    (gtt->gtt_total_entries < gtt->gtt_mappable_entries)) {
		DRM_ERROR("unknown GMADR entries: %d\n",
		    gtt->gtt_mappable_entries);
		ret = -ENXIO;
		goto fail0;
	}

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dev->dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &gtt->gtt_scratch_seg, 1, &nsegs, 0);
	if (ret)
		goto fail0;
	KASSERT(nsegs == 1);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_create(dev->dmat, PAGE_SIZE, 1, PAGE_SIZE, 0, 0,
	    &gtt->gtt_scratch_map);
	if (ret)
		goto fail1;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load_raw(dev->dmat, gtt->gtt_scratch_map,
	    &gtt->gtt_scratch_seg, 1, PAGE_SIZE, BUS_DMA_NOCACHE);
	if (ret)
		goto fail2;

	/* Linux sez:  For GEN6+ the PTEs for the ggtt live at 2MB + BAR0 */
	if (dev->bus_maps[0].bm_size < (gtt->gtt_total_entries *
		sizeof(gtt_pte_t))) {
		DRM_ERROR("BAR0 too small for GTT: 0x%"PRIxMAX" < 0x%"PRIxMAX
		    "\n",
		    dev->bus_maps[0].bm_size,
		    (gtt->gtt_total_entries * sizeof(gtt_pte_t)));
		ret = -ENODEV;
		goto fail3;
	}
	if (bus_space_map(dev->bst, (dev->bus_maps[0].bm_base + (2<<20)),
		(gtt->gtt_total_entries * sizeof(gtt_pte_t)),
		0,
		&gtt->gtt_bsh)) {
		DRM_ERROR("unable to map GTT\n");
		ret = -ENODEV;
		goto fail3;
	}

	DRM_INFO("Memory usable by graphics device = %dM\n",
	    gtt->gtt_total_entries >> 8);
	DRM_DEBUG_DRIVER("GMADR size = %dM\n", gtt->gtt_mappable_entries >> 8);
	DRM_DEBUG_DRIVER("GTT stolen size = %dM\n", gtt->stolen_size >> 20);

	/* Success!  */
	dev_priv->mm.gtt = gtt;
	return 0;

fail3:	bus_dmamap_unload(dev->dmat, gtt->gtt_scratch_map);
fail2:	bus_dmamap_destroy(dev->dmat, gtt->gtt_scratch_map);
fail1:	bus_dmamem_free(dev->dmat, &gtt->gtt_scratch_seg, 1);
fail0:	kmem_free(gtt, sizeof(*gtt));
	return ret;
}

void
i915_gem_gtt_fini(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct intel_gtt *const gtt = dev_priv->mm.gtt;

	bus_space_unmap(dev->bst, gtt->gtt_bsh,
	    (gtt->gtt_total_entries * sizeof(gtt_pte_t)));
	bus_dmamap_unload(dev->dmat, gtt->gtt_scratch_map);
	bus_dmamap_destroy(dev->dmat, gtt->gtt_scratch_map);
	bus_dmamem_free(dev->dmat, &gtt->gtt_scratch_seg, 1);
	kmem_free(gtt, sizeof(*gtt));
}

void
i915_gem_init_global_gtt(struct drm_device *dev, unsigned long start,
    unsigned long mappable_end, unsigned long end)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;

	KASSERT(start <= end);
	KASSERT(start <= mappable_end);
	KASSERT(PAGE_SIZE <= (end - start));
	drm_mm_init(&dev_priv->mm.gtt_space, start, (end - start - PAGE_SIZE));
	if (!HAS_LLC(dev))
		dev_priv->mm.gtt_space.color_adjust = i915_gtt_color_adjust;

	dev_priv->mm.gtt_start = start;
	dev_priv->mm.gtt_mappable_end = mappable_end;
	dev_priv->mm.gtt_end = end;
	dev_priv->mm.gtt_total = (end - start);
	dev_priv->mm.mappable_gtt_total = (MIN(end, mappable_end) - start);

	i915_ggtt_clear_range(dev, (start >> PAGE_SHIFT),
	    ((end - start) >> PAGE_SHIFT));
}

void
i915_gem_fini_global_gtt(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;

	drm_mm_takedown(&dev_priv->mm.gtt_space);
}

static void
i915_gtt_color_adjust(struct drm_mm_node *node, unsigned long color,
    unsigned long *start, unsigned long *end)
{

	if (node->color != color)
		*start += PAGE_SIZE;
	if (list_empty(&node->node_list))
		return;
	node = list_entry(node->node_list.next, struct drm_mm_node, node_list);
	if (node->allocated && (node->color != color))
		*end -= PAGE_SIZE;
}

void
i915_gem_init_ppgtt(struct drm_device *dev __unused)
{
}

int
i915_gem_init_aliasing_ppgtt(struct drm_device *dev __unused)
{
	return -ENODEV;
}

void
i915_gem_cleanup_aliasing_ppgtt(struct drm_device *dev __unused)
{
}

void
i915_ppgtt_bind_object(struct i915_hw_ppgtt *ppgtt __unused,
    struct drm_i915_gem_object *obj __unused,
    enum i915_cache_level cache_level __unused)
{
	panic("%s: not implemented", __func__);
}

void
i915_ppgtt_unbind_object(struct i915_hw_ppgtt *ppgtt __unused,
    struct drm_i915_gem_object *obj __unused)
{
	panic("%s: not implemented", __func__);
}

int
i915_gem_gtt_prepare_object(struct drm_i915_gem_object *obj)
{
	struct drm_device *const dev = obj->base.dev;
	int error;

	if (obj->has_dma_mapping)
		return 0;

	/* XXX errno NetBSD->Linux */
	error = -bus_dmamap_load_raw(dev->dmat, obj->igo_dmamap, obj->pages,
	    obj->igo_nsegs, obj->base.size, BUS_DMA_NOWAIT);
	if (error)
		goto fail0;

	/* Success!  */
	return 0;

fail0:	return error;
}

void
i915_gem_gtt_bind_object(struct drm_i915_gem_object *obj,
    enum i915_cache_level cache_level)
{

	KASSERT(6 < INTEL_INFO(obj->base.dev)->gen); /* XXX gen<6 */
	gen6_ggtt_bind_object(obj, cache_level);
	obj->has_global_gtt_mapping = 1;
}

void
i915_gem_gtt_unbind_object(struct drm_i915_gem_object *obj)
{

	i915_ggtt_clear_range(obj->base.dev,
	    (obj->gtt_space->start >> PAGE_SHIFT),
	    (obj->base.size >> PAGE_SHIFT));
	obj->has_global_gtt_mapping = 0;
}

void
i915_gem_gtt_finish_object(struct drm_i915_gem_object *obj)
{
	struct drm_device *const dev = obj->base.dev;

	/* XXX Idle, for gen<6.  */

	if (obj->has_dma_mapping)
		return;

	bus_dmamap_unload(dev->dmat, obj->igo_dmamap);
}

void
i915_gem_restore_gtt_mappings(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;

	i915_ggtt_clear_range(dev, (dev_priv->mm.gtt_start >> PAGE_SHIFT),
	    ((dev_priv->mm.gtt_start - dev_priv->mm.gtt_end) >> PAGE_SHIFT));

	list_for_each_entry(obj, &dev_priv->mm.bound_list, gtt_list) {
		i915_gem_clflush_object(obj);
		i915_gem_gtt_bind_object(obj, obj->cache_level);
	}

	i915_gem_chipset_flush(dev);
}

static void
i915_ggtt_clear_range(struct drm_device *dev, unsigned start_page,
    unsigned npages)
{

	KASSERT(6 <= INTEL_INFO(dev)->gen);

	gen6_ggtt_clear_range(dev, start_page, npages);
}

static void
gen6_ggtt_bind_object(struct drm_i915_gem_object *obj,
    enum i915_cache_level cache_level)
{
	struct drm_device *const dev = obj->base.dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	const bus_space_tag_t bst = dev->bst;
	const bus_space_handle_t bsh = dev_priv->mm.gtt->gtt_bsh;
	const unsigned first_entry = obj->gtt_space->start >> PAGE_SHIFT;
	bus_addr_t addr;
	bus_size_t len;
	unsigned int seg, i = 0;

	for (seg = 0; seg < obj->igo_dmamap->dm_nsegs; seg++) {
		addr = obj->igo_dmamap->dm_segs[seg].ds_addr;
		len = obj->igo_dmamap->dm_segs[seg].ds_len;
		do {
			KASSERT(PAGE_SIZE <= len);
			bus_space_write_4(bst, bsh, 4*(first_entry + i),
			    pte_encode(dev, addr, cache_level));
			addr += PAGE_SIZE;
			len -= PAGE_SIZE;
			i += 1;
		} while (0 < len);
	}

	KASSERT(first_entry <= dev_priv->mm.gtt->gtt_total_entries);
	KASSERT(i <= (dev_priv->mm.gtt->gtt_total_entries - first_entry));
	KASSERT(i == (obj->base.size >> PAGE_SHIFT));

	/* XXX Why could i ever be zero?  */
	if (0 < i) {
		/* Posting read and sanity check.  */
		/* XXX Shouldn't there be a bus_space_sync?  */
		const uint32_t expected = pte_encode(dev, addr, cache_level);
		const uint32_t actual = bus_space_read_4(bst, bsh,
		    (first_entry + (4*(i-1))));
		if (actual != expected)
			aprint_error_dev(dev->dev, "mismatched PTE"
			    ": 0x%"PRIxMAX" != 0x%"PRIxMAX"\n",
			    (uintmax_t)actual, (uintmax_t)expected);
	}

	I915_WRITE(GFX_FLSH_CNTL_GEN6, GFX_FLSH_CNTL_EN);
	POSTING_READ(GFX_FLSH_CNTL_GEN6);
}

static void
gen6_ggtt_clear_range(struct drm_device *dev, unsigned start_page,
    unsigned npages)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	const bus_space_tag_t bst = dev->bst;
	const bus_space_handle_t bsh = dev_priv->mm.gtt->gtt_bsh;
	const unsigned n = (dev_priv->mm.gtt->gtt_total_entries - start_page);
	const gtt_pte_t scratch_pte = pte_encode(dev,
	    dev_priv->mm.gtt->gtt_scratch_map->dm_segs[0].ds_addr,
	    I915_CACHE_LLC);
	unsigned int i;

	for (i = 0; i < n; i++)
		bus_space_write_4(bst, bsh, 4*(start_page + i), scratch_pte);
	bus_space_read_4(bst, bsh, 4*start_page);
}
