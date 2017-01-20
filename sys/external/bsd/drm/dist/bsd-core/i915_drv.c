/* i915_drv.c -- Intel i915 driver -*- linux-c -*-
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
#include "i915_drm.h"
#include "i915_drv.h"
#include "drm_pciids.h"

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t i915_pciidlist[] = {
	i915_PCI_IDS
};

#if defined(__FreeBSD__)
static int i915_suspend(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!dev || !dev_priv) {
		DRM_ERROR("dev: 0x%lx, dev_priv: 0x%lx\n",
			(unsigned long) dev, (unsigned long) dev_priv);
		DRM_ERROR("DRM not initialized, aborting suspend.\n");
		return -ENODEV;
	}

	i915_save_state(dev);

	return (bus_generic_suspend(kdev));
}

static int i915_resume(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);

	i915_restore_state(dev);

	return (bus_generic_resume(kdev));
}
#endif

static void i915_configure(struct drm_device *dev)
{
	dev->driver->driver_features =
	   DRIVER_USE_AGP | DRIVER_REQUIRE_AGP | DRIVER_USE_MTRR |
	   DRIVER_HAVE_IRQ;

	dev->driver->buf_priv_size	= sizeof(drm_i915_private_t);
	dev->driver->load		= i915_driver_load;
	dev->driver->unload		= i915_driver_unload;
	dev->driver->preclose		= i915_driver_preclose;
	dev->driver->lastclose		= i915_driver_lastclose;
	dev->driver->device_is_agp	= i915_driver_device_is_agp;
	dev->driver->enable_vblank	= i915_enable_vblank;
	dev->driver->disable_vblank	= i915_disable_vblank;
	dev->driver->irq_preinstall	= i915_driver_irq_preinstall;
	dev->driver->irq_postinstall	= i915_driver_irq_postinstall;
	dev->driver->irq_uninstall	= i915_driver_irq_uninstall;
	dev->driver->irq_handler	= i915_driver_irq_handler;

	dev->driver->ioctls		= i915_ioctls;
	dev->driver->max_ioctl		= i915_max_ioctl;

	dev->driver->name		= DRIVER_NAME;
	dev->driver->desc		= DRIVER_DESC;
	dev->driver->date		= DRIVER_DATE;
	dev->driver->major		= DRIVER_MAJOR;
	dev->driver->minor		= DRIVER_MINOR;
	dev->driver->patchlevel		= DRIVER_PATCHLEVEL;
}

#if defined(__FreeBSD__)
static int
i915_probe(device_t kdev)
{
	return drm_probe(kdev, i915_pciidlist);
}

static int
i915_attach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	i915_configure(dev);

	return drm_attach(kdev, i915_pciidlist);
}

static int
i915_detach(device_t kdev)
{
	struct drm_device *dev = device_get_softc(kdev);
	int ret;

	ret = drm_detach(kdev);

	free(dev->driver, DRM_MEM_DRIVER);

	return ret;
}

static device_method_t i915_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		i915_probe),
	DEVMETHOD(device_attach,	i915_attach),
	DEVMETHOD(device_suspend,	i915_suspend),
	DEVMETHOD(device_resume,	i915_resume),
	DEVMETHOD(device_detach,	i915_detach),

	{ 0, 0 }
};

static driver_t i915_driver = {
#if __FreeBSD_version >= 700010
	"drm",
#else
	"drmsub",
#endif
	i915_methods,
	sizeof(struct drm_device)
};

extern devclass_t drm_devclass;
#if __FreeBSD_version >= 700010
DRIVER_MODULE(i915, vgapci, i915_driver, drm_devclass, 0, 0);
#else
DRIVER_MODULE(i915, agp, i915_driver, drm_devclass, 0, 0);
#endif
MODULE_DEPEND(i915, drm, 1, 1, 1);

#elif   defined(__NetBSD__)

static bool
i915drm_suspend(device_t self, const pmf_qual_t *qual)
{
	struct drm_device *dev = device_private(self);

	i915_save_state(dev);
	return true;	
}

static bool
i915drm_resume(device_t self, const pmf_qual_t *qual)
{
	struct drm_device *dev = device_private(self);

	i915_restore_state(dev);
	return true;	
}

static int
i915drm_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	return drm_probe(pa, i915_pciidlist);
}

static void
i915drm_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct drm_device *dev = device_private(self);

	dev->driver = malloc(sizeof(struct drm_driver_info), DRM_MEM_DRIVER,
	    M_WAITOK | M_ZERO);

	i915_configure(dev);

	if (!pmf_device_register(self, i915drm_suspend, i915drm_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	drm_attach(self, pa, i915_pciidlist);
}

static int
i915drm_detach(device_t self, int flags)
{
	pmf_device_deregister(self);

	return drm_detach(self, flags);
}

CFATTACH_DECL_NEW(i915drm, sizeof(struct drm_device), i915drm_probe,
    i915drm_attach, i915drm_detach, NULL);

MODULE(MODULE_CLASS_DRIVER, i915drm, "drm");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
i915drm_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_i915drm,
		    cfattach_ioconf_i915drm, cfdata_ioconf_i915drm);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_i915drm,
		    cfattach_ioconf_i915drm, cfdata_ioconf_i915drm);
#endif
		break;
	default:
		return ENOTTY;
	}

	return error;
}

#endif
