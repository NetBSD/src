/*	$NetBSD: i915_gem_gtt.c,v 1.1.2.9 2014/03/05 14:45:00 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_gem_gtt.c,v 1.1.2.9 2014/03/05 14:45:00 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <dev/pci/agpvar.h>
#include <dev/pci/agp_i810var.h>

#include <drm/drmP.h>

#include "i915_drv.h"

static int	i915_gem_gtt_init_scratch_page(struct intel_gtt *,
		    bus_dma_tag_t);
static void	i915_gem_gtt_fini_scratch_page(struct intel_gtt *,
		    bus_dma_tag_t);

static void	i915_gtt_color_adjust(struct drm_mm_node *, unsigned long,
		    unsigned long *, unsigned long *);
static void	i915_ggtt_clear_range(struct drm_device *, unsigned, unsigned);

static int	agp_gtt_init(struct drm_device *);
static void	agp_gtt_fini(struct drm_device *);
static void	agp_ggtt_bind_object(struct drm_i915_gem_object *,
		    enum i915_cache_level);
static void	agp_ggtt_clear_range(struct drm_device *, unsigned, unsigned);

static int	gen6_gtt_init(struct drm_device *);
static void	gen6_gtt_fini(struct drm_device *);
static void	gen6_ggtt_bind_object(struct drm_i915_gem_object *,
		    enum i915_cache_level);
static void	gen6_ggtt_clear_range(struct drm_device *, unsigned, unsigned);

int
i915_gem_gtt_init(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct intel_gtt *gtt;
	int ret;

	dev_priv->mm.gtt = gtt = kmem_zalloc(sizeof(*gtt), KM_SLEEP);
	if (INTEL_INFO(dev)->gen < 6)
		ret = agp_gtt_init(dev);
	else
		ret = gen6_gtt_init(dev);
	if (ret)
		goto fail0;

	/* Success!  */
	DRM_INFO("Memory usable by graphics device = %dM\n",
	    gtt->gtt_total_entries >> 8);
	DRM_DEBUG_DRIVER("GMADR size = %dM\n", gtt->gtt_mappable_entries >> 8);
	DRM_DEBUG_DRIVER("GTT stolen size = %dM\n", gtt->stolen_size >> 20);
	return 0;

fail1: __unused
	if (INTEL_INFO(dev)->gen < 6)
		agp_gtt_fini(dev);
	else
		gen6_gtt_fini(dev);
fail0:	kmem_free(gtt, sizeof(*gtt));
	return ret;
}

void
i915_gem_gtt_fini(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;

	if (INTEL_INFO(dev)->gen < 6)
		agp_gtt_fini(dev);
	else
		gen6_gtt_fini(dev);
	kmem_free(dev_priv->mm.gtt, sizeof(*dev_priv->mm.gtt));
}

static int
i915_gem_gtt_init_scratch_page(struct intel_gtt *gtt, bus_dma_tag_t dmat)
{
	int nsegs;
	int ret;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &gtt->gtt_scratch_seg, 1, &nsegs, 0);
	if (ret)
		goto fail0;
	KASSERT(nsegs == 1);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_create(dmat, PAGE_SIZE, 1, PAGE_SIZE, 0, 0,
	    &gtt->gtt_scratch_map);
	if (ret)
		goto fail1;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load_raw(dmat, gtt->gtt_scratch_map,
	    &gtt->gtt_scratch_seg, 1, PAGE_SIZE, BUS_DMA_NOCACHE);
	if (ret)
		goto fail2;

	/* Success!  */
	return 0;

fail3: __unused
	bus_dmamap_unload(dmat, gtt->gtt_scratch_map);
fail2:	bus_dmamap_destroy(dmat, gtt->gtt_scratch_map);
fail1:	bus_dmamem_free(dmat, &gtt->gtt_scratch_seg, 1);
fail0:	return ret;
}

static void
i915_gem_gtt_fini_scratch_page(struct intel_gtt *gtt, bus_dma_tag_t dmat)
{

	bus_dmamap_unload(dmat, gtt->gtt_scratch_map);
	bus_dmamap_destroy(dmat, gtt->gtt_scratch_map);
	bus_dmamem_free(dmat, &gtt->gtt_scratch_seg, 1);
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
	struct drm_i915_gem_object *obj, *next;
	int ret;

	/* Empty the mm before taking it down.  */
	list_for_each_entry_safe(obj, next, &dev_priv->mm.unbound_list,
	    gtt_list) {
		ret = i915_gem_object_unbind(obj);
		if (ret)
			DRM_ERROR("Unable to unbind object %p: %d\n", obj,
			    ret);
	}

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
	struct drm_device *const dev = obj->base.dev;

	if (INTEL_INFO(dev)->gen < 6)
		agp_ggtt_bind_object(obj, cache_level);
	else
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

static bool
i915_gem_gtt_idle(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	const bool interruptible = dev_priv->mm.interruptible;

	if (__predict_false(dev_priv->mm.gtt->do_idle_maps)) {
		dev_priv->mm.interruptible = false;
		if (i915_gpu_idle(dev)) {
			DRM_ERROR("unable to idle GPU\n");
			/* XXX Cargo-culted from Linux.  */
			udelay(10);
		}
	}

	return interruptible;
}

static void
i915_gem_gtt_unidle(struct drm_device *dev, bool interruptible)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;

	dev_priv->mm.interruptible = interruptible;
}

void
i915_gem_gtt_finish_object(struct drm_i915_gem_object *obj)
{
	struct drm_device *const dev = obj->base.dev;
	bool interruptible;

	interruptible = i915_gem_gtt_idle(dev);
	if (!obj->has_dma_mapping)
		bus_dmamap_unload(dev->dmat, obj->igo_dmamap);
	i915_gem_gtt_unidle(dev, interruptible);
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

	if (INTEL_INFO(dev)->gen < 6)
		agp_ggtt_clear_range(dev, start_page, npages);
	else
		gen6_ggtt_clear_range(dev, start_page, npages);
}

/*
 * AGP GTT
 */

/*
 * XXX This is factored all wrong.  We should merge this gen6+ code
 * into our agp abstraction like FreeBSD did to clean it up.
 */

/*
 * XXX Kludge!  If you implement VT-d stuff, you should fix this.  I
 * hope you will find this comment, by grepping for the string `VT-d',
 * when you do.  This has to do with using VT-d to map graphics crap.
 * I don't understand it -- you will have to see what it means in Linux
 * to find out.
 */
int intel_iommu_gfx_mapped = 0;

static int
agp_gtt_init(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct intel_gtt *gtt = dev_priv->mm.gtt;
	struct agp_i810_softc *isc;
	uint16_t product;
	int ret;

	if (agp_i810_sc == NULL) {
		DRM_ERROR("no i810 agp device for gtt\n");
		ret = -ENODEV;
		goto fail0;
	}
	isc = agp_i810_sc->as_chipc;

	gtt->gma_bus_addr = agp_i810_sc->as_apaddr;
	gtt->gtt_mappable_entries = (agp_i810_sc->as_apsize >> AGP_PAGE_SHIFT);
	gtt->stolen_size = (isc->stolen >> AGP_PAGE_SHIFT);
	gtt->gtt_total_entries =
	    (gtt->gtt_mappable_entries + gtt->stolen_size);
	gtt->gtt_bsh = isc->bsh;

	product = PCI_PRODUCT(dev->pdev->pd_pa.pa_id);
	if (((product == PCI_PRODUCT_INTEL_IRONLAKE_M_HB) ||
	     (product == PCI_PRODUCT_INTEL_IRONLAKE_M_HB)) &&
	    intel_iommu_gfx_mapped)
		gtt->do_idle_maps = true;
	else
		gtt->do_idle_maps = false;

	ret = i915_gem_gtt_init_scratch_page(gtt, agp_i810_sc->as_dmat);
	if (ret)
		goto fail0;

	/* Success!  */
	return 0;

fail1: __unused
	i915_gem_gtt_fini_scratch_page(gtt, agp_i810_sc->as_dmat);
fail0:	return ret;
}

static void
agp_gtt_fini(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct intel_gtt *gtt = dev_priv->mm.gtt;

	i915_gem_gtt_fini_scratch_page(gtt, agp_i810_sc->as_dmat);
}

static void
agp_ggtt_bind_object(struct drm_i915_gem_object *obj,
    enum i915_cache_level cache_level)
{
	struct drm_device *const dev = obj->base.dev;
	struct drm_i915_private *const dev_priv = dev->dev_private;
	const off_t start = obj->gtt_space->start;
	bus_addr_t addr;
	bus_size_t len;
	unsigned int seg, i = 0;

	for (seg = 0; seg < obj->igo_dmamap->dm_nsegs; seg++) {
		addr = obj->igo_dmamap->dm_segs[seg].ds_addr;
		len = obj->igo_dmamap->dm_segs[seg].ds_len;
		do {
			KASSERT(PAGE_SIZE <= len);
			KASSERT(0 == (len % PAGE_SIZE));
			AGP_BIND_PAGE(agp_i810_sc, (start + (i << PAGE_SHIFT)),
			    addr);
			addr += PAGE_SIZE;
			len -= PAGE_SIZE;
			i += 1;
		} while (0 < len);
	}

	KASSERT((start >> PAGE_SHIFT) <= dev_priv->mm.gtt->gtt_total_entries);
	KASSERT(i <= (dev_priv->mm.gtt->gtt_total_entries -
		(start >> PAGE_SHIFT)));
	KASSERT(i == (obj->base.size >> PAGE_SHIFT));

	/*
	 * XXX Linux does a posting read here of the last PTE, but the
	 * AGP API provides no way to do that.  Hope it's OK...
	 */
}

static void
agp_ggtt_clear_range(struct drm_device *dev, unsigned start_page,
    unsigned npages)
{
	unsigned page;

	for (page = start_page; npages--; page++)
		AGP_UNBIND_PAGE(agp_i810_sc, (off_t)page << PAGE_SHIFT);

	/* XXX Posting read!  */
}

/*
 * Gen6+ GTT
 */

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
	KASSERT(addr <= __BITS(39, 0));
	return (addr | ((addr >> 28) & 0xff0));
}

static gtt_pte_t
gen6_pte_encode(struct drm_device *dev, bus_addr_t addr,
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

static int
gen6_gtt_init(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct pci_attach_args *const pa = &dev->pdev->pd_pa;
	struct intel_gtt *gtt = dev_priv->mm.gtt;
	uint16_t snb_gmch_ctl, ggms, gms;
	int ret;

	gtt->do_idle_maps = false;

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

	/* Linux sez:  For GEN6+ the PTEs for the ggtt live at 2MB + BAR0 */
	if (dev->bus_maps[0].bm_size < (gtt->gtt_total_entries *
		sizeof(gtt_pte_t))) {
		DRM_ERROR("BAR0 too small for GTT: 0x%"PRIxMAX" < 0x%"PRIxMAX
		    "\n",
		    dev->bus_maps[0].bm_size,
		    (gtt->gtt_total_entries * sizeof(gtt_pte_t)));
		ret = -ENODEV;
		goto fail0;
	}
	if (bus_space_map(dev->bst, (dev->bus_maps[0].bm_base + (2<<20)),
		(gtt->gtt_total_entries * sizeof(gtt_pte_t)),
		0,
		&gtt->gtt_bsh)) {
		DRM_ERROR("unable to map GTT\n");
		ret = -ENODEV;
		goto fail0;
	}

	ret = i915_gem_gtt_init_scratch_page(gtt, dev->dmat);
	if (ret)
		goto fail1;

	/* Success!  */
	return 0;

fail2: __unused
	i915_gem_gtt_fini_scratch_page(gtt, dev->dmat);
fail1:	bus_space_unmap(dev->bst, gtt->gtt_bsh,
	    (gtt->gtt_total_entries * sizeof(gtt_pte_t)));
fail0:	/* XXX Undo drm_limit_dma_space?  */
	return ret;
}

static void
gen6_gtt_fini(struct drm_device *dev)
{
	struct drm_i915_private *const dev_priv = dev->dev_private;
	struct intel_gtt *gtt = dev_priv->mm.gtt;

	bus_space_unmap(dev->bst, gtt->gtt_bsh,
	    (gtt->gtt_total_entries * sizeof(gtt_pte_t)));
	i915_gem_gtt_fini_scratch_page(gtt, dev->dmat);
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
			KASSERT(0 == (len % PAGE_SIZE));
			bus_space_write_4(bst, bsh, 4*(first_entry + i),
			    gen6_pte_encode(dev, addr, cache_level));
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
		const uint32_t expected = gen6_pte_encode(dev,
		    addr - PAGE_SIZE, cache_level);
		const uint32_t actual = bus_space_read_4(bst, bsh,
		    4*(first_entry + i-1));
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
	const gtt_pte_t scratch_pte = gen6_pte_encode(dev,
	    dev_priv->mm.gtt->gtt_scratch_map->dm_segs[0].ds_addr,
	    I915_CACHE_LLC);
	unsigned int i;

	for (i = 0; i < n; i++)
		bus_space_write_4(bst, bsh, 4*(start_page + i), scratch_pte);
	bus_space_read_4(bst, bsh, 4*start_page);
}
