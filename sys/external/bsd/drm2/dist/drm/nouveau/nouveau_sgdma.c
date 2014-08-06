/*	$NetBSD: nouveau_sgdma.c,v 1.1.1.2 2014/08/06 12:36:23 riastradh Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_sgdma.c,v 1.1.1.2 2014/08/06 12:36:23 riastradh Exp $");

#include <linux/pagemap.h>
#include <linux/slab.h>

#include <subdev/fb.h>

#include "nouveau_drm.h"
#include "nouveau_ttm.h"

struct nouveau_sgdma_be {
	/* this has to be the first field so populate/unpopulated in
	 * nouve_bo.c works properly, otherwise have to move them here
	 */
	struct ttm_dma_tt ttm;
	struct drm_device *dev;
	struct nouveau_mem *node;
};

static void
nouveau_sgdma_destroy(struct ttm_tt *ttm)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)ttm;

	if (ttm) {
		ttm_dma_tt_fini(&nvbe->ttm);
		kfree(nvbe);
	}
}

static int
nv04_sgdma_bind(struct ttm_tt *ttm, struct ttm_mem_reg *mem)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)ttm;
	struct nouveau_mem *node = mem->mm_node;

	if (ttm->sg) {
		node->sg    = ttm->sg;
		node->pages = NULL;
	} else {
		node->sg    = NULL;
		node->pages = nvbe->ttm.dma_address;
	}
	node->size = (mem->num_pages << PAGE_SHIFT) >> 12;

	nouveau_vm_map(&node->vma[0], node);
	nvbe->node = node;
	return 0;
}

static int
nv04_sgdma_unbind(struct ttm_tt *ttm)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)ttm;
	nouveau_vm_unmap(&nvbe->node->vma[0]);
	return 0;
}

static struct ttm_backend_func nv04_sgdma_backend = {
	.bind			= nv04_sgdma_bind,
	.unbind			= nv04_sgdma_unbind,
	.destroy		= nouveau_sgdma_destroy
};

static int
nv50_sgdma_bind(struct ttm_tt *ttm, struct ttm_mem_reg *mem)
{
	struct nouveau_sgdma_be *nvbe = (struct nouveau_sgdma_be *)ttm;
	struct nouveau_mem *node = mem->mm_node;

	/* noop: bound in move_notify() */
	if (ttm->sg) {
		node->sg    = ttm->sg;
		node->pages = NULL;
	} else {
		node->sg    = NULL;
		node->pages = nvbe->ttm.dma_address;
	}
	node->size = (mem->num_pages << PAGE_SHIFT) >> 12;
	return 0;
}

static int
nv50_sgdma_unbind(struct ttm_tt *ttm)
{
	/* noop: unbound in move_notify() */
	return 0;
}

static struct ttm_backend_func nv50_sgdma_backend = {
	.bind			= nv50_sgdma_bind,
	.unbind			= nv50_sgdma_unbind,
	.destroy		= nouveau_sgdma_destroy
};

struct ttm_tt *
nouveau_sgdma_create_ttm(struct ttm_bo_device *bdev,
			 unsigned long size, uint32_t page_flags,
			 struct page *dummy_read_page)
{
	struct nouveau_drm *drm = nouveau_bdev(bdev);
	struct nouveau_sgdma_be *nvbe;

	nvbe = kzalloc(sizeof(*nvbe), GFP_KERNEL);
	if (!nvbe)
		return NULL;

	nvbe->dev = drm->dev;
	if (nv_device(drm->device)->card_type < NV_50)
		nvbe->ttm.ttm.func = &nv04_sgdma_backend;
	else
		nvbe->ttm.ttm.func = &nv50_sgdma_backend;

	if (ttm_dma_tt_init(&nvbe->ttm, bdev, size, page_flags, dummy_read_page))
		return NULL;
	return &nvbe->ttm.ttm;
}
