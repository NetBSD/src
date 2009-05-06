/* $NetBSD: via_drv.c,v 1.5 2009/05/06 09:25:17 cegger Exp $ */

/* via_drv.c -- VIA unichrome driver -*- linux-c -*-
 * Created: Fri Aug 12 2005 by anholt@FreeBSD.org
 */
/*-
 * Copyright 2005 Eric Anholt
 * Copyright 2007 Jared D. McNeill
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
 * ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *    Jared D. McNeill <jmcneill@NetBSD.org>
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: via_drv.c,v 1.5 2009/05/06 09:25:17 cegger Exp $");

#include "drmP.h"
#include "drm.h"
#include "via_drm.h"
#include "via_drv.h"
#include "drm_pciids.h"

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t via_pciidlist[] = {
	viadrv_PCI_IDS
};

static void viadrm_configure(drm_device_t *dev)
{
	dev->driver.buf_priv_size	= 1;
	dev->driver.load		= via_driver_load;
	dev->driver.unload		= via_driver_unload;
	dev->driver.context_ctor	= via_init_context;
	dev->driver.context_dtor	= via_final_context;
	dev->driver.vblank_wait		= via_driver_vblank_wait;
	dev->driver.irq_preinstall	= via_driver_irq_preinstall;
	dev->driver.irq_postinstall	= via_driver_irq_postinstall;
	dev->driver.irq_uninstall	= via_driver_irq_uninstall;
	dev->driver.irq_handler		= via_driver_irq_handler;
	dev->driver.dma_quiescent	= via_driver_dma_quiescent;

	dev->driver.ioctls		= via_ioctls;
	dev->driver.max_ioctl		= via_max_ioctl;

	dev->driver.name		= DRIVER_NAME;
	dev->driver.desc		= DRIVER_DESC;
	dev->driver.date		= VIA_DRM_DRIVER_DATE;
	dev->driver.major		= VIA_DRM_DRIVER_MAJOR;
	dev->driver.minor		= VIA_DRM_DRIVER_MINOR;
	dev->driver.patchlevel		= VIA_DRM_DRIVER_PATCHLEVEL;

	dev->driver.use_agp		= 1;
	dev->driver.use_mtrr		= 1;
	dev->driver.use_irq		= 1;
	dev->driver.use_vbl_irq		= 1;
}

static int
viadrm_probe(struct device *parent, cfdata_t match, void *opaque)
{
	struct pci_attach_args *pa = opaque;
	return drm_probe(pa, via_pciidlist);
}

static void
viadrm_attach(struct device *parent, struct device *self, void *opaque)
{
	struct pci_attach_args *pa = opaque;
	drm_device_t *dev = device_private(self);

	viadrm_configure(dev);
	drm_attach(self, pa, via_pciidlist);
}

CFATTACH_DECL_NEW(viadrm, sizeof(drm_device_t), viadrm_probe, viadrm_attach,
    drm_detach, drm_activate);
