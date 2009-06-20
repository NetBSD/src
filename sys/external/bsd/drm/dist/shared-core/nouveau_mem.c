/*
 * Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
 * Copyright 2005 Stephane Marchesin
 *
 * The Weather Channel (TM) funded Tungsten Graphics to develop the
 * initial release of the Radeon 8500 driver under the XFree86 license.
 * This notice must be preserved.
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
 * THE AUTHORS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "drmP.h"
#include "drm.h"
#include "drm_sarea.h"
#include "nouveau_drv.h"

static struct mem_block *
split_block(struct mem_block *p, uint64_t start, uint64_t size,
	    struct drm_file *file_priv)
{
	/* Maybe cut off the start of an existing block */
	if (start > p->start) {
		struct mem_block *newblock =
			drm_alloc(sizeof(*newblock), DRM_MEM_BUFS);
		if (!newblock)
			goto out;
		newblock->start = start;
		newblock->size = p->size - (start - p->start);
		newblock->file_priv = NULL;
		newblock->next = p->next;
		newblock->prev = p;
		p->next->prev = newblock;
		p->next = newblock;
		p->size -= newblock->size;
		p = newblock;
	}

	/* Maybe cut off the end of an existing block */
	if (size < p->size) {
		struct mem_block *newblock =
			drm_alloc(sizeof(*newblock), DRM_MEM_BUFS);
		if (!newblock)
			goto out;
		newblock->start = start + size;
		newblock->size = p->size - size;
		newblock->file_priv = NULL;
		newblock->next = p->next;
		newblock->prev = p;
		p->next->prev = newblock;
		p->next = newblock;
		p->size = size;
	}

out:
	/* Our block is in the middle */
	p->file_priv = file_priv;
	return p;
}

struct mem_block *
nouveau_mem_alloc_block(struct mem_block *heap, uint64_t size,
			int align2, struct drm_file *file_priv, int tail)
{
	struct mem_block *p;
	uint64_t mask = (1 << align2) - 1;

	if (!heap)
		return NULL;

	if (tail) {
		list_for_each_prev(p, heap) {
			uint64_t start = ((p->start + p->size) - size) & ~mask;

			if (p->file_priv == 0 && start >= p->start &&
			    start + size <= p->start + p->size)
				return split_block(p, start, size, file_priv);
		}
	} else {
		list_for_each(p, heap) {
			uint64_t start = (p->start + mask) & ~mask;

			if (p->file_priv == 0 &&
			    start + size <= p->start + p->size)
				return split_block(p, start, size, file_priv);
		}
	}

	return NULL;
}

static struct mem_block *find_block(struct mem_block *heap, uint64_t start)
{
	struct mem_block *p;

	list_for_each(p, heap)
		if (p->start == start)
			return p;

	return NULL;
}

void nouveau_mem_free_block(struct mem_block *p)
{
	p->file_priv = NULL;

	/* Assumes a single contiguous range.  Needs a special file_priv in
	 * 'heap' to stop it being subsumed.
	 */
	if (p->next->file_priv == 0) {
		struct mem_block *q = p->next;
		p->size += q->size;
		p->next = q->next;
		p->next->prev = p;
		drm_free(q, sizeof(*q), DRM_MEM_BUFS);
	}

	if (p->prev->file_priv == 0) {
		struct mem_block *q = p->prev;
		q->size += p->size;
		q->next = p->next;
		q->next->prev = q;
		drm_free(p, sizeof(*q), DRM_MEM_BUFS);
	}
}

/* Initialize.  How to check for an uninitialized heap?
 */
int nouveau_mem_init_heap(struct mem_block **heap, uint64_t start,
			  uint64_t size)
{
	struct mem_block *blocks = drm_alloc(sizeof(*blocks), DRM_MEM_BUFS);

	if (!blocks)
		return -ENOMEM;

	*heap = drm_alloc(sizeof(**heap), DRM_MEM_BUFS);
	if (!*heap) {
		drm_free(blocks, sizeof(*blocks), DRM_MEM_BUFS);
		return -ENOMEM;
	}

	blocks->start = start;
	blocks->size = size;
	blocks->file_priv = NULL;
	blocks->next = blocks->prev = *heap;

	memset(*heap, 0, sizeof(**heap));
	(*heap)->file_priv = (struct drm_file *) - 1;
	(*heap)->next = (*heap)->prev = blocks;
	return 0;
}

/*
 * Free all blocks associated with the releasing file_priv
 */
void nouveau_mem_release(struct drm_file *file_priv, struct mem_block *heap)
{
	struct mem_block *p;

	if (!heap || !heap->next)
		return;

	list_for_each(p, heap) {
		if (p->file_priv == file_priv)
			p->file_priv = NULL;
	}

	/* Assumes a single contiguous range.  Needs a special file_priv in
	 * 'heap' to stop it being subsumed.
	 */
	list_for_each(p, heap) {
		while ((p->file_priv == 0) && (p->next->file_priv == 0) &&
		       (p->next!=heap)) {
			struct mem_block *q = p->next;
			p->size += q->size;
			p->next = q->next;
			p->next->prev = p;
			drm_free(q, sizeof(*q), DRM_MEM_DRIVER);
		}
	}
}

/*
 * Cleanup everything
 */
void nouveau_mem_takedown(struct mem_block **heap)
{
	struct mem_block *p;

	if (!*heap)
		return;

	for (p = (*heap)->next; p != *heap;) {
		struct mem_block *q = p;
		p = p->next;
		drm_free(q, sizeof(*q), DRM_MEM_DRIVER);
	}

	drm_free(*heap, sizeof(**heap), DRM_MEM_DRIVER);
	*heap = NULL;
}

void nouveau_mem_close(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	nouveau_mem_takedown(&dev_priv->agp_heap);
	nouveau_mem_takedown(&dev_priv->fb_heap);
	if (dev_priv->pci_heap)
		nouveau_mem_takedown(&dev_priv->pci_heap);
}

/*XXX BSD needs compat functions for pci access
 * #define DRM_PCI_DEV		struct device
 * #define drm_pci_get_bsf	pci_get_bsf
 * and a small inline to do	*val = pci_read_config(pdev->device, where, 4);
 * might work
 */
static int nforce_pci_fn_read_config_dword(int devfn, int where, uint32_t *val)
{
#ifdef __linux__
	DRM_PCI_DEV *pdev;

	if (!(pdev = drm_pci_get_bsf(0, 0, devfn))) {
		DRM_ERROR("nForce PCI device function 0x%02x not found\n",
			  devfn);
		return -ENODEV;
	}

	return drm_pci_read_config_dword(pdev, where, val);
#else
	DRM_ERROR("BSD compat for checking IGP memory amount needed\n");
	return 0;
#endif
}

static void nouveau_mem_check_nforce_dimms(struct drm_device *dev)
{
	uint32_t mem_ctrlr_pciid;

	nforce_pci_fn_read_config_dword(3, 0x00, &mem_ctrlr_pciid);
	mem_ctrlr_pciid >>= 16;

	if (mem_ctrlr_pciid == 0x01a9 || mem_ctrlr_pciid == 0x01ab ||
	    mem_ctrlr_pciid == 0x01ed) {
		uint32_t dimm[3];
		int i;

		for (i = 0; i < 3; i++) {
			nforce_pci_fn_read_config_dword(2, 0x40 + i * 4, &dimm[i]);
			dimm[i] = (dimm[i] >> 8) & 0x4f;
		}

		if (dimm[0] + dimm[1] != dimm[2])
			DRM_INFO("Your nForce DIMMs are not arranged in "
				 "optimal banks!\n");
	}
}

static uint32_t
nouveau_mem_fb_amount_igp(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t mem = 0;

	if (dev_priv->flags & NV_NFORCE) {
		nforce_pci_fn_read_config_dword(1, 0x7C, &mem);
		return (uint64_t)(((mem >> 6) & 31) + 1)*1024*1024;
	}
	if (dev_priv->flags & NV_NFORCE2) {
		nforce_pci_fn_read_config_dword(1, 0x84, &mem);
		return (uint64_t)(((mem >> 4) & 127) + 1)*1024*1024;
	}

	DRM_ERROR("impossible!\n");

	return 0;
}

/* returns the amount of FB ram in bytes */
uint64_t nouveau_mem_fb_amount(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv=dev->dev_private;
	switch(dev_priv->card_type)
	{
		case NV_04:
		case NV_05:
			if (NV_READ(NV03_BOOT_0) & 0x00000100) {
				return (((NV_READ(NV03_BOOT_0) >> 12) & 0xf)*2+2)*1024*1024;
			} else
			switch(NV_READ(NV03_BOOT_0)&NV03_BOOT_0_RAM_AMOUNT)
			{
				case NV04_BOOT_0_RAM_AMOUNT_32MB:
					return 32*1024*1024;
				case NV04_BOOT_0_RAM_AMOUNT_16MB:
					return 16*1024*1024;
				case NV04_BOOT_0_RAM_AMOUNT_8MB:
					return 8*1024*1024;
				case NV04_BOOT_0_RAM_AMOUNT_4MB:
					return 4*1024*1024;
			}
			break;
		case NV_10:
		case NV_11:
		case NV_17:
		case NV_20:
		case NV_30:
		case NV_40:
		case NV_44:
		case NV_50:
		default:
			if (dev_priv->flags & (NV_NFORCE | NV_NFORCE2)) {
				return nouveau_mem_fb_amount_igp(dev);
			} else {
				uint64_t mem;

				mem = (NV_READ(NV10_PFB_CSTATUS) &
				       NV10_PFB_CSTATUS_RAM_AMOUNT_MB_MASK) >>
				      NV10_PFB_CSTATUS_RAM_AMOUNT_MB_SHIFT;
				return mem*1024*1024;
			}
			break;
	}

	DRM_ERROR("Unable to detect video ram size. Please report your setup to " DRIVER_EMAIL "\n");
	return 0;
}

static void nouveau_mem_reset_agp(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t saved_pci_nv_1, saved_pci_nv_19, pmc_enable;

	saved_pci_nv_1 = NV_READ(NV04_PBUS_PCI_NV_1);
	saved_pci_nv_19 = NV_READ(NV04_PBUS_PCI_NV_19);

	/* clear busmaster bit */
	NV_WRITE(NV04_PBUS_PCI_NV_1, saved_pci_nv_1 & ~0x4);
	/* clear SBA and AGP bits */
	NV_WRITE(NV04_PBUS_PCI_NV_19, saved_pci_nv_19 & 0xfffff0ff);

	/* power cycle pgraph, if enabled */
	pmc_enable = NV_READ(NV03_PMC_ENABLE);
	if (pmc_enable & NV_PMC_ENABLE_PGRAPH) {
		NV_WRITE(NV03_PMC_ENABLE, pmc_enable & ~NV_PMC_ENABLE_PGRAPH);
		NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) |
				NV_PMC_ENABLE_PGRAPH);
	}

	/* and restore (gives effect of resetting AGP) */
	NV_WRITE(NV04_PBUS_PCI_NV_19, saved_pci_nv_19);
	NV_WRITE(NV04_PBUS_PCI_NV_1, saved_pci_nv_1);
}

static int
nouveau_mem_init_agp(struct drm_device *dev, int ttm)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_agp_info info;
	struct drm_agp_mode mode;
	int ret;

	nouveau_mem_reset_agp(dev);

	ret = drm_agp_acquire(dev);
	if (ret) {
		DRM_ERROR("Unable to acquire AGP: %d\n", ret);
		return ret;
	}

	ret = drm_agp_info(dev, &info);
	if (ret) {
		DRM_ERROR("Unable to get AGP info: %d\n", ret);
		return ret;
	}

	/* see agp.h for the AGPSTAT_* modes available */
	mode.mode = info.mode;
	ret = drm_agp_enable(dev, mode);
	if (ret) {
		DRM_ERROR("Unable to enable AGP: %d\n", ret);
		return ret;
	}

	if (!ttm) {
		struct drm_agp_buffer agp_req;
		struct drm_agp_binding bind_req;

		agp_req.size = info.aperture_size;
		agp_req.type = 0;
		ret = drm_agp_alloc(dev, &agp_req);
		if (ret) {
			DRM_ERROR("Unable to alloc AGP: %d\n", ret);
				return ret;
		}

		bind_req.handle = agp_req.handle;
		bind_req.offset = 0;
		ret = drm_agp_bind(dev, &bind_req);
		if (ret) {
			DRM_ERROR("Unable to bind AGP: %d\n", ret);
			return ret;
		}
	}

	dev_priv->gart_info.type	= NOUVEAU_GART_AGP;
	dev_priv->gart_info.aper_base	= info.aperture_base;
	dev_priv->gart_info.aper_size	= info.aperture_size;
	return 0;
}

#define HACK_OLD_MM
int
nouveau_mem_init_ttm(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t vram_size, bar1_size;
	int ret;

	dev_priv->agp_heap = dev_priv->pci_heap = dev_priv->fb_heap = NULL;
	dev_priv->fb_phys = drm_get_resource_start(dev,1);
	dev_priv->gart_info.type = NOUVEAU_GART_NONE;

	drm_bo_driver_init(dev);

	/* non-mappable vram */
	dev_priv->fb_available_size = nouveau_mem_fb_amount(dev);
	dev_priv->fb_available_size -= dev_priv->ramin_rsvd_vram;
	vram_size = dev_priv->fb_available_size >> PAGE_SHIFT;
	bar1_size = drm_get_resource_len(dev, 1) >> PAGE_SHIFT;
	if (bar1_size < vram_size) {
		if ((ret = drm_bo_init_mm(dev, DRM_BO_MEM_PRIV0,
					  bar1_size, vram_size - bar1_size, 1))) {
			DRM_ERROR("Failed PRIV0 mm init: %d\n", ret);
			return ret;
		}
		vram_size = bar1_size;
	}

	/* mappable vram */
#ifdef HACK_OLD_MM
	vram_size /= 4;
#endif
	if ((ret = drm_bo_init_mm(dev, DRM_BO_MEM_VRAM, 0, vram_size, 1))) {
		DRM_ERROR("Failed VRAM mm init: %d\n", ret);
		return ret;
	}

	/* GART */
#if !defined(__powerpc__) && !defined(__ia64__)
	if (drm_device_is_agp(dev) && dev->agp) {
		if ((ret = nouveau_mem_init_agp(dev, 1)))
			DRM_ERROR("Error initialising AGP: %d\n", ret);
	}
#endif

	if (dev_priv->gart_info.type == NOUVEAU_GART_NONE) {
		if ((ret = nouveau_sgdma_init(dev)))
			DRM_ERROR("Error initialising PCI SGDMA: %d\n", ret);
	}

	if ((ret = drm_bo_init_mm(dev, DRM_BO_MEM_TT, 0,
				  dev_priv->gart_info.aper_size >>
				  PAGE_SHIFT, 1))) {
		DRM_ERROR("Failed TT mm init: %d\n", ret);
		return ret;
	}

#ifdef HACK_OLD_MM
	vram_size <<= PAGE_SHIFT;
	DRM_INFO("Old MM using %dKiB VRAM\n", (vram_size * 3) >> 10);
	if (nouveau_mem_init_heap(&dev_priv->fb_heap, vram_size, vram_size * 3))
		return -ENOMEM;
#endif

	return 0;
}

int nouveau_mem_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t fb_size;
	int ret = 0;

	dev_priv->agp_heap = dev_priv->pci_heap = dev_priv->fb_heap = NULL;
	dev_priv->fb_phys = 0;
	dev_priv->gart_info.type = NOUVEAU_GART_NONE;

	if (dev_priv->flags & (NV_NFORCE | NV_NFORCE2))
		nouveau_mem_check_nforce_dimms(dev);

	/* setup a mtrr over the FB */
	dev_priv->fb_mtrr = drm_mtrr_add(drm_get_resource_start(dev, 1),
					 nouveau_mem_fb_amount(dev),
					 DRM_MTRR_WC);

	/* Init FB */
	dev_priv->fb_phys=drm_get_resource_start(dev,1);
	fb_size = nouveau_mem_fb_amount(dev);
	/* On G80, limit VRAM to 512MiB temporarily due to limits in how
	 * we handle VRAM page tables.
	 */
	if (dev_priv->card_type >= NV_50 && fb_size > (512 * 1024 * 1024))
		fb_size = (512 * 1024 * 1024);
	fb_size -= dev_priv->ramin_rsvd_vram;
	dev_priv->fb_available_size = fb_size;
	DRM_DEBUG("Available VRAM: %dKiB\n", fb_size>>10);

	if (fb_size>256*1024*1024) {
		/* On cards with > 256Mb, you can't map everything.
		 * So we create a second FB heap for that type of memory */
		if (nouveau_mem_init_heap(&dev_priv->fb_heap,
					  0, 256*1024*1024))
			return -ENOMEM;
		if (nouveau_mem_init_heap(&dev_priv->fb_nomap_heap,
					  256*1024*1024, fb_size-256*1024*1024))
			return -ENOMEM;
	} else {
		if (nouveau_mem_init_heap(&dev_priv->fb_heap, 0, fb_size))
			return -ENOMEM;
		dev_priv->fb_nomap_heap=NULL;
	}

#if !defined(__powerpc__) && !defined(__ia64__)
	/* Init AGP / NV50 PCIEGART */
	if (drm_device_is_agp(dev) && dev->agp) {
		if ((ret = nouveau_mem_init_agp(dev, 0)))
			DRM_ERROR("Error initialising AGP: %d\n", ret);
	}
#endif

	/*Note: this is *not* just NV50 code, but only used on NV50 for now */
	if (dev_priv->gart_info.type == NOUVEAU_GART_NONE &&
	    dev_priv->card_type >= NV_50) {
		ret = nouveau_sgdma_init(dev);
		if (!ret) {
			ret = nouveau_sgdma_nottm_hack_init(dev);
			if (ret)
				nouveau_sgdma_takedown(dev);
		}

		if (ret)
			DRM_ERROR("Error initialising SG DMA: %d\n", ret);
	}

	if (dev_priv->gart_info.type != NOUVEAU_GART_NONE) {
		if (nouveau_mem_init_heap(&dev_priv->agp_heap,
					  0, dev_priv->gart_info.aper_size)) {
			if (dev_priv->gart_info.type == NOUVEAU_GART_SGDMA) {
				nouveau_sgdma_nottm_hack_takedown(dev);
				nouveau_sgdma_takedown(dev);
			}
		}
	}

	/* NV04-NV40 PCIEGART */
	if (!dev_priv->agp_heap && dev_priv->card_type < NV_50) {
		struct drm_scatter_gather sgreq;

		DRM_DEBUG("Allocating sg memory for PCI DMA\n");
		sgreq.size = 16 << 20; //16MB of PCI scatter-gather zone

		if (drm_sg_alloc(dev, &sgreq)) {
			DRM_ERROR("Unable to allocate %ldMB of scatter-gather"
				  " pages for PCI DMA!",sgreq.size>>20);
		} else {
			if (nouveau_mem_init_heap(&dev_priv->pci_heap, 0,
						  dev->sg->pages * PAGE_SIZE)) {
				DRM_ERROR("Unable to initialize pci_heap!");
			}
		}
	}

	/* G8x: Allocate shared page table to map real VRAM pages into */
	if (dev_priv->card_type >= NV_50) {
		unsigned size = ((512 * 1024 * 1024) / 65536) * 8;

		ret = nouveau_gpuobj_new(dev, NULL, size, 0,
					 NVOBJ_FLAG_ZERO_ALLOC |
					 NVOBJ_FLAG_ALLOW_NO_REFS,
					 &dev_priv->vm_vram_pt);
		if (ret) {
			DRM_ERROR("Error creating VRAM page table: %d\n", ret);
			return ret;
		}
	}


	return 0;
}

struct mem_block *
nouveau_mem_alloc(struct drm_device *dev, int alignment, uint64_t size,
		  int flags, struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct mem_block *block;
	int type, tail = !(flags & NOUVEAU_MEM_USER);

	/*
	 * Make things easier on ourselves: all allocations are page-aligned.
	 * We need that to map allocated regions into the user space
	 */
	if (alignment < PAGE_SIZE)
		alignment = PAGE_SIZE;

	/* Align allocation sizes to 64KiB blocks on G8x.  We use a 64KiB
	 * page size in the GPU VM.
	 */
	if (flags & NOUVEAU_MEM_FB && dev_priv->card_type >= NV_50) {
		size = (size + 65535) & ~65535;
		if (alignment < 65536)
			alignment = 65536;
	}

	/* Further down wants alignment in pages, not bytes */
	alignment >>= PAGE_SHIFT;

	/*
	 * Warn about 0 sized allocations, but let it go through. It'll return 1 page
	 */
	if (size == 0)
		DRM_INFO("warning : 0 byte allocation\n");

	/*
	 * Keep alloc size a multiple of the page size to keep drm_addmap() happy
	 */
	if (size & (~PAGE_MASK))
		size = ((size/PAGE_SIZE) + 1) * PAGE_SIZE;


#define NOUVEAU_MEM_ALLOC_AGP {\
		type=NOUVEAU_MEM_AGP;\
                block = nouveau_mem_alloc_block(dev_priv->agp_heap, size,\
                                                alignment, file_priv, tail); \
                if (block) goto alloc_ok;\
	        }

#define NOUVEAU_MEM_ALLOC_PCI {\
                type = NOUVEAU_MEM_PCI;\
                block = nouveau_mem_alloc_block(dev_priv->pci_heap, size, \
						alignment, file_priv, tail); \
                if ( block ) goto alloc_ok;\
	        }

#define NOUVEAU_MEM_ALLOC_FB {\
                type=NOUVEAU_MEM_FB;\
                if (!(flags&NOUVEAU_MEM_MAPPED)) {\
                        block = nouveau_mem_alloc_block(dev_priv->fb_nomap_heap,\
                                                        size, alignment, \
							file_priv, tail); \
                        if (block) goto alloc_ok;\
                }\
                block = nouveau_mem_alloc_block(dev_priv->fb_heap, size,\
                                                alignment, file_priv, tail);\
                if (block) goto alloc_ok;\
	        }


	if (flags&NOUVEAU_MEM_FB) NOUVEAU_MEM_ALLOC_FB
	if (flags&NOUVEAU_MEM_AGP) NOUVEAU_MEM_ALLOC_AGP
	if (flags&NOUVEAU_MEM_PCI) NOUVEAU_MEM_ALLOC_PCI
	if (flags&NOUVEAU_MEM_FB_ACCEPTABLE) NOUVEAU_MEM_ALLOC_FB
	if (flags&NOUVEAU_MEM_AGP_ACCEPTABLE) NOUVEAU_MEM_ALLOC_AGP
	if (flags&NOUVEAU_MEM_PCI_ACCEPTABLE) NOUVEAU_MEM_ALLOC_PCI


	return NULL;

alloc_ok:
	block->flags=type;

	/* On G8x, map memory into VM */
	if (block->flags & NOUVEAU_MEM_FB && dev_priv->card_type >= NV_50 &&
	    !(flags & NOUVEAU_MEM_NOVM)) {
		struct nouveau_gpuobj *pt = dev_priv->vm_vram_pt;
		unsigned offset = block->start;
		unsigned count = block->size / 65536;
		unsigned tile = 0;

		if (!pt) {
			DRM_ERROR("vm alloc without vm pt\n");
			nouveau_mem_free_block(block);
			return NULL;
		}

		/* The tiling stuff is *not* what NVIDIA does - but both the
		 * 2D and 3D engines seem happy with this simpler method.
		 * Should look into why NVIDIA do what they do at some point.
		 */
		if (flags & NOUVEAU_MEM_TILE) {
			if (flags & NOUVEAU_MEM_TILE_ZETA)
				tile = 0x00002800;
			else
				tile = 0x00007000;
		}

		while (count--) {
			unsigned pte = offset / 65536;

			INSTANCE_WR(pt, (pte * 2) + 0, offset | 1);
			INSTANCE_WR(pt, (pte * 2) + 1, 0x00000000 | tile);
			offset += 65536;
		}
	} else {
		block->flags |= NOUVEAU_MEM_NOVM;
	}	

	if (flags&NOUVEAU_MEM_MAPPED)
	{
		struct drm_map_list *entry;
		int ret = 0;
		block->flags|=NOUVEAU_MEM_MAPPED;

		if (type == NOUVEAU_MEM_AGP) {
			if (dev_priv->gart_info.type != NOUVEAU_GART_SGDMA)
			ret = drm_addmap(dev, block->start, block->size,
					 _DRM_AGP, 0, &block->map);
			else
			ret = drm_addmap(dev, block->start, block->size,
					 _DRM_SCATTER_GATHER, 0, &block->map);
		}
		else if (type == NOUVEAU_MEM_FB)
			ret = drm_addmap(dev, block->start + dev_priv->fb_phys,
					 block->size, _DRM_FRAME_BUFFER,
					 0, &block->map);
		else if (type == NOUVEAU_MEM_PCI)
			ret = drm_addmap(dev, block->start, block->size,
					 _DRM_SCATTER_GATHER, 0, &block->map);

		if (ret) {
			nouveau_mem_free_block(block);
			return NULL;
		}

		entry = drm_find_matching_map(dev, block->map);
		if (!entry) {
			nouveau_mem_free_block(block);
			return NULL;
		}
		block->map_handle = entry->user_token;
	}

	DRM_DEBUG("allocated %lld bytes at 0x%llx type=0x%08x\n", block->size, block->start, block->flags);
	return block;
}

void nouveau_mem_free(struct drm_device* dev, struct mem_block* block)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("freeing 0x%llx type=0x%08x\n", block->start, block->flags);

	if (block->flags&NOUVEAU_MEM_MAPPED)
		drm_rmmap(dev, block->map);

	/* G8x: Remove pages from vm */
	if (block->flags & NOUVEAU_MEM_FB && dev_priv->card_type >= NV_50 &&
	    !(block->flags & NOUVEAU_MEM_NOVM)) {
		struct nouveau_gpuobj *pt = dev_priv->vm_vram_pt;
		unsigned offset = block->start;
		unsigned count = block->size / 65536;

		if (!pt) {
			DRM_ERROR("vm free without vm pt\n");
			goto out_free;
		}

		while (count--) {
			unsigned pte = offset / 65536;
			INSTANCE_WR(pt, (pte * 2) + 0, 0);
			INSTANCE_WR(pt, (pte * 2) + 1, 0);
			offset += 65536;
		}
	}

out_free:
	nouveau_mem_free_block(block);
}

/*
 * Ioctls
 */

int
nouveau_ioctl_mem_alloc(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_mem_alloc *alloc = data;
	struct mem_block *block;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (alloc->flags & NOUVEAU_MEM_INTERNAL)
		return -EINVAL;

	block = nouveau_mem_alloc(dev, alloc->alignment, alloc->size,
				  alloc->flags | NOUVEAU_MEM_USER, file_priv);
	if (!block)
		return -ENOMEM;
	alloc->map_handle=block->map_handle;
	alloc->offset=block->start;
	alloc->flags=block->flags;

	if (dev_priv->card_type >= NV_50 && alloc->flags & NOUVEAU_MEM_FB)
		alloc->offset += 512*1024*1024;

	return 0;
}

int
nouveau_ioctl_mem_free(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_mem_free *memfree = data;
	struct mem_block *block;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (dev_priv->card_type >= NV_50 && memfree->flags & NOUVEAU_MEM_FB)
		memfree->offset -= 512*1024*1024;

	block=NULL;
	if (dev_priv->fb_heap && memfree->flags & NOUVEAU_MEM_FB)
		block = find_block(dev_priv->fb_heap, memfree->offset);
	else if (dev_priv->agp_heap && memfree->flags & NOUVEAU_MEM_AGP)
		block = find_block(dev_priv->agp_heap, memfree->offset);
	else if (dev_priv->pci_heap && memfree->flags & NOUVEAU_MEM_PCI)
		block = find_block(dev_priv->pci_heap, memfree->offset);
	if (!block)
		return -EFAULT;
	if (block->file_priv != file_priv)
		return -EPERM;

	nouveau_mem_free(dev, block);
	return 0;
}

int
nouveau_ioctl_mem_tile(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_mem_tile *memtile = data;
	struct mem_block *block = NULL;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	if (dev_priv->card_type < NV_50)
		return -EINVAL;
	
	if (memtile->flags & NOUVEAU_MEM_FB) {
		memtile->offset -= 512*1024*1024;
		block = find_block(dev_priv->fb_heap, memtile->offset);
	}

	if (!block)
		return -EINVAL;

	if (block->file_priv != file_priv)
		return -EPERM;

	{
		struct nouveau_gpuobj *pt = dev_priv->vm_vram_pt;
		unsigned offset = block->start + memtile->delta;
		unsigned count = memtile->size / 65536;
		unsigned tile = 0;

		if (memtile->flags & NOUVEAU_MEM_TILE) {
			if (memtile->flags & NOUVEAU_MEM_TILE_ZETA)
				tile = 0x00002800;
			else
				tile = 0x00007000;
		}

		while (count--) {
			unsigned pte = offset / 65536;

			INSTANCE_WR(pt, (pte * 2) + 0, offset | 1);
			INSTANCE_WR(pt, (pte * 2) + 1, 0x00000000 | tile);
			offset += 65536;
		}
	}

	return 0;
}

