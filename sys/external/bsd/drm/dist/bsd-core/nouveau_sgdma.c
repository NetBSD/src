#include "drmP.h"
#include "nouveau_drv.h"

#define NV_CTXDMA_PAGE_SHIFT 12
#define NV_CTXDMA_PAGE_SIZE  (1 << NV_CTXDMA_PAGE_SHIFT)
#define NV_CTXDMA_PAGE_MASK  (NV_CTXDMA_PAGE_SIZE - 1)

#if 0
struct nouveau_sgdma_be {
	struct drm_ttm_backend backend;
	struct drm_device *dev;

	int         pages;
	int         pages_populated;
	dma_addr_t *pagelist;
	int         is_bound;

	unsigned int pte_start;
};

static int
nouveau_sgdma_needs_ub_cache_adjust(struct drm_ttm_backend *be)
{
	return ((be->flags & DRM_BE_FLAG_BOUND_CACHED) ? 0 : 1);
}

static int
nouveau_sgdma_populate(struct drm_ttm_backend *be, unsigned long num_pages,
		       struct page **pages, struct page *dummy_read_page)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)be;
	int p, d, o;

	DRM_DEBUG("num_pages = %ld\n", num_pages);

	if (nvbe->pagelist)
		return -EINVAL;
	nvbe->pages    = (num_pages << PAGE_SHIFT) >> NV_CTXDMA_PAGE_SHIFT;
	nvbe->pagelist = drm_alloc(nvbe->pages*sizeof(dma_addr_t),
				   DRM_MEM_PAGES);

	nvbe->pages_populated = d = 0;
	for (p = 0; p < num_pages; p++) {
		for (o = 0; o < PAGE_SIZE; o += NV_CTXDMA_PAGE_SIZE) {
			struct page *page = pages[p];
			if (!page)
				page = dummy_read_page;
#ifdef __linux__
			nvbe->pagelist[d] = pci_map_page(nvbe->dev->pdev,
							 page, o,
							 NV_CTXDMA_PAGE_SIZE,
							 PCI_DMA_BIDIRECTIONAL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
			if (pci_dma_mapping_error(nvbe->dev->pdev, nvbe->pagelist[d])) {
#else
			if (pci_dma_mapping_error(nvbe->pagelist[d])) {
#endif
				be->func->clear(be);
				DRM_ERROR("pci_map_page failed\n");
				return -EINVAL;
			}
#endif
			nvbe->pages_populated = ++d;
		}
	}

	return 0;
}

static void
nouveau_sgdma_clear(struct drm_ttm_backend *be)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)be;
#ifdef __linux__
	int d;
#endif
	DRM_DEBUG("\n");

	if (nvbe && nvbe->pagelist) {
		if (nvbe->is_bound)
			be->func->unbind(be);
#ifdef __linux__
		for (d = 0; d < nvbe->pages_populated; d++) {
			pci_unmap_page(nvbe->dev->pdev, nvbe->pagelist[d],
				       NV_CTXDMA_PAGE_SIZE,
				       PCI_DMA_BIDIRECTIONAL);
		}
#endif
		drm_free(nvbe->pagelist, nvbe->pages*sizeof(dma_addr_t),
			 DRM_MEM_PAGES);
	}
}

static int
nouveau_sgdma_bind(struct drm_ttm_backend *be, struct drm_bo_mem_reg *mem)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)be;
	struct drm_nouveau_private *dev_priv = nvbe->dev->dev_private;
	struct nouveau_gpuobj *gpuobj = dev_priv->gart_info.sg_ctxdma;
	uint64_t offset = (mem->mm_node->start << PAGE_SHIFT);
	uint32_t i;

	DRM_DEBUG("pg=0x%lx (0x%llx), cached=%d\n", mem->mm_node->start,
		  (unsigned long long)offset,
		  (mem->flags & DRM_BO_FLAG_CACHED) == 1);

	if (offset & NV_CTXDMA_PAGE_MASK)
		return -EINVAL;
	nvbe->pte_start = (offset >> NV_CTXDMA_PAGE_SHIFT);
	if (dev_priv->card_type < NV_50)
		nvbe->pte_start += 2; /* skip ctxdma header */

	for (i = nvbe->pte_start; i < nvbe->pte_start + nvbe->pages; i++) {
		uint64_t pteval = nvbe->pagelist[i - nvbe->pte_start];

		if (pteval & NV_CTXDMA_PAGE_MASK) {
			DRM_ERROR("Bad pteval 0x%llx\n",
				(unsigned long long)pteval);
			return -EINVAL;
		}

		if (dev_priv->card_type < NV_50) {
			INSTANCE_WR(gpuobj, i, pteval | 3);
		} else {
			INSTANCE_WR(gpuobj, (i<<1)+0, pteval | 0x21);
			INSTANCE_WR(gpuobj, (i<<1)+1, 0x00000000);
		}
	}

	nvbe->is_bound  = 1;
	return 0;
}

static int
nouveau_sgdma_unbind(struct drm_ttm_backend *be)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)be;
	struct drm_nouveau_private *dev_priv = nvbe->dev->dev_private;

	DRM_DEBUG("\n");

	if (nvbe->is_bound) {
		struct nouveau_gpuobj *gpuobj = dev_priv->gart_info.sg_ctxdma;
		unsigned int pte;

		pte = nvbe->pte_start;
		while (pte < (nvbe->pte_start + nvbe->pages)) {
			uint64_t pteval = dev_priv->gart_info.sg_dummy_bus;

			if (dev_priv->card_type < NV_50) {
				INSTANCE_WR(gpuobj, pte, pteval | 3);
			} else {
				INSTANCE_WR(gpuobj, (pte<<1)+0, pteval | 0x21);
				INSTANCE_WR(gpuobj, (pte<<1)+1, 0x00000000);
			}

			pte++;
		}

		nvbe->is_bound = 0;
	}

	return 0;
}

static void
nouveau_sgdma_destroy(struct drm_ttm_backend *be)
{
	DRM_DEBUG("\n");
	if (be) {
		struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)be;
		if (nvbe) {
			if (nvbe->pagelist)
				be->func->clear(be);
			drm_ctl_free(nvbe, sizeof(*nvbe), DRM_MEM_TTM);
		}
	}
}

static struct drm_ttm_backend_func nouveau_sgdma_backend = {
	.needs_ub_cache_adjust	= nouveau_sgdma_needs_ub_cache_adjust,
	.populate		= nouveau_sgdma_populate,
	.clear			= nouveau_sgdma_clear,
	.bind			= nouveau_sgdma_bind,
	.unbind			= nouveau_sgdma_unbind,
	.destroy		= nouveau_sgdma_destroy
};

struct drm_ttm_backend *
nouveau_sgdma_init_ttm(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_sgdma_be *nvbe;

	if (!dev_priv->gart_info.sg_ctxdma)
		return NULL;

	nvbe = drm_ctl_calloc(1, sizeof(*nvbe), DRM_MEM_TTM);
	if (!nvbe)
		return NULL;

	nvbe->dev = dev;

	nvbe->backend.func	= &nouveau_sgdma_backend;

	return &nvbe->backend;
}
#endif

int
nouveau_sgdma_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *gpuobj = NULL;
	uint32_t aper_size, obj_size;
	int i, ret;

	if (dev_priv->card_type < NV_50) {
		aper_size = (64 * 1024 * 1024);
		obj_size  = (aper_size >> NV_CTXDMA_PAGE_SHIFT) * 4;
		obj_size += 8; /* ctxdma header */
	} else {
		/* 1 entire VM page table */
		aper_size = (512 * 1024 * 1024);
		obj_size  = (aper_size >> NV_CTXDMA_PAGE_SHIFT) * 8;
	}

	if ((ret = nouveau_gpuobj_new(dev, NULL, obj_size, 16,
				      NVOBJ_FLAG_ALLOW_NO_REFS |
				      NVOBJ_FLAG_ZERO_ALLOC |
				      NVOBJ_FLAG_ZERO_FREE, &gpuobj)))  {
		DRM_ERROR("Error creating sgdma object: %d\n", ret);
		return ret;
	}
#ifdef __linux__
	dev_priv->gart_info.sg_dummy_page =
		alloc_page(GFP_KERNEL|__GFP_DMA32);
	set_page_locked(dev_priv->gart_info.sg_dummy_page);
	dev_priv->gart_info.sg_dummy_bus =
		pci_map_page(dev->pdev, dev_priv->gart_info.sg_dummy_page, 0,
			     PAGE_SIZE, PCI_DMA_BIDIRECTIONAL);
#endif
	if (dev_priv->card_type < NV_50) {
		/* Maybe use NV_DMA_TARGET_AGP for PCIE? NVIDIA do this, and
		 * confirmed to work on c51.  Perhaps means NV_DMA_TARGET_PCIE
		 * on those cards? */
		INSTANCE_WR(gpuobj, 0, NV_CLASS_DMA_IN_MEMORY |
				       (1 << 12) /* PT present */ |
				       (0 << 13) /* PT *not* linear */ |
				       (NV_DMA_ACCESS_RW  << 14) |
				       (NV_DMA_TARGET_PCI << 16));
		INSTANCE_WR(gpuobj, 1, aper_size - 1);
		for (i=2; i<2+(aper_size>>12); i++) {
			INSTANCE_WR(gpuobj, i,
				    dev_priv->gart_info.sg_dummy_bus | 3);
		}
	} else {
		for (i=0; i<obj_size; i+=8) {
			INSTANCE_WR(gpuobj, (i+0)/4,
				    dev_priv->gart_info.sg_dummy_bus | 0x21);
			INSTANCE_WR(gpuobj, (i+4)/4, 0);
		}
	}

	dev_priv->gart_info.type      = NOUVEAU_GART_SGDMA;
	dev_priv->gart_info.aper_base = 0;
	dev_priv->gart_info.aper_size = aper_size;
	dev_priv->gart_info.sg_ctxdma = gpuobj;
	return 0;
}

void
nouveau_sgdma_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if (dev_priv->gart_info.sg_dummy_page) {
#ifdef __linux__
		pci_unmap_page(dev->pdev, dev_priv->gart_info.sg_dummy_bus,
			       NV_CTXDMA_PAGE_SIZE, PCI_DMA_BIDIRECTIONAL);
		unlock_page(dev_priv->gart_info.sg_dummy_page);
		__free_page(dev_priv->gart_info.sg_dummy_page);
#endif
		dev_priv->gart_info.sg_dummy_page = NULL;
		dev_priv->gart_info.sg_dummy_bus = 0;
	}

	nouveau_gpuobj_del(dev, &dev_priv->gart_info.sg_ctxdma);
}

#if 0
int
nouveau_sgdma_nottm_hack_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_ttm_backend *be;
	struct drm_scatter_gather sgreq;
	struct drm_mm_node mm_node;
	struct drm_bo_mem_reg mem;
	int ret;

	dev_priv->gart_info.sg_be = nouveau_sgdma_init_ttm(dev);
	if (!dev_priv->gart_info.sg_be)
		return -ENOMEM;
	be = dev_priv->gart_info.sg_be;

	/* Hack the aperture size down to the amount of system memory
	 * we're going to bind into it.
	 */
	if (dev_priv->gart_info.aper_size > 32*1024*1024)
		dev_priv->gart_info.aper_size = 32*1024*1024;

	sgreq.size = dev_priv->gart_info.aper_size;
	if ((ret = drm_sg_alloc(dev, &sgreq))) {
		DRM_ERROR("drm_sg_alloc failed: %d\n", ret);
		return ret;
	}
	dev_priv->gart_info.sg_handle = sgreq.handle;

	if ((ret = be->func->populate(be, dev->sg->pages, dev->sg->pagelist, dev->bm.dummy_read_page))) {
		DRM_ERROR("failed populate: %d\n", ret);
		return ret;
	}

	mm_node.start = 0;
	mem.mm_node = &mm_node;

	if ((ret = be->func->bind(be, &mem))) {
		DRM_ERROR("failed bind: %d\n", ret);
		return ret;
	}

	return 0;
}

void
nouveau_sgdma_nottm_hack_takedown(struct drm_device *dev)
{
}
#endif

int
nouveau_sgdma_get_page(struct drm_device *dev, uint32_t offset, uint32_t *page)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *gpuobj = dev_priv->gart_info.sg_ctxdma;
	int pte;

	pte = (offset >> NV_CTXDMA_PAGE_SHIFT);
	if (dev_priv->card_type < NV_50) {
		*page = INSTANCE_RD(gpuobj, (pte + 2)) & ~NV_CTXDMA_PAGE_MASK;
		return 0;
	}

	DRM_ERROR("Unimplemented on NV50\n");
	return -EINVAL;
}
