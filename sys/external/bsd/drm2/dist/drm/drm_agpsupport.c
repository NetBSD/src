/*	$NetBSD: drm_agpsupport.c,v 1.10 2018/08/28 03:41:38 riastradh Exp $	*/

/**
 * \file drm_agpsupport.c
 * DRM support for AGP/GART backend
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Gareth Hughes <gareth@valinux.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_agpsupport.c,v 1.10 2018/08/28 03:41:38 riastradh Exp $");

#include <drm/drmP.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "drm_legacy.h"

#include <asm/agp.h>

/**
 * Get AGP information.
 *
 * \param inode device inode.
 * \param file_priv DRM file private.
 * \param cmd command.
 * \param arg pointer to a (output) drm_agp_info structure.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device has been initialized and acquired and fills in the
 * drm_agp_info structure with the information in drm_agp_head::agp_info.
 */
static int drm_agp_info_hook(struct drm_device *dev, struct drm_agp_info *info)
{
	struct agp_kern_info *kern;

	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;

	kern = &dev->agp->agp_info;
#if __NetBSD__
	info->agp_version_major = 1;
	info->agp_version_minor = 0;
	info->mode = kern->aki_info.ai_mode;
	info->aperture_base = kern->aki_info.ai_aperture_base;
	info->aperture_size = kern->aki_info.ai_aperture_size;
	info->memory_allowed = kern->aki_info.ai_memory_allowed;
	info->memory_used = kern->aki_info.ai_memory_used;
	info->id_vendor = PCI_VENDOR(kern->aki_info.ai_devid);
	info->id_device = PCI_PRODUCT(kern->aki_info.ai_devid);
#else
	info->agp_version_major = kern->version.major;
	info->agp_version_minor = kern->version.minor;
	info->mode = kern->mode;
	info->aperture_base = kern->aper_base;
	info->aperture_size = kern->aper_size * 1024 * 1024;
	info->memory_allowed = kern->max_memory << PAGE_SHIFT;
	info->memory_used = kern->current_memory << PAGE_SHIFT;
	info->id_vendor = kern->device->vendor;
	info->id_device = kern->device->device;
#endif

	return 0;
}

EXPORT_SYMBOL(drm_agp_info);

static int drm_agp_info_ioctl_hook(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_agp_info *info = data;
	int err;

	err = drm_agp_info(dev, info);
	if (err)
		return err;

	return 0;
}

/**
 * Acquire the AGP device.
 *
 * \param dev DRM device that is to acquire AGP.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device hasn't been acquired before and calls
 * \c agp_backend_acquire.
 */
static int drm_agp_acquire_hook(struct drm_device * dev)
{
	if (!dev->agp)
		return -ENODEV;
	if (dev->agp->acquired)
		return -EBUSY;
	if (!(dev->agp->bridge = agp_backend_acquire(dev->pdev)))
		return -ENODEV;
	dev->agp->acquired = 1;
	return 0;
}

EXPORT_SYMBOL(drm_agp_acquire);

/**
 * Acquire the AGP device (ioctl).
 *
 * \param inode device inode.
 * \param file_priv DRM file private.
 * \param cmd command.
 * \param arg user argument.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device hasn't been acquired before and calls
 * \c agp_backend_acquire.
 */
static int drm_agp_acquire_ioctl_hook(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	return drm_agp_acquire((struct drm_device *) file_priv->minor->dev);
}

/**
 * Release the AGP device.
 *
 * \param dev DRM device that is to release AGP.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device has been acquired and calls \c agp_backend_release.
 */
static int drm_agp_release_hook(struct drm_device * dev)
{
	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;
	agp_backend_release(dev->agp->bridge);
	dev->agp->acquired = 0;
	return 0;
}
EXPORT_SYMBOL(drm_agp_release);

static int drm_agp_release_ioctl_hook(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	return drm_agp_release(dev);
}

/**
 * Enable the AGP bus.
 *
 * \param dev DRM device that has previously acquired AGP.
 * \param mode Requested AGP mode.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device has been acquired but not enabled, and calls
 * \c agp_enable.
 */
static int drm_agp_enable_hook(struct drm_device * dev, struct drm_agp_mode mode)
{
	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;

	dev->agp->mode = mode.mode;
	agp_enable(dev->agp->bridge, mode.mode);
	dev->agp->enabled = 1;
	return 0;
}

EXPORT_SYMBOL(drm_agp_enable);

static int drm_agp_enable_ioctl_hook(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	struct drm_agp_mode *mode = data;

	return drm_agp_enable(dev, *mode);
}

/**
 * Allocate AGP memory.
 *
 * \param inode device inode.
 * \param file_priv file private pointer.
 * \param cmd command.
 * \param arg pointer to a drm_agp_buffer structure.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device is present and has been acquired, allocates the
 * memory via agp_allocate_memory() and creates a drm_agp_mem entry for it.
 */
static int drm_agp_alloc_hook(struct drm_device *dev, struct drm_agp_buffer *request)
{
	struct drm_agp_mem *entry;
	struct agp_memory *memory;
	unsigned long pages;
	u32 type;

	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;
	if (!(entry = kzalloc(sizeof(*entry), GFP_KERNEL)))
		return -ENOMEM;

	pages = (request->size + AGP_PAGE_SIZE - 1) / AGP_PAGE_SIZE;
	type = (u32) request->type;
	if (!(memory = agp_allocate_memory(dev->agp->bridge, pages, type))) {
		kfree(entry);
		return -ENOMEM;
	}

#ifdef __NetBSD__
	/* I presume the `+ 1' is there to avoid an id of 0 or something.  */
	entry->handle = (unsigned long)memory->am_id + 1;
#else
	entry->handle = (unsigned long)memory->key + 1;
#endif
	entry->memory = memory;
	entry->bound = 0;
	entry->pages = pages;
	list_add(&entry->head, &dev->agp->memory);

	request->handle = entry->handle;
#ifdef __NetBSD__
	{
		struct agp_memory_info info;
		agp_memory_info(dev->agp->bridge, memory, &info);
		request->physical = info.ami_physical;
	}
#else
	request->physical = memory->physical;
#endif

	return 0;
}
EXPORT_SYMBOL(drm_agp_alloc);


static int drm_agp_alloc_ioctl_hook(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_agp_buffer *request = data;

	return drm_agp_alloc(dev, request);
}

/**
 * Search for the AGP memory entry associated with a handle.
 *
 * \param dev DRM device structure.
 * \param handle AGP memory handle.
 * \return pointer to the drm_agp_mem structure associated with \p handle.
 *
 * Walks through drm_agp_head::memory until finding a matching handle.
 */
static struct drm_agp_mem *drm_agp_lookup_entry(struct drm_device * dev,
					   unsigned long handle)
{
	struct drm_agp_mem *entry;

	list_for_each_entry(entry, &dev->agp->memory, head) {
		if (entry->handle == handle)
			return entry;
	}
	return NULL;
}

/**
 * Unbind AGP memory from the GATT (ioctl).
 *
 * \param inode device inode.
 * \param file_priv DRM file private.
 * \param cmd command.
 * \param arg pointer to a drm_agp_binding structure.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device is present and acquired, looks-up the AGP memory
 * entry and passes it to the unbind_agp() function.
 */
static int drm_agp_unbind_hook(struct drm_device *dev, struct drm_agp_binding *request)
{
	struct drm_agp_mem *entry;
	int ret;

	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;
	if (!(entry = drm_agp_lookup_entry(dev, request->handle)))
		return -EINVAL;
	if (!entry->bound)
		return -EINVAL;
#ifdef __NetBSD__
	ret = drm_unbind_agp(dev->agp->bridge, entry->memory);
#else
	ret = drm_unbind_agp(entry->memory);
#endif
	if (ret == 0)
		entry->bound = 0;
	return ret;
}
EXPORT_SYMBOL(drm_agp_unbind);


static int drm_agp_unbind_ioctl_hook(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	struct drm_agp_binding *request = data;

	return drm_agp_unbind(dev, request);
}

/**
 * Bind AGP memory into the GATT (ioctl)
 *
 * \param inode device inode.
 * \param file_priv DRM file private.
 * \param cmd command.
 * \param arg pointer to a drm_agp_binding structure.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device is present and has been acquired and that no memory
 * is currently bound into the GATT. Looks-up the AGP memory entry and passes
 * it to bind_agp() function.
 */
static int drm_agp_bind_hook(struct drm_device *dev, struct drm_agp_binding *request)
{
	struct drm_agp_mem *entry;
	int retcode;
	int page;

	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;
	if (!(entry = drm_agp_lookup_entry(dev, request->handle)))
		return -EINVAL;
	if (entry->bound)
		return -EINVAL;
	page = (request->offset + AGP_PAGE_SIZE - 1) / AGP_PAGE_SIZE;
#ifdef __NetBSD__
	if ((retcode = drm_bind_agp(dev->agp->bridge, entry->memory, page)))
		return retcode;
#else
	if ((retcode = drm_bind_agp(entry->memory, page)))
		return retcode;
#endif
	entry->bound = dev->agp->base + (page << PAGE_SHIFT);
	DRM_DEBUG("base = 0x%lx entry->bound = 0x%lx\n",
		  dev->agp->base, entry->bound);
	return 0;
}
EXPORT_SYMBOL(drm_agp_bind);


static int drm_agp_bind_ioctl_hook(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_agp_binding *request = data;

	return drm_agp_bind(dev, request);
}

/**
 * Free AGP memory (ioctl).
 *
 * \param inode device inode.
 * \param file_priv DRM file private.
 * \param cmd command.
 * \param arg pointer to a drm_agp_buffer structure.
 * \return zero on success or a negative number on failure.
 *
 * Verifies the AGP device is present and has been acquired and looks up the
 * AGP memory entry. If the memory it's currently bound, unbind it via
 * unbind_agp(). Frees it via free_agp() as well as the entry itself
 * and unlinks from the doubly linked list it's inserted in.
 */
static int drm_agp_free_hook(struct drm_device *dev, struct drm_agp_buffer *request)
{
	struct drm_agp_mem *entry;

	if (!dev->agp || !dev->agp->acquired)
		return -EINVAL;
	if (!(entry = drm_agp_lookup_entry(dev, request->handle)))
		return -EINVAL;
	if (entry->bound)
#ifdef __NetBSD__
		drm_unbind_agp(dev->agp->bridge, entry->memory);
#else
		drm_unbind_agp(entry->memory);
#endif

	list_del(&entry->head);

#ifdef __NetBSD__
	drm_free_agp(dev->agp->bridge, entry->memory, entry->pages);
#else
	drm_free_agp(entry->memory, entry->pages);
#endif
	kfree(entry);
	return 0;
}
EXPORT_SYMBOL(drm_agp_free);



static int drm_agp_free_ioctl_hook(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_agp_buffer *request = data;

	return drm_agp_free(dev, request);
}

/**
 * Initialize the AGP resources.
 *
 * \return pointer to a drm_agp_head structure.
 *
 * Gets the drm_agp_t structure which is made available by the agpgart module
 * via the inter_module_* functions. Creates and initializes a drm_agp_head
 * structure.
 *
 * Note that final cleanup of the kmalloced structure is directly done in
 * drm_pci_agp_destroy.
 */
static struct drm_agp_head *drm_agp_init_hook(struct drm_device *dev)
{
	struct drm_agp_head *head = NULL;

	if (!(head = kzalloc(sizeof(*head), GFP_KERNEL)))
		return NULL;
	head->bridge = agp_find_bridge(dev->pdev);
	if (!head->bridge) {
		if (!(head->bridge = agp_backend_acquire(dev->pdev))) {
			kfree(head);
			return NULL;
		}
		agp_copy_info(head->bridge, &head->agp_info);
		agp_backend_release(head->bridge);
	} else {
		agp_copy_info(head->bridge, &head->agp_info);
	}
#ifndef __NetBSD__
	/* Why would anything even attach in this case?  */
	if (head->agp_info.chipset == NOT_SUPPORTED) {
		kfree(head);
		return NULL;
	}
#endif
	INIT_LIST_HEAD(&head->memory);
#ifdef __NetBSD__
	head->cant_use_aperture = false; /* XXX */
	head->page_mask = ~0UL;
	head->base = head->agp_info.aki_info.ai_aperture_base;
#else
	head->cant_use_aperture = head->agp_info.cant_use_aperture;
	head->page_mask = head->agp_info.page_mask;
	head->base = head->agp_info.aper_base;
#endif
	return head;
}

/**
 * drm_agp_clear - Clear AGP resource list
 * @dev: DRM device
 *
 * Iterate over all AGP resources and remove them. But keep the AGP head
 * intact so it can still be used. It is safe to call this if AGP is disabled or
 * was already removed.
 *
 * If DRIVER_MODESET is active, nothing is done to protect the modesetting
 * resources from getting destroyed. Drivers are responsible of cleaning them up
 * during device shutdown.
 */
static void drm_agp_clear_hook(struct drm_device *dev)
{
	struct drm_agp_mem *entry, *tempe;

	if (!dev->agp)
		return;
	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return;

	list_for_each_entry_safe(entry, tempe, &dev->agp->memory, head) {
#ifdef __NetBSD__
		if (entry->bound)
			drm_unbind_agp(dev->agp->bridge, entry->memory);
		drm_free_agp(dev->agp->bridge, entry->memory, entry->pages);
#else
		if (entry->bound)
			drm_unbind_agp(entry->memory);
		drm_free_agp(entry->memory, entry->pages);
#endif
		kfree(entry);
	}
	INIT_LIST_HEAD(&dev->agp->memory);

	if (dev->agp->acquired)
		drm_agp_release(dev);

	dev->agp->acquired = 0;
	dev->agp->enabled = 0;
}

#ifndef __NetBSD__		/* XXX Dead code that doesn't make sense...  */
/**
 * Binds a collection of pages into AGP memory at the given offset, returning
 * the AGP memory structure containing them.
 *
 * No reference is held on the pages during this time -- it is up to the
 * caller to handle that.
 */
struct agp_memory *
drm_agp_bind_pages(struct drm_device *dev,
		   struct page **pages,
		   unsigned long num_pages,
		   uint32_t gtt_offset,
		   u32 type)
{
	struct agp_memory *mem;
	int ret, i;

	DRM_DEBUG("\n");

	mem = agp_allocate_memory(dev->agp->bridge, num_pages,
				      type);
	if (mem == NULL) {
		DRM_ERROR("Failed to allocate memory for %ld pages\n",
			  num_pages);
		return NULL;
	}

	for (i = 0; i < num_pages; i++)
		mem->pages[i] = pages[i];
	mem->page_count = num_pages;

	mem->is_flushed = true;
	ret = agp_bind_memory(mem, gtt_offset / AGP_PAGE_SIZE);
	if (ret != 0) {
		DRM_ERROR("Failed to bind AGP memory: %d\n", ret);
		agp_free_memory(mem);
		return NULL;
	}

	return mem;
}
EXPORT_SYMBOL(drm_agp_bind_pages);
#endif

#ifdef __NetBSD__

static void __pci_iomem *
drm_agp_borrow_hook(struct drm_device *dev, unsigned i, bus_size_t size)
{
	struct pci_dev *pdev = dev->pdev;

	if (!agp_i810_borrow(pdev->pd_resources[i].addr, size,
		&pdev->pd_resources[i].bsh))
		return NULL;
	/* XXX Synchronize with pci_iomap in linux_pci.c.  */
	pdev->pd_resources[i].bst = pdev->pd_pa.pa_memt;
	pdev->pd_resources[i].kva = bus_space_vaddr(pdev->pd_resources[i].bst,
	    pdev->pd_resources[i].bsh);
	pdev->pd_resources[i].mapped = true;

	return pdev->pd_resources[i].kva;
}

static void
drm_agp_flush_hook(void)
{

	agp_flush_cache();
}

static const struct drm_agp_hooks agp_hooks = {
	.agph_info = drm_agp_info_hook,
	.agph_info_ioctl = drm_agp_info_ioctl_hook,
	.agph_acquire = drm_agp_acquire_hook,
	.agph_acquire_ioctl = drm_agp_acquire_ioctl_hook,
	.agph_release = drm_agp_release_hook,
	.agph_release_ioctl = drm_agp_release_ioctl_hook,
	.agph_enable = drm_agp_enable_hook,
	.agph_enable_ioctl = drm_agp_enable_ioctl_hook,
	.agph_alloc = drm_agp_alloc_hook,
	.agph_alloc_ioctl = drm_agp_alloc_ioctl_hook,
	.agph_unbind = drm_agp_unbind_hook,
	.agph_unbind_ioctl = drm_agp_unbind_ioctl_hook,
	.agph_bind = drm_agp_bind_hook,
	.agph_bind_ioctl = drm_agp_bind_ioctl_hook,
	.agph_free = drm_agp_free_hook,
	.agph_free_ioctl = drm_agp_free_ioctl_hook,
	.agph_init = drm_agp_init_hook,
	.agph_clear = drm_agp_clear_hook,
	.agph_borrow = drm_agp_borrow_hook,
	.agph_flush = drm_agp_flush_hook,
};

#include <sys/module.h>
#include <sys/once.h>

MODULE(MODULE_CLASS_MISC, drmkms_agp, "drmkms"); /* XXX agp */

static int
drmkms_agp_init(void)
{

	return drm_agp_register(&agp_hooks);
}

int
drmkms_agp_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(drmkms_agp_init_once);

	return RUN_ONCE(&drmkms_agp_init_once, &drmkms_agp_init);
#endif
}

static int
drmkms_agp_fini(void)
{

	return drm_agp_deregister(&agp_hooks);
}

static int
drmkms_agp_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = drmkms_agp_init();
#else
		error = drmkms_agp_guarantee_initialized();
#endif
		if (error)
			return error;
		return 0;
	case MODULE_CMD_FINI:
		error = drmkms_agp_fini();
		if (error)
			return error;
		return 0;
	default:
		return ENOTTY;
	}
}

#endif	/* __NetBSD__ */
