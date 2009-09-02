/*-
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

/** @file ati_pcigart.c
 * Implementation of ATI's PCIGART, which provides an aperture in card virtual
 * address space with addresses remapped to system memory.
 */

#include "drmP.h"

#define ATI_PCIGART_PAGE_SIZE		4096	/* PCI GART page size */
#define ATI_PCIGART_PAGE_MASK		(~(ATI_PCIGART_PAGE_SIZE-1))

#define ATI_PCIE_WRITE 0x4
#define ATI_PCIE_READ 0x8

#if defined(__FreeBSD__)
static void
drm_ati_alloc_pcigart_table_cb(void *arg, bus_dma_segment_t *segs,
			       int nsegs, int error)
{
	struct drm_dma_handle *dmah = arg;

	if (error != 0)
		return;

	DRM_KASSERT(nsegs == 1,
	    ("drm_ati_alloc_pcigart_table_cb: bad dma segment count"));

	dmah->busaddr = segs[0].ds_addr;
}
#endif

static int
drm_ati_alloc_pcigart_table(struct drm_device *dev,
			    struct drm_ati_pcigart_info *gart_info)
{
	struct drm_dma_handle *dmah;
	int flags, ret;
#if defined(__NetBSD__)
	int nsegs;
#endif

	dmah = malloc(sizeof(struct drm_dma_handle), DRM_MEM_DMA,
	    M_ZERO | M_NOWAIT);
	if (dmah == NULL)
		return ENOMEM;

#if defined(__FreeBSD__)
	DRM_UNLOCK();
	ret = bus_dma_tag_create(NULL, PAGE_SIZE, 0, /* tag, align, boundary */
	    gart_info->table_mask, BUS_SPACE_MAXADDR, /* lowaddr, highaddr */
	    NULL, NULL, /* filtfunc, filtfuncargs */
	    gart_info->table_size, 1, /* maxsize, nsegs */
	    gart_info->table_size, /* maxsegsize */
	    BUS_DMA_ALLOCNOW, NULL, NULL, /* flags, lockfunc, lockfuncargs */
	    &dmah->tag);
	if (ret != 0) {
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}

	flags = BUS_DMA_NOWAIT | BUS_DMA_ZERO;
	if (gart_info->gart_reg_if == DRM_ATI_GART_IGP)
	    flags |= BUS_DMA_NOCACHE;
	
	ret = bus_dmamem_alloc(dmah->tag, &dmah->vaddr, flags, &dmah->map);
	if (ret != 0) {
		bus_dma_tag_destroy(dmah->tag);
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}
	DRM_LOCK();

	ret = bus_dmamap_load(dmah->tag, dmah->map, dmah->vaddr,
	    gart_info->table_size, drm_ati_alloc_pcigart_table_cb, dmah, 0);
	if (ret != 0) {
		bus_dmamem_free(dmah->tag, dmah->vaddr, dmah->map);
		bus_dma_tag_destroy(dmah->tag);
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}

#elif   defined(__NetBSD__)
	dmah->tag = dev->pa.pa_dmat;

	flags = BUS_DMA_NOWAIT;
	if (gart_info->gart_reg_if == DRM_ATI_GART_IGP)
		flags |= BUS_DMA_NOCACHE;

	ret = bus_dmamem_alloc(dmah->tag, gart_info->table_size, PAGE_SIZE,
	0, dmah->segs, 1, &nsegs, flags);
	if (ret != 0) {
		printf("drm: unable to allocate %zu bytes of DMA, error %d\n",
			(size_t)gart_info->table_size, ret);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}
	if (nsegs != 1) {
		printf("drm: bad segment count\n");
		bus_dmamem_free(dmah->tag, dmah->segs, 1);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}

	ret = bus_dmamem_map(dmah->tag, dmah->segs, nsegs,
			     gart_info->table_size, &dmah->vaddr,
			     flags | BUS_DMA_COHERENT);
	if (ret != 0) {
		printf("drm: Unable to map DMA, error %d\n", ret);
		bus_dmamem_free(dmah->tag, dmah->segs, 1);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}

	ret = bus_dmamap_create(dmah->tag, gart_info->table_size, 1,
				gart_info->table_size, 0,
				BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &dmah->map);
	if (ret != 0) {
		printf("drm: Unable to create DMA map, error %d\n", ret);
		bus_dmamem_unmap(dmah->tag, dmah->vaddr, gart_info->table_size);
		bus_dmamem_free(dmah->tag, dmah->segs, 1);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}

	ret = bus_dmamap_load(dmah->tag, dmah->map, dmah->vaddr,
			      gart_info->table_size, NULL, BUS_DMA_NOWAIT);
	if (ret != 0) {
		printf("drm: Unable to load DMA map, error %d\n", ret);
		bus_dmamap_destroy(dmah->tag, dmah->map);
		bus_dmamem_unmap(dmah->tag, dmah->vaddr, gart_info->table_size);
		bus_dmamem_free(dmah->tag, dmah->segs, 1);
		dmah->tag = NULL;
		free(dmah, DRM_MEM_DMA);
		return ENOMEM;
	}
	dmah->busaddr = dmah->map->dm_segs[0].ds_addr;
	dmah->size = gart_info->table_size;
	dmah->nsegs = 1;
#if 0
	/*
	 * Mirror here FreeBSD doing BUS_DMA_ZERO.
	 * But I see this same memset() is done in drm_ati_pcigart_init(),
	 * so maybe this is not needed.
	 */
	memset(dmah->vaddr, 0, gart_info->table_size);
#endif
#endif

	dev->sg->dmah = dmah;

	return 0;
}

static void
drm_ati_free_pcigart_table(struct drm_device *dev,
			   struct drm_ati_pcigart_info *gart_info)
{
	struct drm_dma_handle *dmah = dev->sg->dmah;

#if defined(__FreeBSD__)
	bus_dmamem_free(dmah->tag, dmah->vaddr, dmah->map);
	bus_dma_tag_destroy(dmah->tag);
#elif   defined(__NetBSD__)
	bus_dmamap_unload(dmah->tag, dmah->map);
	bus_dmamap_destroy(dmah->tag, dmah->map);
	bus_dmamem_unmap(dmah->tag, dmah->vaddr, dmah->size);
	bus_dmamem_free(dmah->tag, dmah->segs, 1);
	dmah->tag = NULL;
#endif
	free(dmah, DRM_MEM_DMA);
	dev->sg->dmah = NULL;
}

int
drm_ati_pcigart_cleanup(struct drm_device *dev,
			struct drm_ati_pcigart_info *gart_info)
{
	/* we need to support large memory configurations */
	if (dev->sg == NULL) {
		DRM_ERROR("no scatter/gather memory!\n");
		return 0;
	}

	if (gart_info->bus_addr) {
		if (gart_info->gart_table_location == DRM_ATI_GART_MAIN) {
			gart_info->bus_addr = 0;
			if (dev->sg->dmah)
				drm_ati_free_pcigart_table(dev, gart_info);
		}
	}

	return 1;
}

int
drm_ati_pcigart_init(struct drm_device *dev,
		     struct drm_ati_pcigart_info *gart_info)
{
	void *address = NULL;
	unsigned long pages;
	u32 *pci_gart, page_base;
	dma_addr_t bus_address = 0;
	dma_addr_t entry_addr;
	int i, j, ret = 0;
	int max_pages;

	/* we need to support large memory configurations */
	if (dev->sg == NULL) {
		DRM_ERROR("no scatter/gather memory!\n");
		goto done;
	}

	if (gart_info->gart_table_location == DRM_ATI_GART_MAIN) {
		DRM_DEBUG("PCI: no table in VRAM: using normal RAM\n");

		ret = drm_ati_alloc_pcigart_table(dev, gart_info);
		if (ret) {
			DRM_ERROR("cannot allocate PCI GART page!\n");
			goto done;
		}

		address = (void *)dev->sg->dmah->vaddr;
		bus_address = dev->sg->dmah->busaddr;
	} else {
		address = gart_info->addr;
		bus_address = gart_info->bus_addr;
		DRM_DEBUG("PCI: Gart Table: VRAM %08X mapped at %08lX\n",
			  (unsigned int)bus_address, (unsigned long)address);
	}

	pci_gart = (u32 *) address;

	max_pages = (gart_info->table_size / sizeof(u32));
	pages = (dev->sg->pages <= max_pages)
	    ? dev->sg->pages : max_pages;

	memset(pci_gart, 0, max_pages * sizeof(u32));

	DRM_KASSERT(PAGE_SIZE >= ATI_PCIGART_PAGE_SIZE, ("page size too small"));

	for (i = 0; i < pages; i++) {
		entry_addr = dev->sg->busaddr[i];
		for (j = 0; j < (PAGE_SIZE / ATI_PCIGART_PAGE_SIZE); j++) {
			page_base = (u32) entry_addr & ATI_PCIGART_PAGE_MASK;
			switch(gart_info->gart_reg_if) {
			case DRM_ATI_GART_IGP:
				page_base |=
				    (upper_32_bits(entry_addr) & 0xff) << 4;
				page_base |= 0xc;
				break;
			case DRM_ATI_GART_PCIE:
				page_base >>= 8;
				page_base |=
				    (upper_32_bits(entry_addr) & 0xff) << 24;
				page_base |= ATI_PCIE_READ | ATI_PCIE_WRITE;
				break;
			default:
			case DRM_ATI_GART_PCI:
				break;
			}
			*pci_gart = cpu_to_le32(page_base);
			pci_gart++;
			entry_addr += ATI_PCIGART_PAGE_SIZE;
		}
	}

	ret = 1;

    done:
	gart_info->addr = address;
	gart_info->bus_addr = bus_address;
	return ret;
}
