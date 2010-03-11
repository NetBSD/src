/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "via_drm.h"
#include "via_drv.h"

#include "drm_pciids.h"


static int dri_library_name(struct drm_device * dev, char * buf)
{
	return snprintf(buf, PAGE_SIZE, "unichrome\n");
}

static struct pci_device_id pciidlist[] = {
	viadrv_PCI_IDS
};


#ifdef VIA_HAVE_FENCE
extern struct drm_fence_driver via_fence_driver;
#endif

#ifdef VIA_HAVE_BUFFER

/**
 * If there's no thrashing. This is the preferred memory type order.
 */
static uint32_t via_mem_prios[] = {DRM_BO_MEM_PRIV0, DRM_BO_MEM_VRAM, DRM_BO_MEM_TT, DRM_BO_MEM_LOCAL};

/**
 * If we have thrashing, most memory will be evicted to TT anyway, so we might as well
 * just move the new buffer into TT from the start.
 */
static uint32_t via_busy_prios[] = {DRM_BO_MEM_TT, DRM_BO_MEM_PRIV0, DRM_BO_MEM_VRAM, DRM_BO_MEM_LOCAL};


static struct drm_bo_driver via_bo_driver = {
	.mem_type_prio = via_mem_prios,
	.mem_busy_prio = via_busy_prios,
	.num_mem_type_prio = DRM_ARRAY_SIZE(via_mem_prios),
	.num_mem_busy_prio = DRM_ARRAY_SIZE(via_busy_prios),
	.create_ttm_backend_entry = via_create_ttm_backend_entry,
	.fence_type = via_fence_types,
	.invalidate_caches = via_invalidate_caches,
	.init_mem_type = via_init_mem_type,
	.evict_flags = via_evict_flags,
	.move = NULL,
	.ttm_cache_flush = NULL,
	.command_stream_barrier = NULL
};
#endif

static int probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static struct drm_driver driver = {
	.driver_features =
	    DRIVER_USE_AGP | DRIVER_USE_MTRR | DRIVER_HAVE_IRQ |
	    DRIVER_IRQ_SHARED,
	.load = via_driver_load,
	.unload = via_driver_unload,
#ifndef VIA_HAVE_CORE_MM
	.context_ctor = via_init_context,
#endif
	.context_dtor = via_final_context,
	.get_vblank_counter = via_get_vblank_counter,
	.enable_vblank = via_enable_vblank,
	.disable_vblank = via_disable_vblank,
	.irq_preinstall = via_driver_irq_preinstall,
	.irq_postinstall = via_driver_irq_postinstall,
	.irq_uninstall = via_driver_irq_uninstall,
	.irq_handler = via_driver_irq_handler,
	.dma_quiescent = via_driver_dma_quiescent,
	.dri_library_name = dri_library_name,
	.reclaim_buffers = drm_core_reclaim_buffers,
	.reclaim_buffers_locked = NULL,
#ifdef VIA_HAVE_CORE_MM
	.reclaim_buffers_idlelocked = via_reclaim_buffers_locked,
	.lastclose = via_lastclose,
#endif
	.get_map_ofs = drm_core_get_map_ofs,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.ioctls = via_ioctls,
	.fops = {
		.owner = THIS_MODULE,
		.open = drm_open,
		.release = drm_release,
		.ioctl = drm_ioctl,
		.mmap = drm_mmap,
		.poll = drm_poll,
		.fasync = drm_fasync,
		},
	.pci_driver = {
		.name = DRIVER_NAME,
		.id_table = pciidlist,
		.probe = probe,
		.remove = __devexit_p(drm_cleanup_pci),
	},
#ifdef VIA_HAVE_FENCE
	.fence_driver = &via_fence_driver,
#endif
#ifdef VIA_HAVE_BUFFER
	.bo_driver = &via_bo_driver,
#endif
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = VIA_DRM_DRIVER_DATE,
	.major = VIA_DRM_DRIVER_MAJOR,
	.minor = VIA_DRM_DRIVER_MINOR,
	.patchlevel = VIA_DRM_DRIVER_PATCHLEVEL
};

static int probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	return drm_get_dev(pdev, ent, &driver);
}

static int __init via_init(void)
{
	driver.num_ioctls = via_max_ioctl;

	via_init_command_verifier();
	return drm_init(&driver, pciidlist);
}

static void __exit via_exit(void)
{
	drm_exit(&driver);
}

module_init(via_init);
module_exit(via_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");
