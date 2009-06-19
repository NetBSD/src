/* nouveau_drv.c.c -- nouveau nouveau driver -*- linux-c -*-
 * Created: Wed Feb 14 17:10:04 2001 by gareth@valinux.com
 */
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"
#include "drm_pciids.h"

extern struct drm_ioctl_desc nouveau_ioctls[];
extern int nouveau_max_ioctl;

/* drv_PCI_IDs for nouveau is just to match the vendor id */
static struct drm_pci_id_list nouveau_pciidlist[] = {
	{0x10DE, 0, 0, "NVidia Display Adapter"}, \
	{0, 0, 0, NULL}
};

static void nouveau_configure(struct drm_device *dev)
{
	dev->driver->driver_features =
	   DRIVER_USE_AGP | DRIVER_PCI_DMA | DRIVER_SG | DRIVER_HAVE_IRQ;

	dev->driver->buf_priv_size	= sizeof(struct drm_nouveau_private);
	dev->driver->load		= nouveau_load;
	dev->driver->unload		= nouveau_unload;
	dev->driver->firstopen		= nouveau_firstopen;
	dev->driver->preclose		= nouveau_preclose;
	dev->driver->lastclose		= nouveau_lastclose;
	dev->driver->irq_preinstall	= nouveau_irq_preinstall;
	dev->driver->irq_postinstall	= nouveau_irq_postinstall;
	dev->driver->irq_uninstall	= nouveau_irq_uninstall;
	dev->driver->irq_handler	= nouveau_irq_handler;

	dev->driver->ioctls		= nouveau_ioctls;
	dev->driver->max_ioctl		= nouveau_max_ioctl;

	dev->driver->name		= DRIVER_NAME;
	dev->driver->desc		= DRIVER_DESC;
	dev->driver->date		= DRIVER_DATE;
	dev->driver->major		= DRIVER_MAJOR;
	dev->driver->minor		= DRIVER_MINOR;
	dev->driver->patchlevel		= DRIVER_PATCHLEVEL;
}

static int
nouveau_probe(device_t kdev)
{
	int vendor;

	if (pci_get_class(kdev) == PCIC_DISPLAY) {
		vendor = pci_get_vendor(kdev);
		if (vendor == 0x10de) {

			const char *ident;
			char model[64];

			if (pci_get_vpd_ident(kdev, &ident) == 0) {
				snprintf(model, 64, "%s", ident);
				device_set_desc_copy(kdev, model);
				DRM_DEBUG("VPD : %s\n", model);
			}

			return drm_probe(kdev, nouveau_pciidlist);
		}
	}
	return ENXIO;
}

static int
nouveau_attach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	nouveau_configure(dev);

	return drm_attach(kdev, nouveau_pciidlist);
}

static int
nouveau_detach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);
	int ret;

	ret = drm_detach(kdev);

	free(dev->driver, DRM_MEM_DRIVER);

	return ret;
}

static device_method_t nouveau_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		nouveau_probe),
	DEVMETHOD(device_attach,	nouveau_attach),
	DEVMETHOD(device_detach,	nouveau_detach),

	{ 0, 0 }
};

static driver_t nouveau_driver = {
#if __FreeBSD_version >= 700010
	"drm",
#else
	"drmsub",
#endif
	nouveau_methods,
	sizeof(struct drm_device)
};

extern devclass_t drm_devclass;
#if __FreeBSD_version >= 700010
DRIVER_MODULE(nouveau, vgapci, nouveau_driver, drm_devclass, 0, 0);
#else
DRIVER_MODULE(nouveau, agp, nouveau_driver, drm_devclass, 0, 0);
#endif
MODULE_DEPEND(nouveau, drm, 1, 1, 1);
