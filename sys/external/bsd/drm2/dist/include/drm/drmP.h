/*	$NetBSD: drmP.h,v 1.61 2021/12/19 09:51:34 riastradh Exp $	*/

/*
 * Internal Header for the Direct Rendering Manager
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * Copyright (c) 2009-2010, Code Aurora Forum.
 * All rights reserved.
 *
 * Author: Rickard E. (Rik) Faith <faith@valinux.com>
 * Author: Gareth Hughes <gareth@valinux.com>
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

#ifndef _DRM_P_H_
#define _DRM_P_H_

#include <linux/agp_backend.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/ratelimit.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#include <asm/mman.h>
#include <asm/pgalloc.h>
#include <asm/uaccess.h>

#include <uapi/drm/drm.h>
#include <uapi/drm/drm_mode.h>

#ifdef __NetBSD__
#include <drm/drm_os_netbsd.h>
#include <asm/barrier.h>
#include <asm/bug.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fence.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/uidgid.h>
#else
#include <drm/drm_os_linux.h>
#endif

#include <drm/drm_agpsupport.h>
#include <drm/drm_crtc.h>
#include <drm/drm_global.h>
#include <drm/drm_hashtab.h>
#include <drm/drm_mem_util.h>
#include <drm/drm_mm.h>
#include <drm/drm_sarea.h>
#include <drm/drm_vma_manager.h>

struct module;

struct drm_file;
struct drm_device;
struct drm_agp_head;
struct drm_local_map;
struct drm_device_dma;
struct drm_dma_handle;
struct drm_bus_irq_cookie;

struct device_node;
struct videomode;
struct reservation_object;
struct dma_buf_attachment;

/***********************************************************************/
/** \name Internal types and structures */
/*@{*/

#define DRM_IF_VERSION(maj, min) (maj << 16 | min)

#ifdef __NetBSD__		/* XXX debugfs */
struct seq_file;
#endif

/**
 * Info file list entry. This structure represents a debugfs or proc file to
 * be created by the drm core
 */
struct drm_info_list {
	const char *name; /** file name */
	int (*show)(struct seq_file*, void*); /** show callback */
	u32 driver_features; /**< Required driver features for this entry */
	void *data;
};

/**
 * debugfs node structure. This structure represents a debugfs file.
 */
struct drm_info_node {
	struct list_head list;
	struct drm_minor *minor;
	const struct drm_info_list *info_ent;
	struct dentry *dent;
};


/******************************************************************/
/** \name Internal function definitions */
/*@{*/

				/* Device support (drm_fops.h) */
extern int drm_new_set_master(struct drm_device *dev, struct drm_file *fpriv);

				/* IRQ support (drm_irq.h) */
#ifdef __NetBSD__
extern int drm_irq_install(struct drm_device *dev);
#else
extern int drm_irq_install(struct drm_device *dev, int irq);
#endif
extern int drm_irq_uninstall(struct drm_device *dev);

				/* Stub support (drm_stub.h) */
extern struct drm_master *drm_master_get(struct drm_master *master);
extern void drm_master_put(struct drm_master **master);

extern void drm_put_dev(struct drm_device *dev);
extern void drm_unplug_dev(struct drm_device *dev);
extern unsigned int drm_debug;
extern bool drm_atomic;

int drm_pci_set_unique(struct drm_device *dev,
		       struct drm_master *master,
		       struct drm_unique *u);
extern struct drm_dma_handle *drm_pci_alloc(struct drm_device *dev, size_t size,
					    size_t align);
extern void drm_pci_free(struct drm_device *dev, struct drm_dma_handle * dmah);

#ifdef __NetBSD__
int drm_limit_dma_space(struct drm_device *, resource_size_t, resource_size_t);
int drm_guarantee_initialized(void);
#endif

/*@}*/

/* PCI section */
void drm_pci_agp_destroy(struct drm_device *dev);

extern int drm_pci_init(struct drm_driver *driver, struct pci_driver *pdriver);
extern void drm_pci_exit(struct drm_driver *driver, struct pci_driver *pdriver);
#ifdef __NetBSD__
int drm_pci_request_irq(struct drm_device *, int);
void drm_pci_free_irq(struct drm_device *);
extern int drm_pci_attach(device_t, const struct pci_attach_args *,
    struct pci_dev *, struct drm_driver *, unsigned long,
    struct drm_device **);
extern int drm_pci_detach(struct drm_device *, int);
#endif

extern int drm_pcie_get_speed_cap_mask(struct drm_device *dev, u32 *speed_mask);

/* platform section */
extern int drm_platform_init(struct drm_driver *driver, struct platform_device *platform_device);
extern int drm_platform_set_busid(struct drm_device *d, struct drm_master *m);

#ifdef __NetBSD__

/* XXX This is pretty kludgerific.  */

#include <linux/io-mapping.h>

static inline struct io_mapping *
drm_io_mapping_create_wc(struct drm_device *dev, resource_size_t addr,
    unsigned long size)
{
	return bus_space_io_mapping_create_wc(dev->bst, addr, size);
}

static inline bool
drm_io_mapping_init_wc(struct drm_device *dev, struct io_mapping *mapping,
    resource_size_t addr, unsigned long size)
{
	return bus_space_io_mapping_init_wc(dev->bst, mapping, addr, size);
}

#endif	/* defined(__NetBSD__) */

#ifdef __NetBSD__
extern const struct cdevsw drm_cdevsw;
#endif

#endif
