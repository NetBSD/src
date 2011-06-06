/* via_drv.c -- VIA unichrome driver -*- linux-c -*-
 * Created: Fri Aug 12 2005 by anholt@FreeBSD.org
 */
/*-
 * Copyright 2005 Eric Anholt
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
 *
 */

#include "drmP.h"
#include "drm.h"
#include "via_drm.h"
#include "via_drv.h"
#include "drm_pciids.h"

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t via_pciidlist[] = {
	viadrv_PCI_IDS
};

static void
via_configure(struct drm_device *dev)
{
	dev->driver->driver_features =
	    DRIVER_USE_AGP | DRIVER_USE_MTRR | DRIVER_HAVE_IRQ;

	dev->driver->buf_priv_size	= sizeof(drm_via_private_t);
	dev->driver->load		= via_driver_load;
	dev->driver->unload		= via_driver_unload;
	dev->driver->lastclose		= via_lastclose;
	dev->driver->get_vblank_counter	= via_get_vblank_counter;
	dev->driver->enable_vblank	= via_enable_vblank;
	dev->driver->disable_vblank	= via_disable_vblank;
	dev->driver->irq_preinstall	= via_driver_irq_preinstall;
	dev->driver->irq_postinstall	= via_driver_irq_postinstall;
	dev->driver->irq_uninstall	= via_driver_irq_uninstall;
	dev->driver->irq_handler	= via_driver_irq_handler;
	dev->driver->dma_quiescent	= via_driver_dma_quiescent;

	dev->driver->ioctls		= via_ioctls;
	dev->driver->max_ioctl		= via_max_ioctl;

	dev->driver->name		= DRIVER_NAME;
	dev->driver->desc		= DRIVER_DESC;
	dev->driver->date		= DRIVER_DATE;
	dev->driver->major		= DRIVER_MAJOR;
	dev->driver->minor		= DRIVER_MINOR;
	dev->driver->patchlevel		= DRIVER_PATCHLEVEL;
}

static int
viadrm_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	return drm_probe(pa, via_pciidlist);
}

static void
viadrm_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct drm_device *dev = device_private(self);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	via_configure(dev);

	drm_attach(self, pa, via_pciidlist);
}

static int
viadrm_detach(device_t self, int flags)
{
	pmf_device_deregister(self);

	return drm_detach(self, flags);
}

CFATTACH_DECL_NEW(viadrm, sizeof(struct drm_device),
    viadrm_probe, viadrm_attach, viadrm_detach, NULL);

MODULE(MODULE_CLASS_DRIVER, viadrm, "drm");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
viadrm_modcmd(modcmd_t cmd, void *arg)
{
	int err = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		err = config_init_component(cfdriver_ioconf_viadrm,
		    cfattach_ioconf_viadrm, cfdata_ioconf_viadrm);
#endif
		return err;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		err = config_fini_component(cfdriver_ioconf_viadrm,
		    cfattach_ioconf_viadrm, cfdata_ioconf_viadrm);
#endif
		return err;
	default:
		return ENOTTY;
	}
}
