/*-
 * Copyright 1999, 2000 Precision Insight, Inc., Cedar Park, Texas.
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
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

/** @file drm_drv.c
 * The catch-all file for DRM device support, including module setup/teardown,
 * open/close, and ioctl dispatch.
 */

#if defined(__FreeBSD__)
#include <sys/limits.h>
#elif   defined(__NetBSD__)
#include <sys/module.h>
#endif
#include "drmP.h"
#include "drm.h"
#include "drm_sarea.h"

#ifdef DRM_DEBUG_DEFAULT_ON
int drm_debug_flag = 1;
#else
int drm_debug_flag = 0;
#endif

static int drm_load(struct drm_device *dev);
static void drm_unload(struct drm_device *dev);
static drm_pci_id_list_t *drm_find_description(int vendor, int device,
    drm_pci_id_list_t *idlist);

#if defined(__FreeBSD__)
#define DRIVER_SOFTC(unit) \
	((struct drm_device *)devclass_get_softc(drm_devclass, unit))

MODULE_VERSION(drm, 1);
MODULE_DEPEND(drm, agp, 1, 1, 1);
MODULE_DEPEND(drm, pci, 1, 1, 1);
MODULE_DEPEND(drm, mem, 1, 1, 1);
#elif   defined(__NetBSD__)
#define DRIVER_SOFTC(kdev) drm_get_device_from_kdev(kdev)
#endif

static drm_ioctl_desc_t		  drm_ioctls[256] = {
	DRM_IOCTL_DEF(DRM_IOCTL_VERSION, drm_version, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_UNIQUE, drm_getunique, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_MAGIC, drm_getmagic, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_IRQ_BUSID, drm_irq_by_busid, DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_MAP, drm_getmap, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CLIENT, drm_getclient, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_STATS, drm_getstats, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_SET_VERSION, drm_setversion, DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_UNIQUE, drm_setunique, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_BLOCK, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_UNBLOCK, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AUTH_MAGIC, drm_authmagic, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_MAP, drm_addmap_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_MAP, drm_rmmap_ioctl, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_SAREA_CTX, drm_setsareactx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_SAREA_CTX, drm_getsareactx, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_CTX, drm_addctx, DRM_AUTH|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_CTX, drm_rmctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_MOD_CTX, drm_modctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CTX, drm_getctx, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_SWITCH_CTX, drm_switchctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_NEW_CTX, drm_newctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RES_CTX, drm_resctx, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_DRAW, drm_adddraw, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_DRAW, drm_rmdraw, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_LOCK, drm_lock, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_UNLOCK, drm_unlock, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_FINISH, drm_noop, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_BUFS, drm_addbufs, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_MARK_BUFS, drm_markbufs, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_INFO_BUFS, drm_infobufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_MAP_BUFS, drm_mapbufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_FREE_BUFS, drm_freebufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_DMA, drm_dma, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_CONTROL, drm_control, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ACQUIRE, drm_agp_acquire_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_RELEASE, drm_agp_release_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ENABLE, drm_agp_enable_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_INFO, drm_agp_info_ioctl, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ALLOC, drm_agp_alloc_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_FREE, drm_agp_free_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_BIND, drm_agp_bind_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_UNBIND, drm_agp_unbind_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_SG_ALLOC, drm_sg_alloc_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_SG_FREE, drm_sg_free, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_WAIT_VBLANK, drm_wait_vblank, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_MODESET_CTL, drm_modeset_ctl, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_UPDATE_DRAW, drm_update_draw, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
};

#if defined(__FreeBSD__)
static struct cdevsw drm_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	drm_open,
	.d_read =	drm_read,
	.d_ioctl =	drm_ioctl,
	.d_poll =	drm_poll,
	.d_mmap =	drm_mmap,
	.d_name =	"drm",
	.d_flags =	D_TRACKCLOSE
};

int drm_msi = 1;	/* Enable by default. */
TUNABLE_INT("hw.drm.msi", &drm_msi);

static struct drm_msi_blacklist_entry drm_msi_blacklist[] = {
	{0x8086, 0x2772}, /* Intel i945G	*/ \
	{0x8086, 0x27A2}, /* Intel i945GM	*/ \
	{0x8086, 0x27AE}, /* Intel i945GME	*/ \
	{0, 0}
};

static int drm_msi_is_blacklisted(int vendor, int device)
{
	int i = 0;
	
	for (i = 0; drm_msi_blacklist[i].vendor != 0; i++) {
		if ((drm_msi_blacklist[i].vendor == vendor) &&
		    (drm_msi_blacklist[i].device == device)) {
			return 1;
		}
	}

	return 0;
}

int drm_probe(device_t kdev, drm_pci_id_list_t *idlist)
{
	drm_pci_id_list_t *id_entry;
	int vendor, device;
#if __FreeBSD_version < 700010
	device_t realdev;

	if (!strcmp(device_get_name(kdev), "drmsub"))
		realdev = device_get_parent(kdev);
	else
		realdev = kdev;
	vendor = pci_get_vendor(realdev);
	device = pci_get_device(realdev);
#else
	vendor = pci_get_vendor(kdev);
	device = pci_get_device(kdev);
#endif

	if (pci_get_class(kdev) != PCIC_DISPLAY
	    || pci_get_subclass(kdev) != PCIS_DISPLAY_VGA)
		return ENXIO;

	id_entry = drm_find_description(vendor, device, idlist);
	if (id_entry != NULL) {
		if (!device_get_desc(kdev)) {
			DRM_DEBUG("desc : %s\n", device_get_desc(kdev));
			device_set_desc(kdev, id_entry->name);
		}
		return 0;
	}

	return ENXIO;
}

int drm_attach(device_t kdev, drm_pci_id_list_t *idlist)
{
	struct drm_device *dev;
	drm_pci_id_list_t *id_entry;
	int unit, msicount;

	unit = device_get_unit(kdev);
	dev = device_get_softc(kdev);

#if __FreeBSD_version < 700010
	if (!strcmp(device_get_name(kdev), "drmsub"))
		dev->device = device_get_parent(kdev);
	else
		dev->device = kdev;
#else
	dev->device = kdev;
#endif
	dev->devnode = make_dev(&drm_cdevsw,
			unit,
			DRM_DEV_UID,
			DRM_DEV_GID,
			DRM_DEV_MODE,
			"dri/card%d", unit);

#if __FreeBSD_version >= 700053
	dev->pci_domain = pci_get_domain(dev->device);
#else
	dev->pci_domain = 0;
#endif
	dev->pci_bus = pci_get_bus(dev->device);
	dev->pci_slot = pci_get_slot(dev->device);
	dev->pci_func = pci_get_function(dev->device);

	dev->pci_vendor = pci_get_vendor(dev->device);
	dev->pci_device = pci_get_device(dev->device);

	if (drm_msi &&
	    !drm_msi_is_blacklisted(dev->pci_vendor, dev->pci_device)) {
		msicount = pci_msi_count(dev->device);
		DRM_DEBUG("MSI count = %d\n", msicount);
		if (msicount > 1)
			msicount = 1;

		if (pci_alloc_msi(dev->device, &msicount) == 0) {
			DRM_INFO("MSI enabled %d message(s)\n", msicount);
			dev->msi_enabled = 1;
			dev->irqrid = 1;
		}
	}

	dev->irqr = bus_alloc_resource_any(dev->device, SYS_RES_IRQ,
	    &dev->irqrid, RF_SHAREABLE);
	if (!dev->irqr) {
		return ENOENT;
	}

	dev->irq = (int) rman_get_start(dev->irqr);

	mtx_init(&dev->dev_lock, "drmdev", NULL, MTX_DEF);
	mtx_init(&dev->irq_lock, "drmirq", NULL, MTX_DEF);
	mtx_init(&dev->vbl_lock, "drmvbl", NULL, MTX_DEF);
	mtx_init(&dev->drw_lock, "drmdrw", NULL, MTX_DEF);

	id_entry = drm_find_description(dev->pci_vendor,
	    dev->pci_device, idlist);
	dev->id_entry = id_entry;

	return drm_load(dev);
}

int drm_detach(device_t kdev)
{
	struct drm_device *dev;

	dev = device_get_softc(kdev);

	drm_unload(dev);

	bus_release_resource(dev->device, SYS_RES_IRQ, dev->irqrid, dev->irqr);

	if (dev->msi_enabled) {
		pci_release_msi(dev->device);
		DRM_INFO("MSI released\n");
	}

	return 0;
}

#ifndef DRM_DEV_NAME
#define DRM_DEV_NAME "drm"
#endif

devclass_t drm_devclass;

#elif   defined(__NetBSD__)

/*
 * Mapping from device minor to struct drm_device.
 * This is needed, because each driver (radeon, i915, etc.) has its own
 * _cd structure, so we can't just lookup from _cd with minor.
 */
struct drm_device *drm_units[DRM_MAXUNITS];

struct cdevsw drm_cdevsw = {
	drm_open,
	drm_close,
	drm_read,
	nowrite,
	drm_ioctl,
	nostop,
	notty,
	drm_poll,
	drm_mmap,
	nokqfilter,
	D_TTY
};

int drm_refcnt = 0;

int
drm_probe(struct pci_attach_args *pa, drm_pci_id_list_t *idlist)
{
	drm_pci_id_list_t *id_entry;

	id_entry = drm_find_description(PCI_VENDOR(pa->pa_id),
	    PCI_PRODUCT(pa->pa_id), idlist);
	if (id_entry != NULL) {
		return 1;
	}

	return 0;
}

void
drm_attach(device_t kdev, struct pci_attach_args *pa, drm_pci_id_list_t *idlist)
{
	struct drm_device *dev;
	int unit;

    unit = device_unit(kdev);
    if (unit < 0 || unit >= DRM_MAXUNITS)
        panic("drm_attach: device unit %d invalid", unit);
    if (drm_units[unit] != NULL)
        panic("drm_attach: unit %d already attached", unit);

	dev = device_private(kdev);
    dev->device = kdev;
    drm_units[unit] = dev;

	for (unit = 0; unit < DRM_MAX_PCI_RESOURCE; unit++)
	{
		dev->pci_map_data[unit].mapped = 0;
		dev->pci_map_data[unit].maptype =
			pci_mapreg_type(pa->pa_pc, pa->pa_tag,
				PCI_MAPREG_START + unit*4);
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag,
				PCI_MAPREG_START + unit*4,
				dev->pci_map_data[unit].maptype,
				&(dev->pci_map_data[unit].base),
				&(dev->pci_map_data[unit].size),
				&(dev->pci_map_data[unit].flags)))
		{
			DRM_ERROR("pci_mapreg info for %d failed\n",
				PCI_MAPREG_START + unit*4);
			dev->pci_map_data[unit].base = 0;
			dev->pci_map_data[unit].size = 0;
		}
		if (dev->pci_map_data[unit].maptype == PCI_MAPREG_TYPE_MEM)
			dev->pci_map_data[unit].flags |= BUS_SPACE_MAP_LINEAR;
		if (dev->pci_map_data[unit].maptype
                    == (PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT))
			dev->pci_map_data[unit].flags |= BUS_SPACE_MAP_LINEAR;
		DRM_DEBUG("pci resource %d: type=%d, base=%lx, size=%zx, flags=%x\n",
			unit, dev->pci_map_data[unit].maptype,
			dev->pci_map_data[unit].base,
			dev->pci_map_data[unit].size,
			dev->pci_map_data[unit].flags);
	}
	for (unit = 0; unit < DRM_MAX_PCI_RESOURCE; unit++) {
		dev->agp_map_data[unit].mapped = 0;
		dev->agp_map_data[unit].maptype = PCI_MAPREG_TYPE_MEM;
	}

	mutex_init(&dev->dev_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&dev->irq_lock, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&dev->vbl_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&dev->drw_lock, MUTEX_DEFAULT, IPL_NONE);

	memcpy(&dev->pa, pa, sizeof(dev->pa));

	dev->irq = pa->pa_intrline;
	/* XXX this needs to be deal with for other platforms, e.g. alpha */
	dev->pci_domain = 0;
	dev->pci_bus = pa->pa_bus;
	dev->pci_slot = pa->pa_device;
	dev->pci_func = pa->pa_function;
	dev->pci_vendor = PCI_VENDOR(pa->pa_id);
	dev->pci_device = PCI_PRODUCT(pa->pa_id);

	dev->id_entry = drm_find_description(PCI_VENDOR(pa->pa_id),
	    PCI_PRODUCT(pa->pa_id), idlist);

	aprint_normal(": %s\n", dev->id_entry->name);
	drm_load(dev);
}

int
drm_detach(device_t self, int flags)
{
	struct drm_device *dev = device_private(self);

	/* XXX locking */
	if (dev->open_count)
		return EBUSY;

	drm_unload(dev);

    drm_units[device_unit(self)] = NULL;

    free(dev->driver, DRM_MEM_DRIVER);

	return 0;
}

int
drm_activate(device_t self, devact_t act)
{
	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		/* FIXME */
		break;
	}
	return (0);
}

#endif

drm_pci_id_list_t *drm_find_description(int vendor, int device,
    drm_pci_id_list_t *idlist)
{
	int i = 0;
	
	for (i = 0; idlist[i].vendor != 0; i++) {
		if ((idlist[i].vendor == vendor) &&
		    ((idlist[i].device == device) ||
		    (idlist[i].device == 0))) {
			return &idlist[i];
		}
	}
	return NULL;
}

static int drm_firstopen(struct drm_device *dev)
{
	drm_local_map_t *map;
	int i;

	DRM_SPINLOCK_ASSERT(&dev->dev_lock);

	/* prebuild the SAREA */
	i = drm_addmap(dev, 0, SAREA_MAX, _DRM_SHM,
	    _DRM_CONTAINS_LOCK, &map);
	if (i != 0)
		return i;

	if (dev->driver->firstopen)
		dev->driver->firstopen(dev);

	dev->buf_use = 0;

	if (drm_core_check_feature(dev, DRIVER_HAVE_DMA)) {
		i = drm_dma_setup(dev);
		if (i != 0)
			return i;
	}

	for (i = 0; i < DRM_HASH_SIZE; i++) {
		dev->magiclist[i].head = NULL;
		dev->magiclist[i].tail = NULL;
	}

	dev->lock.lock_queue = 0;
	dev->irq_enabled = 0;
	dev->context_flag = 0;
	dev->last_context = 0;
	dev->if_version = 0;

#if defined(__FreeBSD__)
	dev->buf_sigio = NULL;
#elif   defined(__NetBSD__)
    dev->buf_pgid = 0;
#endif

	DRM_DEBUG("\n");

	return 0;
}

static int drm_lastclose(struct drm_device *dev)
{
	drm_magic_entry_t *pt, *next;
	drm_local_map_t *map, *mapsave;
	int i;

	DRM_SPINLOCK_ASSERT(&dev->dev_lock);

	DRM_DEBUG("\n");

	if (dev->driver->lastclose != NULL)
		dev->driver->lastclose(dev);

	if (dev->irq_enabled)
		drm_irq_uninstall(dev);

	if (dev->unique) {
		free(dev->unique, DRM_MEM_DRIVER);
		dev->unique = NULL;
		dev->unique_len = 0;
	}
	/* Clear pid list */
	for (i = 0; i < DRM_HASH_SIZE; i++) {
		for (pt = dev->magiclist[i].head; pt; pt = next) {
			next = pt->next;
			free(pt, DRM_MEM_MAGIC);
		}
		dev->magiclist[i].head = dev->magiclist[i].tail = NULL;
	}

	DRM_UNLOCK();
	drm_drawable_free_all(dev);
	DRM_LOCK();

	/* Clear AGP information */
	if (dev->agp) {
		drm_agp_mem_t *entry;
		drm_agp_mem_t *nexte;

		/* Remove AGP resources, but leave dev->agp intact until
		 * drm_unload is called.
		 */
		for (entry = dev->agp->memory; entry; entry = nexte) {
			nexte = entry->next;
			if (entry->bound)
				drm_agp_unbind_memory(entry->handle);
			drm_agp_free_memory(entry->handle);
			free(entry, DRM_MEM_AGPLISTS);
		}
		dev->agp->memory = NULL;

		if (dev->agp->acquired)
			drm_agp_release(dev);

		dev->agp->acquired = 0;
		dev->agp->enabled  = 0;
	}
	if (dev->sg != NULL) {
		drm_sg_cleanup(dev->sg);
		dev->sg = NULL;
	}

	TAILQ_FOREACH_SAFE(map, &dev->maplist, link, mapsave) {
		if (!(map->flags & _DRM_DRIVER))
			drm_rmmap(dev, map);
	}

	drm_dma_takedown(dev);
	if (dev->lock.hw_lock) {
		dev->lock.hw_lock = NULL; /* SHM removed */
		dev->lock.file_priv = NULL;
		DRM_WAKEUP_INT((void *)&dev->lock.lock_queue);
	}

	return 0;
}

static int drm_load(struct drm_device *dev)
{
	int i, retcode;

	DRM_DEBUG("\n");

	TAILQ_INIT(&dev->maplist);

	drm_mem_init();
	drm_sysctl_init(dev);
	TAILQ_INIT(&dev->files);

	dev->counters  = 6;
	dev->types[0]  = _DRM_STAT_LOCK;
	dev->types[1]  = _DRM_STAT_OPENS;
	dev->types[2]  = _DRM_STAT_CLOSES;
	dev->types[3]  = _DRM_STAT_IOCTLS;
	dev->types[4]  = _DRM_STAT_LOCKS;
	dev->types[5]  = _DRM_STAT_UNLOCKS;

	for (i = 0; i < DRM_ARRAY_SIZE(dev->counts); i++)
		atomic_set(&dev->counts[i], 0);

	if (dev->driver->load != NULL) {
		DRM_LOCK();
		/* Shared code returns -errno. */
		retcode = -dev->driver->load(dev,
		    dev->id_entry->driver_private);
#if defined(__FreeBSD__)
		if (pci_enable_busmaster(dev->device))
			DRM_ERROR("Request to enable bus-master failed.\n");
#endif
		DRM_UNLOCK();
		if (retcode != 0)
			goto error;
	}

	if (drm_core_has_AGP(dev)) {
		if (drm_device_is_agp(dev))
			dev->agp = drm_agp_init(dev);
		if (drm_core_check_feature(dev, DRIVER_REQUIRE_AGP) &&
		    dev->agp == NULL) {
			DRM_ERROR("Card isn't AGP, or couldn't initialize "
			    "AGP.\n");
			retcode = ENOMEM;
			goto error;
		}
		if (dev->agp != NULL) {
			if (drm_mtrr_add(dev->agp->info.ai_aperture_base,
			    dev->agp->info.ai_aperture_size, DRM_MTRR_WC) == 0)
				dev->agp->mtrr = 1;
		}
	}

	retcode = drm_ctxbitmap_init(dev);
	if (retcode != 0) {
		DRM_ERROR("Cannot allocate memory for context bitmap.\n");
		goto error;
	}

#if defined(__FreeBSD__)
	dev->drw_unrhdr = new_unrhdr(1, INT_MAX, NULL);
	if (dev->drw_unrhdr == NULL) {
		DRM_ERROR("Couldn't allocate drawable number allocator\n");
		goto error;
	}
#else
    dev->drw_no = 0;
#endif

#if defined(__NetBSD__)
    aprint_normal_dev(dev->device,
#else
	DRM_INFO(
#endif
        "Initialized %s %d.%d.%d %s\n",
	    dev->driver->name,
	    dev->driver->major,
	    dev->driver->minor,
	    dev->driver->patchlevel,
	    dev->driver->date);

	return 0;

error:
#if defined(__FreeBSD__)
	drm_sysctl_cleanup(dev);
#endif
	DRM_LOCK();
	drm_lastclose(dev);
	DRM_UNLOCK();
#if defined(__FreeBSD__)
	destroy_dev(dev->devnode);
#endif

	DRM_SPINUNINIT(&dev->drw_lock);
	DRM_SPINUNINIT(&dev->vbl_lock);
	DRM_SPINUNINIT(&dev->irq_lock);
	DRM_SPINUNINIT(&dev->dev_lock);

	return retcode;
}

static void drm_unload(struct drm_device *dev)
{
#if defined(__FreeBSD__)
	int i;
#endif

	DRM_DEBUG("\n");

#if defined(__FreeBSD__)
	drm_sysctl_cleanup(dev);
	destroy_dev(dev->devnode);
#endif

	drm_ctxbitmap_cleanup(dev);

	if (dev->agp && dev->agp->mtrr) {
		int __unused retcode;

		retcode = drm_mtrr_del(
#if defined(__FreeBSD__)
            0,
#endif
            dev->agp->info.ai_aperture_base,
		    dev->agp->info.ai_aperture_size, DRM_MTRR_WC);
		DRM_DEBUG("mtrr_del = %d", retcode);
	}

	DRM_LOCK();
	drm_lastclose(dev);
	DRM_UNLOCK();

	drm_vblank_cleanup(dev);

#if defined(__FreeBSD__)
	/* Clean up PCI resources allocated by drm_bufs.c.  We're not really
	 * worried about resource consumption while the DRM is inactive (between
	 * lastclose and firstopen or unload) because these aren't actually
	 * taking up KVA, just keeping the PCI resource allocated.
	 */
	for (i = 0; i < DRM_MAX_PCI_RESOURCE; i++) {
		if (dev->pcir[i] == NULL)
			continue;
		bus_release_resource(dev->device, SYS_RES_MEMORY,
		    dev->pcirid[i], dev->pcir[i]);
		dev->pcir[i] = NULL;
	}
#endif

	if (dev->agp) {
		free(dev->agp, DRM_MEM_AGPLISTS);
		dev->agp = NULL;
	}

	if (dev->driver->unload != NULL) {
		DRM_LOCK();
		dev->driver->unload(dev);
		DRM_UNLOCK();
	}

#if defined(__FreeBSD__)
	delete_unrhdr(dev->drw_unrhdr);
#endif

	drm_mem_uninit();

#if defined(__FreeBSD__)
	if (pci_disable_busmaster(dev->device))
		DRM_ERROR("Request to disable bus-master failed.\n");
#endif

	DRM_SPINUNINIT(&dev->drw_lock);
	DRM_SPINUNINIT(&dev->vbl_lock);
	DRM_SPINUNINIT(&dev->irq_lock);
	DRM_SPINUNINIT(&dev->dev_lock);
}

int drm_version(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_version *drmvers = data;
	int len;

#define DRM_COPY( name, value )						\
	len = strlen( value );						\
	if ( len > name##_len ) len = name##_len;			\
	name##_len = strlen( value );					\
	if ( len && name ) {						\
		if ( DRM_COPY_TO_USER( name, value, len ) )		\
			return EFAULT;				\
	}

	drmvers->version_major		= dev->driver->major;
	drmvers->version_minor		= dev->driver->minor;
	drmvers->version_patchlevel	= dev->driver->patchlevel;

	DRM_COPY(drmvers->name, dev->driver->name);
	DRM_COPY(drmvers->date, dev->driver->date);
	DRM_COPY(drmvers->desc, dev->driver->desc);

	return 0;
}

int drm_open(DRM_CDEV kdev, int flags, int fmt, DRM_STRUCTCDEVPROC *p)
{
	struct drm_device *dev = NULL;
	int retcode = 0;

#if defined(__FreeBSD__)
	dev = DRIVER_SOFTC(dev2unit(kdev));
#elif   defined(__NetBSD__)
	dev = DRIVER_SOFTC(kdev);
    if (dev == NULL) {
        return ENXIO;
    }
    dev->kdev = kdev;
#endif

	DRM_DEBUG("open_count = %d\n", dev->open_count);

	retcode = drm_open_helper(kdev, flags, fmt, DRM_CDEVPROC_REAL(p), dev);

	if (!retcode) {
		atomic_inc(&dev->counts[_DRM_STAT_OPENS]);
		DRM_LOCK();
#if defined(__FreeBSD__)
		device_busy(dev->device);
#endif
		if (!dev->open_count++)
			retcode = drm_firstopen(dev);
		DRM_UNLOCK();
	}

	return retcode;
}

#if defined(__FreeBSD__)
void drm_close(void *data)
#elif   defined(__NetBSD__)
int
drm_close(dev_t kdev, int flags, int fmt, struct lwp *l)
#endif
{
	struct drm_file *file_priv;
	struct drm_device *dev;
	int retcode = 0;

#if defined(__FreeBSD__)
    file_priv = data;
    dev = file_priv->dev;
#elif   defined(__NetBSD__)
    dev = DRIVER_SOFTC(kdev);
#endif

	DRM_DEBUG("open_count = %d\n", dev->open_count);

	DRM_LOCK();

#if defined(__NetBSD__)
    file_priv = drm_find_file_by_proc(dev, l->l_proc);
    if (file_priv == NULL) {
        DRM_UNLOCK();
        DRM_ERROR("can't find authenticator\n");
        return EINVAL;
    }
#endif

	if (dev->driver->preclose != NULL)
		dev->driver->preclose(dev, file_priv);

	/* ========================================================
	 * Begin inline drm_release
	 */

#if defined(__FreeBSD__)
	DRM_DEBUG("pid = %d, device = 0x%lx, open_count = %d\n",
	    DRM_CURRENTPID, (long)dev->device, dev->open_count);
#elif   defined(__NetBSD__)
	DRM_DEBUG("pid = %d, device = 0x%p, open_count = %d\n",
	    DRM_CURRENTPID, dev->device, dev->open_count);
#endif

	if (dev->lock.hw_lock && _DRM_LOCK_IS_HELD(dev->lock.hw_lock->lock)
	    && dev->lock.file_priv == file_priv) {
		DRM_DEBUG("Process %d dead, freeing lock for context %d\n",
			  DRM_CURRENTPID,
			  _DRM_LOCKING_CONTEXT(dev->lock.hw_lock->lock));
		if (dev->driver->reclaim_buffers_locked != NULL)
			dev->driver->reclaim_buffers_locked(dev, file_priv);

		drm_lock_free(&dev->lock,
		    _DRM_LOCKING_CONTEXT(dev->lock.hw_lock->lock));
		
				/* FIXME: may require heavy-handed reset of
                                   hardware at this point, possibly
                                   processed via a callback to the X
                                   server. */
	} else if (dev->driver->reclaim_buffers_locked != NULL &&
	    dev->lock.hw_lock != NULL) {
		/* The lock is required to reclaim buffers */
		for (;;) {
			if (!dev->lock.hw_lock) {
				/* Device has been unregistered */
				retcode = EINTR;
				break;
			}
			if (drm_lock_take(&dev->lock, DRM_KERNEL_CONTEXT)) {
				dev->lock.file_priv = file_priv;
				dev->lock.lock_time = jiffies;
				atomic_inc(&dev->counts[_DRM_STAT_LOCKS]);
				break;	/* Got lock */
			}
			/* Contention */
#if defined(__FreeBSD__)
			retcode = mtx_sleep((void *)&dev->lock.lock_queue,
			    &dev->dev_lock, PCATCH, "drmlk2", 0);
#elif   defined(__NetBSD__)
			retcode = mtsleep((void *)&dev->lock.lock_queue,
			    PCATCH, "drmlk2", 0, &dev->dev_lock);
#endif
			if (retcode)
				break;
		}
		if (retcode == 0) {
			dev->driver->reclaim_buffers_locked(dev, file_priv);
			drm_lock_free(&dev->lock, DRM_KERNEL_CONTEXT);
		}
	}

	if (drm_core_check_feature(dev, DRIVER_HAVE_DMA) &&
	    !dev->driver->reclaim_buffers_locked)
		drm_reclaim_buffers(dev, file_priv);

#if defined(__FreeBSD__)
	funsetown(&dev->buf_sigio);
#else
    dev->buf_pgid = 0;
#endif

	if (dev->driver->postclose != NULL)
		dev->driver->postclose(dev, file_priv);
	TAILQ_REMOVE(&dev->files, file_priv, link);
	free(file_priv, DRM_MEM_FILES);

	/* ========================================================
	 * End inline drm_release
	 */

	atomic_inc(&dev->counts[_DRM_STAT_CLOSES]);
#if defined(__FreeBSD__)
	device_unbusy(dev->device);
#endif
#if defined(__NetBSD__)
	/* On NetBSD, close will only be called once */
	DRM_DEBUG("setting open_count %d to 1\n", (int)dev->open_count);
	dev->open_count = 1;
#endif
	if (--dev->open_count == 0) {
		retcode = drm_lastclose(dev);
	}

	DRM_UNLOCK();

#if defined(__NetBSD__)
    return retcode;
#endif
}

/* drm_ioctl is called whenever a process performs an ioctl on /dev/drm.
 */
int drm_ioctl(DRM_CDEV kdev, u_long cmd, DRM_IOCTL_DATA data, int flags, 
    DRM_STRUCTCDEVPROC *p)
{
	struct drm_device *dev = drm_get_device_from_kdev(kdev);
	int retcode = 0;
	drm_ioctl_desc_t *ioctl;
	int (*func)(struct drm_device *dev, void *data, struct drm_file *file_priv);
	int nr = DRM_IOCTL_NR(cmd);
	int is_driver_ioctl = 0;
	struct drm_file *file_priv;

#if defined(__FreeBSD__)
	retcode = devfs_get_cdevpriv((void **)&file_priv);
	if (retcode != 0) {
		DRM_ERROR("can't find authenticator\n");
		return EINVAL;
	}
#elif   defined(__NetBSD__)
    DRM_LOCK();
    file_priv = drm_find_file_by_proc(dev, p->l_proc);
    DRM_UNLOCK();
    if (file_priv == NULL) {
		DRM_ERROR("can't find authenticator\n");
		return EINVAL;
	}
#endif

	atomic_inc(&dev->counts[_DRM_STAT_IOCTLS]);
	++file_priv->ioctl_count;

#if defined(__FreeBSD__)
	DRM_DEBUG("pid=%d, cmd=0x%02lx, nr=0x%02x, dev 0x%lx, auth=%d\n",
	    DRM_CURRENTPID, cmd, nr, (long)dev->device,
	    file_priv->authenticated);
#elif   defined(__NetBSD__)
	DRM_DEBUG("pid=%d, cmd=0x%02lx, nr=0x%02x, dev %p, auth=%d\n",
	    DRM_CURRENTPID, cmd, nr, dev->device,
	    file_priv->authenticated);
#endif

	switch (cmd) {
	case FIONBIO:
	case FIOASYNC:
		return 0;

#if defined(__FreeBSD__)
	case FIOSETOWN:
		return fsetown(*(int *)data, &dev->buf_sigio);

	case FIOGETOWN:
		*(int *) data = fgetown(&dev->buf_sigio);
		return 0;
#endif
#if defined(__NetBSD__)
	case SIOCSPGRP:
	case TIOCSPGRP:
	case FIOSETOWN:
		return fsetown(&dev->buf_pgid, cmd, data);

	case SIOCGPGRP:
	case TIOCGPGRP:
	case FIOGETOWN:
		return fgetown(dev->buf_pgid, cmd, data);
#endif
	}

	if (IOCGROUP(cmd) != DRM_IOCTL_BASE) {
		DRM_DEBUG("Bad ioctl group 0x%x\n", (int)IOCGROUP(cmd));
		return EINVAL;
	}

	ioctl = &drm_ioctls[nr];
	/* It's not a core DRM ioctl, try driver-specific. */
	if (ioctl->func == NULL && nr >= DRM_COMMAND_BASE) {
		/* The array entries begin at DRM_COMMAND_BASE ioctl nr */
		nr -= DRM_COMMAND_BASE;
		if (nr > dev->driver->max_ioctl) {
			DRM_DEBUG("Bad driver ioctl number, 0x%x (of 0x%x)\n",
			    nr, dev->driver->max_ioctl);
			return EINVAL;
		}
		ioctl = &dev->driver->ioctls[nr];
		is_driver_ioctl = 1;
	}
	func = ioctl->func;

	if (func == NULL) {
		DRM_DEBUG("no function\n");
		return EINVAL;
	}

	if (((ioctl->flags & DRM_ROOT_ONLY) && !DRM_SUSER(DRM_CDEVPROC_REAL(p))) ||
	    ((ioctl->flags & DRM_AUTH) && !file_priv->authenticated) ||
	    ((ioctl->flags & DRM_MASTER) && !file_priv->master))
		return EACCES;

	if (is_driver_ioctl) {
		DRM_LOCK();
		/* shared code returns -errno */
		retcode = -func(dev, data, file_priv);
		DRM_UNLOCK();
	} else {
		retcode = func(dev, data, file_priv);
	}

	if (retcode != 0)
		DRM_DEBUG("    returning %d\n", retcode);

	return retcode;
}

drm_local_map_t *drm_getsarea(struct drm_device *dev)
{
	drm_local_map_t *map;

	DRM_SPINLOCK_ASSERT(&dev->dev_lock);
	TAILQ_FOREACH(map, &dev->maplist, link) {
		if (map->type == _DRM_SHM && (map->flags & _DRM_CONTAINS_LOCK))
			return map;
	}

	return NULL;
}

#if DRM_LINUX

#include <sys/sysproto.h>

MODULE_DEPEND(DRIVER_NAME, linux, 1, 1, 1);

#define LINUX_IOCTL_DRM_MIN		0x6400
#define LINUX_IOCTL_DRM_MAX		0x64ff

static linux_ioctl_function_t drm_linux_ioctl;
static struct linux_ioctl_handler drm_handler = {drm_linux_ioctl, 
    LINUX_IOCTL_DRM_MIN, LINUX_IOCTL_DRM_MAX};

SYSINIT(drm_register, SI_SUB_KLD, SI_ORDER_MIDDLE, 
    linux_ioctl_register_handler, &drm_handler);
SYSUNINIT(drm_unregister, SI_SUB_KLD, SI_ORDER_MIDDLE, 
    linux_ioctl_unregister_handler, &drm_handler);

/* The bits for in/out are switched on Linux */
#define LINUX_IOC_IN	IOC_OUT
#define LINUX_IOC_OUT	IOC_IN

static int
drm_linux_ioctl(DRM_STRUCTPROC *p, struct linux_ioctl_args* args)
{
	int error;
	int cmd = args->cmd;

	args->cmd &= ~(LINUX_IOC_IN | LINUX_IOC_OUT);
	if (cmd & LINUX_IOC_IN)
		args->cmd |= IOC_IN;
	if (cmd & LINUX_IOC_OUT)
		args->cmd |= IOC_OUT;
	
	error = ioctl(p, (struct ioctl_args *)args);

	return error;
}
#endif /* DRM_LINUX */

#if defined(__NetBSD__)
/* Module support */

MODULE(MODULE_CLASS_MISC, drm, NULL);

static int
drm_modcmd(modcmd_t cmd, void *arg)
{
#ifdef _MODULE
	int bmajor = -1, cmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		return devsw_attach("drm", NULL, &bmajor, &drm_cdevsw, &cmajor);
	case MODULE_CMD_FINI:
		devsw_detach(NULL, &drm_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
#else
	if (cmd == MODULE_CMD_INIT)
		return 0;
	return ENOTTY;
#endif
}

#endif /* __NetBSD__ */
